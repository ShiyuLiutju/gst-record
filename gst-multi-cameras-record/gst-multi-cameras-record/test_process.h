/**
 * @file test_process.h
 * @brief
 *
 * @author Yu Jin
 * @version
 * @date Dec 16, 2017
 */

#ifndef _TEST_PROCESS_H_
#define _TEST_PROCESS_H_

#include <memory>
#include <string>
#include <vector>
#include "driver/camerav4l2.h"
#include "opencv2/core/core.hpp"
#include <queue>
#include <sys/time.h>
#include <thread>
#include <mutex>

struct FrameData{
    std::vector<cv::Mat> cameras;
    unsigned long long timestamp;
};

class TestProcess {
public:
    TestProcess();
    ~TestProcess();
    int Init(const int camera_number, const int capture_fps, const std::string v4l2_uri, const char *camera_open);
    void StartCapturing();
    int GetCameraBufferSize();
    int fps;
    std::queue<FrameData> cameraBuffer_;

private:
    static int Update(TestProcess*);
    void GetCameraOpenIndex(const char* camera_open, bool* camera_open_index);
    const int kSleepSecond = 1;
    std::vector<std::shared_ptr<CameraV4l2>> cameras_;
    std::vector<cv::Mat> data_;
    int camera_number_;
    int camera_open_size_;
    int durationTime_;
    bool enable;
    std::thread* thread_handle_;
    std::mutex mutex_;
};

#endif  // _TEST_PROCESS_H_
