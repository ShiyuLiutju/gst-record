#ifndef MULTICAMERASRECORD_H
#define MULTICAMERASRECORD_H
#include "config.h"
#include "multicamerasprocess.h"
#include "gst_record.h"
class MultiCamerasRecord
{
public:
    MultiCamerasRecord();
    ~MultiCamerasRecord();
    void setData(std::queue<FrameData> &cameraBuffer, bool *status);
    void Init(int _recordersNum, configData &configdata);
    void Start();
    void Stop();
    int recordersNum;
private:
    int fps;
    int bitrate;
    configData mconfigData;
    std::queue<FrameData> _cameraBuffer;
    bool *_status;
    std::vector<std::shared_ptr<GstRecorder>> recorders_;
};

#endif // MULTICAMERASRECORD_H
