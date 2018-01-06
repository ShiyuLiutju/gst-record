#ifndef MultiCamerasProcess_H
#define MultiCamerasProcess_H
#include "tcpconnect/tcp_client.h"
#include "opencv2/core/core.hpp"
#include "driver/camerav4l2.h"
#include "config.h"
#include <queue>
#include <signal.h>
#include <sys/time.h>
#include <mutex>

struct FrameData{
    std::vector<cv::Mat> cameras;
    unsigned long long timestamp;
};

class MultiCamerasProcess
{
public:
    //common
    int fps;
    int bitrate;
    int groupsNum;
    bool *bufferstatus;//hasData,isPop
    MultiCamerasProcess();
    ~MultiCamerasProcess();

    int Init(configData &_configdata);

    void StartCapturing();
    int GetCameraBufferSize();
    //v4l2 camera
    std::queue<FrameData> cameraBuffer_;


private:
    //commom
    void GetCameraOpenIndex();
    bool parseGroupInfo();
    int* cameraOpenIndex;
    configData mConfigdata;
    std::thread* thread_handle_;
    std::mutex mutex_;
    std::vector<int> *group;
    static int Update(MultiCamerasProcess*);



    //tcp camera
    TcpClient* mTcp = new TcpClient();
    int mTcpStatus;
    int tcpcameraOpenSize;


    //v4l2 camera
    int v4l2cameraOpenSize;
    std::vector<std::shared_ptr<CameraV4l2>> cameras_;
    std::vector<cv::Mat> data_;
    const int kSleepSecond = 1;
    int durationTime_;


};

#endif // MultiCamerasProcess_H
