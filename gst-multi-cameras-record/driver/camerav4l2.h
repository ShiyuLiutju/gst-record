#ifndef CAMERAV4L2_H
#define CAMERAV4L2_H
#include <thread>
#include <mutex>
#include <exception>
#include <string>
#include "ICamera.h"
#define PIXEK_FORMAT_RGB24  1
#define PIXEL_FORMAT_YUV420 2
#define PIXEL_FORMAT_YUV422 3
#define PIXEL_FORMAT_YUV    4
#define PIXEL_FORMAT_MJPEG  5

class V4l2Exception:std::exception
{
public:
    V4l2Exception(std::string msg)_GLIBCXX_USE_NOEXCEPT
    {
        msg_=msg;
    }
    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT
    {
        return msg_.c_str();
    }
private:
    std::string msg_;
};
typedef void (*error_process)(V4l2Exception e);
extern error_process camera_error; 
struct mmap_buffer;
class CameraV4l2:public ICamera
{
public:
    CameraV4l2(const char* dev);
    ~CameraV4l2();
    int height()
    {
        return height_;
    }

    int width()
    {
        return width_;
    }

    int pixel_format()
    {
        return pixel_format_;
    }

    bool Update();
    void* GetFrameData(unsigned long long& frame_index,unsigned long long& timestamp);
    unsigned char* data;
    void start_capturing();
    void Open(const char* dev);
    void Init();
    void stop_capturing();

private:
    char dev_[1000];
    unsigned char* yuv_device_;
    unsigned char* rgb_device_;
    static void process_loop(CameraV4l2* );
    void init_camera();
    void init_mmap();
    int fd_;
    int height_;
    int width_;
    int bytesperline_;
    int pixel_format_;
    std::thread* thread_handle_;
    std::mutex mutex_;
    //int enable_;
    unsigned char* camera_bufer_;
    mmap_buffer* user_buffer;
    bool enable_;
    bool has_data_;
    unsigned long long frame_index;
    unsigned long long timestamp;

    unsigned long long current_frame_index;
    unsigned long long current_timestamp;
};

#endif // CAMERAV4L2_H
