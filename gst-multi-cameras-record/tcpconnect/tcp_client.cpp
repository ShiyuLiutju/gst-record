/*
 * tcp_client.cpp
 *
 *  Created on: Jun 3, 2017
 *      Author: chengsq
 */
#include "tcp_connection.h"
#include "tcp_client.h"
#include <string.h>
#include "calmcar.prototxt.pb.h"
#include <glog/logging.h>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>

#define PIXEL_FORMAT_YUV422 1
#define PIXEL_FORMAT_RGB888 2
#define PIXEL_FORMAT_YUV420 3

char* command_get_info = (char*)"GET INFO\r\n";
char *command_start = (char*)"START\r\n";
#define FRAME_LENGHT 4096000
TcpClient::TcpClient() {
	tcp_ = NULL;
	frame_length_ = 0;
	pixel_format_ = -1;
	read_buffer_ = NULL;
}

TcpClient::~TcpClient() {
	// TODO Auto-generated destructor stub
}


int TcpClient::ConnectTo(char* ip, int port) {
	tcp_ = new TcpConnection();
    int connectStatus = tcp_->ConnectTo(ip, port);
    if(connectStatus == -1)
        return connectStatus;
	char msg[100];
	int len = tcp_->Read(msg, sizeof(msg));
    //LOG(INFO)<<msg;
	tcp_->Write(command_get_info, strlen(command_get_info));
	len = tcp_->Read(msg, sizeof(msg));
    //LOG(INFO)<<msg;
	msg[len] = 0;
	char pixel_format_s[10];
	char can_version[10];
	sscanf(msg, "WIDTH %d HEIGHT %d FORMAT %s CAN VERSION %s", &width_, &height_,
			pixel_format_s,can_version);
	tcp_->Write(command_start, strlen(command_start));
	std::string pixel_format_cstring = pixel_format_s;
	if (pixel_format_cstring == "YUV422") {
		pixel_format_ = PIXEL_FORMAT_YUV422;
		frame_length_ = width_ * height_ * 2 + 160 + 50;
	} else if (pixel_format_cstring == "RGB888") {
		pixel_format_ = PIXEL_FORMAT_RGB888;
		frame_length_ = width_ * height_ * 3;
	} else if (pixel_format_cstring == "YUV420") {
		pixel_format_ = PIXEL_FORMAT_YUV420;
		frame_length_ = width_ * height_ * 1.5 + 160 + 50;
	}
	read_buffer_ = new char[FRAME_LENGHT * 2];
	frame_length_=FRAME_LENGHT*2;
    //LOG(INFO)<<"frame_length_:"<<frame_length_;
    return connectStatus;
}

int TcpClient::ReadFrame(cv::Mat& frame)
{
    char package_header[9]={0};
    int read_length = 0;
    int curent_frame_length = 0;
    int head_len=12;
    while(read_length<head_len)
    {
       int size=tcp_->Read(read_buffer_+read_length,head_len-read_length);
       read_length+=size;
    }
    read_length=0;
    memcpy(package_header,read_buffer_,8);
    memcpy(&curent_frame_length, read_buffer_+8, 4);
    if(strncmp(package_header,"calmcar:",8)!=0)
    {
	//printf("head:%s\n",package_header);
        return -1;
    }
    if(frame_length_<curent_frame_length)
    {
        delete read_buffer_;
        read_buffer_=new char[curent_frame_length];
        frame_length_=curent_frame_length;
    }
    while (read_length < curent_frame_length)
    {
        int size = tcp_->Read(read_buffer_ + read_length, curent_frame_length - read_length);
        read_length += size;
    }
    CalmCarProto proto_data;
    proto_data.ParseFromArray(read_buffer_, read_length);
    if(proto_data.has_camera_data())
    {
        std::string image_data=proto_data.camera_data();
        if (pixel_format_ == PIXEL_FORMAT_YUV422)
        {
                cv::Mat yuv422_image(height_, width_, CV_8UC2, (char*)image_data.data());
                cv::cvtColor(yuv422_image, frame, CV_YUV2BGR_YUYV);
        }
        else if (pixel_format_ == PIXEL_FORMAT_RGB888)
        {
                if(image_data.length() == height_ * width_ * 3)
                {
                     frame=cv::Mat(height_,width_,CV_8UC3);
                     memcpy(frame.ptr(), image_data.data(), image_data.length());
                }
//                else
//                     LOG(ERROR)<< "image_data.length() != image_mat_.rows * image_mat_.cols * 3" << image_data.length();
         }
         else if(pixel_format_==PIXEL_FORMAT_YUV420)
         {
                cv::Mat yuv420_image(height_*1.5, width_, CV_8UC1, (char*)image_data.data());
                cv::cvtColor(yuv420_image, frame, CV_YUV2BGR_I420);
         }
    }

    return read_length;
}


char* TcpClient::GetFrameData()
{
	return NULL;
}
