#include "camerav4l2.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <glog/logging.h>
#include <cuda.h>
#include <nppi.h>
#include <cuda_runtime.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#define V4L2_CTRL_CLASS_CAMERA		0x009a0000	/* Camera class controls */
#define V4L2_CID_TEGRA_CAMERA_BASE	(V4L2_CTRL_CLASS_CAMERA | 0x2000)
#define V4L2_CID_ISP_BRIGHTNESS_TUNING  	(V4L2_CID_TEGRA_CAMERA_BASE+105)
error_process camera_error=nullptr;
struct BUFTYPE
{
    void *start;
    int length;
};
struct mmap_buffer
{
    BUFTYPE *user_buffer;
    int n_buffer;
};

CameraV4l2::CameraV4l2(const char* dev)
{
    Open(dev);
}

CameraV4l2::~CameraV4l2()
{
    stop_capturing();
}

void CameraV4l2::Open(const char* dev) {
    strcpy(dev_, dev);
    char text[100];
    fd_=access(dev,F_OK);
    if(fd_!=0)
    {
        sprintf(text,"can not access %s",dev);
        throw V4l2Exception(text);
    }
    if((fd_=open(dev,O_RDWR|O_NONBLOCK))<0)
    {
        sprintf(text,"can not open %s",dev);
        throw V4l2Exception(text);
    }
}

void CameraV4l2::Init() {
    init_camera();
    init_mmap();
    frame_index=0;
    timestamp=0;
}

void CameraV4l2::init_camera()
{
    struct v4l2_fmtdesc fmt;
    struct v4l2_capability cap;
    struct v4l2_format stream_fmt;
    int ret=0;
    fmt.index=0;
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while((ret=ioctl(fd_,VIDIOC_ENUM_FMT,&fmt))==0)
    {
        fmt.index++;
        std::cout<<"pixelformat = "<<(char)(fmt.pixelformat &0xff)<<(char)((fmt.pixelformat>>8)&0xff)<<(char)((fmt.pixelformat>>16)&0xff)<<(char)((fmt.pixelformat>>24)&0xff)<<",description = "<<fmt.description<<std::endl;
    }
    ret=ioctl(fd_,VIDIOC_QUERYCAP,&cap);
    if(ret<0)
    {
        LOG(ERROR)<<"Fail to ioctl VIDIOC_QUERYCAP";
        close(fd_);
        throw V4l2Exception("VIDIOC_QUERYCAP error");
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        LOG(ERROR)<<"The current device dose not support streaming i/o\n";
        close(fd_);
        throw V4l2Exception("Device not support streaming");
    }
    stream_fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd_,VIDIOC_G_FMT,&stream_fmt);
    height_=stream_fmt.fmt.pix.height;
    width_=stream_fmt.fmt.pix.width;
    //height_=960;
    //width_=1280;
    printf("get frame information, width=%d,height=%d\n",width_,height_);
    bytesperline_=stream_fmt.fmt.pix.bytesperline;

    //config camera brightness
    struct v4l2_control v4l2_ctrl;
    v4l2_ctrl.id=V4L2_CID_ISP_BRIGHTNESS_TUNING;
    ret=ioctl(fd_,VIDIOC_G_CTRL,&v4l2_ctrl);
    v4l2_ctrl.value=0x98ff;
    ret=ioctl(fd_,VIDIOC_S_CTRL,&v4l2_ctrl);
}

void CameraV4l2::init_mmap()
{
    int i=0;
    struct v4l2_requestbuffers reqbuf;
    bzero(&reqbuf,sizeof(reqbuf));
    reqbuf.count=4;
    reqbuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory=V4L2_MEMORY_MMAP;
    if(-1==ioctl(fd_,VIDIOC_REQBUFS,&reqbuf))
    {
        close(fd_);
        throw V4l2Exception("Fail to ioctl 'VIDIOC_REQBUFS'");
    }
    user_buffer=new mmap_buffer;
    user_buffer->n_buffer=reqbuf.count;
    user_buffer->user_buffer=new BUFTYPE[reqbuf.count];
    if(!user_buffer->user_buffer)
    {
        LOG(ERROR)<<"Out of memory";
        delete user_buffer;
        close(fd_);
        throw V4l2Exception("Can not malloc memory");
    }
    for(i=0;i<reqbuf.count;i++)
    {
        v4l2_buffer buf;
        bzero(&buf,sizeof(buf));
        buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory=V4L2_MEMORY_MMAP;
        buf.index=i;
        if(-1==ioctl(fd_,VIDIOC_QUERYBUF,&buf))
        {
            LOG(ERROR)<<"Fail to ioctl 'VIDIOC_QUERYBUF'";
            LOG(ERROR)<<"Out of memory";
            delete user_buffer;
            close(fd_);
            throw V4l2Exception("Can not set device mmap");
        }
        user_buffer->user_buffer[i].length=buf.length;
        user_buffer->user_buffer[i].start=
                mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd_,buf.m.offset
                     );
        if(MAP_FAILED==user_buffer->user_buffer[i].start)
        {
            LOG(ERROR)<<"mmap error";
            delete user_buffer;
            close(fd_);
            throw V4l2Exception("mmap failed");
        }
    }
}
void CameraV4l2::start_capturing()
{
    enum v4l2_buf_type type;
    for(int i=0;i<user_buffer->n_buffer;i++)
    {
        struct v4l2_buffer buf;
        bzero(&buf,sizeof(buf));
        buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory=V4L2_MEMORY_MMAP;
        buf.index=i;
        if(-1==ioctl(fd_,VIDIOC_QBUF,&buf))
        {
            LOG(ERROR)<<"Fail to ioctl 'VIDIOC_QBUF'\n";
            throw V4l2Exception("start capturing ioctl VIDIOC_QBUF error");
        }
    }
    type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1==ioctl(fd_,VIDIOC_STREAMON,&type))
    {
        LOG(ERROR)<<"Fail to ioctl 'VIDIOC_STREAMON'";
        throw V4l2Exception("start streaming error");
    }
    enable_=true;
    data=new unsigned char[height_*width_*3];
    camera_bufer_=new unsigned char[height_*width_*2];
    yuv_device_=NULL;
    rgb_device_=NULL;
    cudaMalloc(&yuv_device_,height_*width_*2);
    cudaMalloc(&rgb_device_,height_*width_*3);
    printf("camera width=%d,height=%d\n",width_,height_);
    has_data_=false;
    thread_handle_=new std::thread(process_loop,this);
}
void CameraV4l2::process_loop(CameraV4l2 *camera)
{
    printf("camera start read data\n");
    unsigned long long index=0;
    while(camera->enable_)
    {
        fd_set fds;
        timeval tv;
        FD_ZERO(&fds);
        FD_SET(camera->fd_,&fds);
        tv.tv_sec=0;
        tv.tv_usec=1000 * 50;
        int r=select(camera->fd_+1,&fds,NULL,NULL,&tv);
        time_t timestamp=clock();
        timestamp=timestamp/(float)CLOCKS_PER_SEC*1000;
        //printf("read data status %d\n",r);
        if(-1==r)
        {
            LOG(ERROR)<<"camera error";
            //throw V4l2Exception("select error");
            if(camera_error)
                camera_error(V4l2Exception("select error"));
        }
        else if(r==0)
        {
            //LOG(ERROR)<<"camera timeout";
        }
        else if(r!=0)
        {
            v4l2_buffer buf;
            unsigned int i;
            bzero(&buf,sizeof(buf));
            buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory=V4L2_MEMORY_MMAP;
            if(-1==ioctl(camera->fd_,VIDIOC_DQBUF,&buf))
            {
                LOG(ERROR)<<"VIDIOC_DQBUF error\n";
                //throw V4l2Exception("process loop ioctl VIDIOC_DQBUF error");
                if(camera_error)
                {
                    camera->has_data_=true;
                    char err_msg[1000];
                    sprintf(err_msg, "%s ioctl VIDIOC_DQBUF error", camera->dev_);
                    camera_error(V4l2Exception(err_msg));
                }
                //break;
                return;
            }
            if(buf.index>camera->user_buffer->n_buffer)
            {
                LOG(INFO)<<"buffer index error";
                continue;
            }
            unsigned char* yuv_10bit=(unsigned char*)camera->user_buffer->user_buffer[buf.index].start;
            unsigned char* yuv_8bit=camera->camera_bufer_;
            int len=camera->height_*camera->width_*2;

//            for(int i=0;i<len;i+=4)
//            {
//                unsigned char U=yuv_10bit[i];
//                unsigned char Y1=
//                        yuv_8bit[i]=yuv_10bit[i+1];
//                yuv_8bit[i+1]=yuv_10bit[i];
//                yuv_8bit[i+2]=yuv_10bit[i+3];
//                yuv_8bit[i+3]=yuv_10bit[i+2];
//            }

            memcpy(yuv_8bit,yuv_10bit,len);
            ioctl(camera->fd_,VIDIOC_QBUF,&buf);
            camera->mutex_.lock();
            cudaMemcpy(camera->yuv_device_,yuv_8bit,len,cudaMemcpyHostToDevice);
            camera->has_data_=true;
            camera->frame_index=index;
            camera->timestamp=timestamp;
            camera->mutex_.unlock();
//            cv::Mat yuv(camera->height_,camera->width_,CV_8UC2,yuv_8bit);
//            cv::Mat rgb;
//            cv::cvtColor(yuv,rgb,CV_YUV2BGR_Y422);
//            cv::resize(rgb,rgb,cv::Size(320,240));
//            cv::imshow("frame",rgb);
//            cv::waitKey(1);
            index++;
        }

    }
}
void CameraV4l2::stop_capturing()
{
    LOG(INFO)<<"release camera";
    enum v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    enable_=false;
    if (thread_handle_ && thread_handle_->joinable())
        thread_handle_->join();
    delete thread_handle_;
    ioctl(fd_,VIDIOC_STREAMOFF,&type);
    if(user_buffer)
    {
        for(int i=0;i<user_buffer->n_buffer;i++)
        {
            munmap(user_buffer->user_buffer[i].start,user_buffer->user_buffer[i].length);
        }
        delete[] user_buffer->user_buffer;
        user_buffer->user_buffer=NULL;
        delete user_buffer;
        user_buffer=NULL;
    }
    close(fd_);
}
bool CameraV4l2::Update()
{
    while(1)
    {
        if(has_data_)
        {
            has_data_=false;
            mutex_.lock();
            current_frame_index=frame_index;
            current_timestamp=timestamp;
            NppiSize roi;
            roi.height=height_;
            roi.width=width_;
            NppStatus status=nppiYUV422ToRGB_8u_C2C3R(yuv_device_,width_*2,rgb_device_,width_*3,roi);
            if(status!=NPP_SUCCESS)
            {
                LOG(ERROR)<<"yuv422 convert to rgb error:"<<status;
                mutex_.unlock();
                return false;
            }
            mutex_.unlock();
            cudaMemcpy(data,rgb_device_,width_*height_*3,cudaMemcpyDeviceToHost);
            int len=width_*height_*3;
            for(int i=0;i<len;i+=3)
            {
                unsigned char R=data[i];
                data[i]=data[i+2];
                data[i+2]=R;
            }
            return true;
        }
        else
        {
            usleep(10);
        }
    }


}
void* CameraV4l2::GetFrameData(unsigned long long& frame_index,unsigned long long& timestamp)
{
    frame_index=current_frame_index;
    timestamp=current_timestamp;
    return data;
}
