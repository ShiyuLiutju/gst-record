#include "gst_record.h"
#include <glog/logging.h>
#include <time.h>

inline std::string TimeToString() {
    time_t t=time(0);
    char time_c[1024];
    strftime(time_c, sizeof(time_c), "%Y-%m-%d_%H-%M-%S", localtime(&t));
    return std::string{time_c};
}

GstRecorder::GstRecorder()
{
    isInited = false;
}

GstRecorder::~GstRecorder()
{
    /* Free resources */
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(data.pipeline));
    g_source_remove (bus_watch_id);
}

int GstRecorder::setsavefile(std::string path, std::string prefix)
{
    std::string timestr = TimeToString();
    if(!prefix.empty())
    {
        data.save_name=g_strdup_printf("%s%s-group-%02d-%s.mp4",
                                       path.c_str(),prefix.c_str(),bufferId,timestr.c_str());
    }
    else
    {
        data.save_name=g_strdup_printf("%sgroup-%02d-%s.mp4",
                                       path.c_str(),bufferId,timestr.c_str());
    }
    if(data.sink)
    {
        g_object_set (data.sink, "location", data.save_name, NULL);
        return 0;
    }
    else
    {
        g_print("save file name set error.\n");
        return -1;
    }
}


bool GstRecorder::cb_need_data (GstElement * appsrc,guint unused, GstRecorder * ctx)
{
    GstBuffer *buffer;

    GstFlowReturn ret;

    buffer = gst_buffer_new_allocate (NULL, ctx->buffersize, NULL);


    gst_buffer_fill(buffer, 0, ctx->_cameraBuffer.front().cameras[ctx->bufferId].ptr<uchar>(), ctx->buffersize);


    GST_BUFFER_PTS (buffer) = ctx->data.timestamp;

    GST_BUFFER_DURATION (buffer) = ctx->data.duration;

    ctx->data.timestamp += GST_BUFFER_DURATION (buffer);

    g_signal_emit_by_name ((GstElement*)appsrc, "push-buffer", buffer, &ret);


    if(ret !=  GST_FLOW_OK)
    {
        LOG(ERROR)<<"push buffer";
        g_printerr("push buffer returned %d for %d bytes \n", ret, ctx->buffersize);
    }

    gst_buffer_unref (buffer);

    return TRUE;
}

void GstRecorder::start_pipeline()
{
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
}

void GstRecorder::paused_pipeline(){
    gst_element_set_state(data.pipeline, GST_STATE_PAUSED);
}

void GstRecorder::eos_pipeline()
{
    gst_element_send_event(data.pipeline, gst_event_new_eos());
}

void GstRecorder::setData(std::queue<FrameData> &cameraBuffer, bool *status){

    _cameraBuffer = cameraBuffer;
    _status = status;
}
bool GstRecorder::init(unsigned int bufferId, unsigned int fps,unsigned int bitrate)
{

    isInited = false;

    this->bufferId = bufferId;
    //init
    width=_cameraBuffer.front().cameras[bufferId].cols;
    height=_cameraBuffer.front().cameras[bufferId].rows;
    fps_ = fps;
    buffersize = width * height*3;
    this->bufferId = bufferId;
    this->bitrate = bitrate;
    data.timestamp = 0;
    data.duration = gst_util_uint64_scale_int (1, GST_SECOND, fps);

    gst_init(NULL,NULL);

    data.pipeline = gst_pipeline_new(NULL);

    data.appsrc =(GstAppSrc*)gst_element_factory_make ("appsrc", NULL);

    g_object_set (G_OBJECT ((GstElement*)data.appsrc), "caps",
                  gst_caps_new_simple ("video/x-raw",
                                       "format", G_TYPE_STRING, "BGR",
                                       "width", G_TYPE_INT, width,
                                       "height", G_TYPE_INT, height,
                                       "framerate", GST_TYPE_FRACTION, fps_, 1,
                                       NULL), NULL);


    g_object_set (G_OBJECT (data.appsrc), "stream-type", 0,"format", GST_FORMAT_TIME, NULL);

    g_signal_connect(data.appsrc, "need-data", G_CALLBACK((cb_need_data)), this);

    data.conv = gst_element_factory_make ("videoconvert", "conv");
    data.encoder = gst_element_factory_make("omxh264enc", NULL);
    g_object_set (data.encoder, "bitrate", bitrate, NULL);
    data.parse = gst_element_factory_make("h264parse", NULL);
    data.mux = gst_element_factory_make("qtmux", NULL);
    data.sink = gst_element_factory_make("filesink", NULL);

    if (!data.pipeline || !data.appsrc || !data.conv || !data.encoder|| !data.parse||!data.mux || !data.sink)
    {
        LOG(ERROR)<<"One element could not be created.Exiting.\n";
        return false;
    }
    //add to pipeline
    gst_bin_add_many(GST_BIN(data.pipeline), (GstElement*)data.appsrc,data.conv,data.encoder,data.parse,data.mux, data.sink, NULL);
    //link
    if(gst_element_link((GstElement*)data.appsrc,data.conv) != TRUE)
    {
        g_print("Elements appsrc,conv could not be linked.\n");
        return false;
    }

    GstCaps *caps = gst_caps_new_simple ("video/x-raw",
                                         "format", G_TYPE_STRING, "I420",
                                         "width", G_TYPE_INT, width,
                                         "height", G_TYPE_INT, height,
                                         "framerate", GST_TYPE_FRACTION, fps_, 1,
                                         NULL);
    if(!caps)
    {
        LOG(ERROR)<<"gst_caps_new_simple failed.\n";
        return false;
    }
    if(!gst_element_link_filtered (data.conv, data.encoder, caps))
    {
        LOG(ERROR)<<"Elements conv, encoder could not be linked.\n";
        return false;
    }
    gst_caps_unref(caps);

    caps = gst_caps_new_simple ("video/x-h264","stream-format", G_TYPE_STRING, "byte-stream",NULL);
    if(!caps)
    {
        g_print("caps fail");
        return false;
    }
    if(!gst_element_link_filtered (data.encoder, data.parse, caps))
    {
        g_print("Elements encoder, parse could not be linked.\n");
        return false;
    }
    gst_caps_unref(caps);

    if(gst_element_link_many(data.parse,data.mux,data.sink,NULL)!= TRUE)
    {
        g_print("Elements parse,mux,sink could not be linked.\n");
        return false;
    }


    data.bus = gst_pipeline_get_bus(GST_PIPELINE(data.pipeline));

    bus_watch_id = gst_bus_add_watch(data.bus, bus_call, &data);

    gst_object_unref (data.bus);

    isInited = true;
}


gboolean GstRecorder::bus_call(GstBus *bus, GstMessage *msg, gpointer data_)
{
    CustomData *data = (CustomData*)data_;
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR: {
        GError *err;
        gchar *debug;

        gst_message_parse_error (msg, &err, &debug);
        g_print ("Error: %s\n", err->message);
        g_error_free (err);
        g_free (debug);

        gst_element_set_state (data->pipeline, GST_STATE_READY);
        //g_main_loop_quit (data->loop);
        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        g_print ("End-Of-Stream reached.\n");
        gst_element_set_state (data->pipeline, GST_STATE_READY);
        //g_main_loop_quit (data->loop);
        break;
    case GST_MESSAGE_BUFFERING: {
        gint percent = 0;

        /* If the stream is live, we do not care about buffering. */
        //if (data->is_live) break;

        gst_message_parse_buffering (msg, &percent);
        g_print ("Buffering (%3d%%)\r", percent);
        /* Wait until buffering is complete before start/resume playing */
        if (percent < 100)
            gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
        else
            gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
        break;
    }
    case GST_MESSAGE_CLOCK_LOST:
        /* Get a new clock */
        gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
        gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
        break;
    default:
        /* Unhandled message */
        break;
    }

}

