#ifndef ICAMERA_H
#define ICAMERA_H
class ICamera
{
public:
    ICamera(){}
    virtual ~ICamera(){}
    virtual void* GetFrameData(unsigned long long& frame_index,unsigned long long& timestamp)=0;
    virtual bool Update()=0;
};
#endif
