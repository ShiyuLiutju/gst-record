
#ifndef GST_RECORD_H
#define GST_RECORD_H
#endif

#include <gst/gst.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <gst/app/gstappsrc.h>
#include <mutex>
#include <queue>
#include <gst/net/gstnet.h>
#include "multicamerasprocess.h"

typedef struct _CustomData {
    GstElement* pipeline;
    GstAppSrc  *appsrc;
    GstElement   *conv;
    GstElement* encoder;
    GstElement* parse;
    GstElement* mux;
    GstElement* sink;
    GstBus *bus;
    gchar* save_name;
    GstClockTime timestamp;
    guint64 duration;
} CustomData;

class GstRecorder{

public:
    GstRecorder();
    ~GstRecorder();

    bool init(unsigned int bufferId, unsigned int fps, unsigned int bitrate);
    int setsavefile(std::string path, std::string prefix="");

    void setData(std::queue<FrameData> &cameraBuffer,bool *status);

    void start_pipeline();
    void paused_pipeline();
    void eos_pipeline();

private:
    static bool cb_need_data(GstElement * appsrc,guint unused, GstRecorder * ctx);
    static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);

public:
    int width;
    int height;
    int bitrate;
    int fps_;

    CustomData data;
    std::queue<FrameData> _cameraBuffer;
    bool *_status;
    std::mutex mtx;

    int stop=0;

private:
    guint buffersize;
    unsigned int bufferId;
    bool isPlaying=false;
    bool isInited;
    guint bus_watch_id;
};



