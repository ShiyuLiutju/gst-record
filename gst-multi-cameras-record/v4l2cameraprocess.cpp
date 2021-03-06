/**
 * @file test_process.cpp
 * @brief
 *
 * @author Yu Jin
 * @version
 * @date Dec 16, 2017
 */

#include "v4l2cameraprocess.h"
#include <signal.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include "glog/logging.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


inline double TimeSince(const std::chrono::steady_clock::time_point& tp0) {
    auto tp1 = std::chrono::steady_clock::now();
    double ret =
            std::chrono::duration_cast<std::chrono::duration<double>>(tp1 - tp0)
                                                                     .count();
    return ret;
}

bool g_enable=true;
void sigRoutine(int number) {
    g_enable = false;
}

V4l2CameraProcess::V4l2CameraProcess() : enable(true) {
    signal(SIGHUP, sigRoutine);
    signal(SIGINT, sigRoutine);
    signal(SIGKILL, sigRoutine);
}

V4l2CameraProcess::~V4l2CameraProcess() {}
void Camera_error_process(V4l2Exception e)
{
    g_enable=false;
    printf("%s\n", e.what());
}

int V4l2CameraProcess::Init(const int camera_number, const int capture_fps, const std::string v4l2_uri, const char *camera_open) {

}

void V4l2CameraProcess::GetCameraOpenIndex(const char* camera_open, bool* camera_open_index){

    camera_open_size_ = 0;
    for(int i=1;i<strlen(camera_open);i++)
    {
        if(camera_open[i]=='1')
        {
            camera_open_index[i-1] = true;
            camera_open_size_++;
        }
    }

}
int V4l2CameraProcess::GetCameraBufferSize(){
    mutex_.lock();
    int size = cameraBuffer_.size();
    mutex_.unlock();
    return size;
}
void V4l2CameraProcess::StartCapturing(){

    thread_handle_=new std::thread(Update,this);
}

int V4l2CameraProcess::Update(V4l2CameraProcess* proc) {
    unsigned long long time_stamp;
    while (g_enable) {

        FrameData frameData;

        bool ret = true;
        //get timeval:tv_sec,tv_usec
        timeval tv;
        gettimeofday(&tv,NULL);

        time_stamp = tv.tv_sec*1000000 + tv.tv_usec;

        for (int i = 0; i < proc->camera_open_size_; ++i) {
            ret &= proc->cameras_[i]->Update();
            if (!ret) {
                break;
            }
            unsigned long long innerframecount_,innertimestamp_;

            memcpy(proc->data_[i].data,
                   (unsigned char*)(proc->cameras_[i]->GetFrameData(innerframecount_,
                                                              innertimestamp_)),
                   proc->cameras_[i]->height() * proc->cameras_[i]->width() * 3);
            //push data
            frameData.cameras.push_back(proc->data_[i]);

        }

        if(frameData.cameras.size()==proc->camera_open_size_){

            gettimeofday(&tv,NULL);

            time_stamp = proc->durationTime_ - tv.tv_sec*1000000 - tv.tv_usec + time_stamp;
            time_stamp = (time_stamp > 0 ? time_stamp:0);
            printf("frameData.cameras.size():%d,%d,%ld\n",frameData.cameras.size(),proc->camera_open_size_,time_stamp);
            //usleep(time_stamp);

            frameData.timestamp = time_stamp;
            proc->mutex_.lock();
            proc->cameraBuffer_.push(frameData);
            proc->mutex_.unlock();
            //printf("frameData.cameras.size():%d,%d,%ld,%d\n",frameData.cameras.size(),proc->size_,time_stamp,proc->cameraBuffer_.size());
        }
    }
}

//cv::Mat frame{data_[0].clone()};
//for (int i = 1; i < size_; ++i) {
//    cv::vconcat(frame, data_[i], frame);
//}
//cv::resize(frame, frame, cv::Size(frame.cols / 4, frame.rows / 4));
//cv::imshow("camera", frame);
//if (cv::waitKey(1) == 'q') {
//    break;
//}
