#pragma once
#include "headers/NetTcp.h"
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <opencv2/core/core.hpp>
#include <thread>
#include <mutex>



//class FrameData;
class CalmCarClient
{
public:
    CalmCarClient();
    ~CalmCarClient();
    int connect(char *ip, short port);
    void reconnect();
    int ReadFrame(cv::Mat &data);
	void close();
	int width;
	int height;
	int pixel_format;
    bool enable_=false;
private:
    void read();
private:
    bool stop=true;
    NetTcp *client__;
    int read_buffer_size;
	char *read_buff;
    int read_header_size;
    char* read_header;
    cv::Mat image_mat_;
    std::mutex lock_;
    std::thread* readthread_=NULL;
  //  FrameData* frame=NULL;
    cv::Mat outputImage;
    bool readenable=false;
    std::string ip_="";
    short port_=8000;
};
