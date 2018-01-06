#include "multicamerasrecord.h"

MultiCamerasRecord::MultiCamerasRecord()
{
    recordersNum = 0;
}

MultiCamerasRecord::~MultiCamerasRecord()
{

}


void MultiCamerasRecord::setData(std::queue<FrameData> &cameraBuffer, bool *status)
{
    _cameraBuffer = cameraBuffer;
    _status = status;
}

void MultiCamerasRecord::Init(int _recordersNum, configData &configdata)
{
    mconfigData = configdata;
    fps = mconfigData.default_fps;
    bitrate = mconfigData.default_bitrate;

    recordersNum = _recordersNum;

    recorders_.resize(recordersNum);
    //create recorder
    for(unsigned int i = 0;i<recordersNum;i++)
    {
        recorders_[i] = std::shared_ptr<GstRecorder> {new GstRecorder()};
    }
    //init recorder
    for(unsigned int i = 0;i<recordersNum;i++)
    {
        recorders_[i]->setData(_cameraBuffer,_status);
        recorders_[i]->init(i,fps,bitrate);
        if(recorders_[i]->setsavefile(configdata.u_dir))
            return ;
    }
}
bool MultiCamerasRecord::Start()
{
    for(unsigned int i = 0;i<recordersNum;i++)
    {  if(i=0)
		{
			
		
        recorders_[i]->start_pipeline();
    
		}
       else	
	   {   while(recorders__[i-1]->ispaused==false)
		   {
		   }
		   if(recorders__[i-1]->ispaused==true)
		   {
	    recorders_[i]->start_pipeline();
		   }
	   }
	   
	}
	 while(recorders__[recordersNum-1]->ispaused==false)
		   {
		   }
    return true
}
void MultiCamerasRecord::Stop()
{
    for(unsigned int i = 0;i<recordersNum;i++)
    {
        recorders_[i]->eos_pipeline();
    }
}

