#include "CalmCarClient.h"
#include <string>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "calmcar.prototxt.pb.h"


#define PIXEL_FORMAT_YUV422 1
#define PIXEL_FORMAT_RGB888 2
#define PIXEL_FORMAT_YUV420 3
#define PIXEL_FORMAT_UYVY   4

char* command_get_info = (char*)"GET INFO\r\n";
char *command_start = (char*)"START\r\n";

CalmCarClient::CalmCarClient()
{
    client__ = NULL;
	//frame = new FrameData();
}


CalmCarClient::~CalmCarClient()
{
    close();
    //qDebug()<<"client close";
}

int CalmCarClient::connect(char *ip, short port)
{
    this->ip_=ip;
    this->port_=port;
    client__->Init();
    client__ = new NetTcp();

    client__->ConnectTo(const_cast<char*>(this->ip_.c_str()), this->port_);
    char msg[100];
    int len = client__->Read(msg, sizeof(msg));
	if (len == SOCKET_ERROR)
	{
		//////LOG(INFO) <<"connect error "<<"SOCKET_ERROR:"<< len;
        /// return -1;
	}
	else
	{
		//////LOG(INFO) << len;
	}
    client__->Write(command_get_info, strlen(command_get_info));
    len = client__->Read(msg, sizeof(msg));
    msg[len] = 0;
    char pixel_format_s[10];
    char can_version[10];
    sscanf(msg, "WIDTH %d HEIGHT %d FORMAT %s CAN VERSION", &width, &height, pixel_format_s, can_version);
    //////LOG(INFO)<<msg;
    printf("%s\n",msg);
    client__->Write(command_start, strlen(command_start));
    std::string pixel_format_cstring = pixel_format_s;
    read_buffer_size = 1024*1024*16;
    read_buff = new char[read_buffer_size];
    read_header=new char[10];
    if (pixel_format_cstring == "YUV422")
    {
        pixel_format = PIXEL_FORMAT_YUV422;
    }
    else if (pixel_format_cstring == "RGB888")
    {
        pixel_format = PIXEL_FORMAT_RGB888;
    }
	else if (pixel_format_cstring == "YUV420")
	{
		pixel_format = PIXEL_FORMAT_YUV420;
	}
	else if (pixel_format_cstring == "UYVY")
	{
		pixel_format = PIXEL_FORMAT_UYVY;
	}
    image_mat_ = cv::Mat(height, width, CV_8UC3);
    enable_=true;
    stop=false;
    readthread_=new std::thread(std::mem_fn(&CalmCarClient::read),this);
}

void CalmCarClient::reconnect()
{
    ////LOG(ERROR)<<"reconnect";
    if(client__)
    {
        delete client__;
        client__=NULL;
        client__=new NetTcp();
		try
		{
			client__->ConnectTo(const_cast<char*>(this->ip_.c_str()), this->port_);
		}
		catch (NetException& ex)
		{
		////LOG(ERROR) << ex.what();
            return;
		}
        char msg[100];
        int len = client__->Read(msg, sizeof(msg));
        ////LOG(INFO)<<len;
        client__->Write(command_get_info, strlen(command_get_info));
        len = client__->Read(msg, sizeof(msg));
        msg[len] = 0;
        char pixel_format_s[10];
        char can_version[10];
        sscanf(msg, "WIDTH %d HEIGHT %d FORMAT %s CAN VERSION", &width, &height, pixel_format_s, can_version);
        ////LOG(INFO)<<msg;
        client__->Write(command_start, strlen(command_start));
        std::string pixel_format_cstring = pixel_format_s;
        if (pixel_format_cstring == "YUV422")
        {
            pixel_format = PIXEL_FORMAT_YUV422;
        }
        else if (pixel_format_cstring == "RGB888")
        {
            pixel_format = PIXEL_FORMAT_RGB888;
        }
        else if (pixel_format_cstring == "YUV420")
        {
            pixel_format = PIXEL_FORMAT_YUV420;
        }
		else if (pixel_format_cstring == "UYVY")
		{
			pixel_format = PIXEL_FORMAT_UYVY;
		}
    }
}

void CalmCarClient::read()
{
    while(enable_)
    {
        if(stop)
        {
            ////LOG(INFO)<<"CalmCarClient stop";
			return;
        }
        int readed_len = 0;
        int total_len = 0;
        //TimeProfile profile;
        //profile.Reset();
        memset(read_buff,0,12);
		int headreaded_len = 0;
		int headtotal_len = 12;
		int headread_ret = 0;
		while (headreaded_len < headtotal_len)
		{
			headread_ret = client__->Read(read_buff + headreaded_len, headtotal_len - headreaded_len);
			if (headread_ret == SOCKET_ERROR)
			{
				break;
			}
			if (headread_ret == 0)
			{
				////LOG(ERROR) << "hdader reader length=0";
				reconnect();
			}
			headreaded_len += headread_ret;
		}
		if (headread_ret == SOCKET_ERROR)
		{
			////LOG(ERROR) << "calmcar header read error";
			reconnect();
			continue;
		}
        memcpy(read_header,read_buff,8);
        memcpy(&total_len, read_buff + 8, 4);
        int result=strncmp(read_buff,"calmcar:",8);        
        if(result!=0)
        {
            ////LOG(ERROR)<<"calmcar header error";
            reconnect();
            continue;
        }
        ////LOG(INFO) << "total_len:" << total_len;
        if(total_len > read_buffer_size)
        {
            ////LOG(ERROR)<<"total_len > buffer size"<<total_len;
            reconnect();
            continue;
        }
		int bodyread_ret = 0;
        while (readed_len < total_len)
        {
			bodyread_ret = client__->Read(read_buff + readed_len, total_len - readed_len);
			if (bodyread_ret == SOCKET_ERROR)
            {               
				break;
            }
			if (bodyread_ret == 0)
			{
				////LOG(ERROR) << "body reader length=0";
				reconnect();
			}
			readed_len += bodyread_ret;
        }
		if (bodyread_ret == SOCKET_ERROR)
		{
            ////LOG(ERROR) << "socket recv status:" << "ret=" << bodyread_ret;
			reconnect();
			continue;
		}
        //profile.Update("TCP_READ");

        CalmCarProto proto_data;
        proto_data.ParseFromArray(read_buff, total_len);
        if (proto_data.has_camera_data())
        {
            std::string image_data = proto_data.camera_data();
            if (pixel_format == PIXEL_FORMAT_YUV422)
            {
                cv::Mat yuv422_image(height, width, CV_8UC2, (char*)image_data.data());
                cv::cvtColor(yuv422_image, image_mat_, CV_YUV2BGR_YUYV);
            }
            else if (pixel_format == PIXEL_FORMAT_RGB888)
            {
                if(image_data.length() == image_mat_.rows * image_mat_.cols * 3)
                     memcpy(image_mat_.data, image_data.data(), image_data.length());
               // else
                     ////LOG(ERROR)<< "image_data.length() != image_mat_.rows * image_mat_.cols * 3" << image_data.length();
            }
            else if(pixel_format==PIXEL_FORMAT_YUV420)
            {
                cv::Mat yuv420_image(height*1.5, width, CV_8UC1, (char*)image_data.data());
                cv::cvtColor(yuv420_image, image_mat_, CV_YUV2BGR_I420);
            }
			else if (pixel_format == PIXEL_FORMAT_UYVY)
			{
				cv::Mat yuv422_image(height, width, CV_8UC2, (char*)image_data.data());
				cv::cvtColor(yuv422_image, image_mat_, CV_YUV2BGR_Y422);
			}
        }
        lock_.lock();
        image_mat_.copyTo(outputImage);
        lock_.unlock();
       readenable = true;
        ////LOG(INFO)<<"total_len:"<<profile.GetTimeProfileString();
#ifdef WIN32
		Sleep(0);
#else
       usleep(10);
#endif
    }
}

int CalmCarClient::ReadFrame(cv::Mat &data)
{
    if(lock_.try_lock())
    {
        if(outputImage.data&&readenable)
        {
            outputImage.copyTo(data);
            outputImage.release();
            readenable=false;
            lock_.unlock();
            return 1;
        }
        lock_.unlock();
        return -1;
    }
    return -1;
}
void CalmCarClient::close()
{

    if(readthread_)
    {        
        stop=true;
		enable_ = false;
        readthread_->join();
        delete readthread_;        
        readthread_=NULL;
    }

    if (client__)
    {
        client__->close();
        delete client__;
        client__ = NULL;
    }
}

