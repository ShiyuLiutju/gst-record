/**
 * @file main.cpp
 * @brief
 *
 * @version
 * @date Dec 16, 2017
 */

#include <signal.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <signal.h>
#include "glog/logging.h"
#include "multicamerasprocess.h"
#include "config.h"
#include "multicamerasrecord.h"


bool isRun = true;

void myhander(int number){
    isRun = false;
}

int main(int argc, char* argv[]) {

    signal(SIGQUIT, myhander);
    signal(SIGINT,myhander);
    signal(SIGTERM,myhander);

    google::InitGoogleLogging("green-test");

    Config* config_=NULL;
    configData configdata;
    config_ = Config::Instance();

    if(config_->Load("../config.json"))
    {

        std::string ipstr=(std::string)config_->Value("RecordInfo","ip");
        configdata.default_ip=(char *)ipstr.c_str();
        configdata.default_port=(int)config_->Value("RecordInfo","port");
        configdata.uri_v4l2=(std::string)config_->Value("RecordInfo","uri_v4l2");
        configdata.camera_number=(int)config_->Value("RecordInfo","camera_number");
        configdata.default_bitrate=(int)config_->Value("RecordInfo","bitrate");
        configdata.default_fps=(int)config_->Value("RecordInfo","fps");
        configdata.u_dir=(std::string)config_->Value("RecordInfo","u_dir");
        if(access(configdata.u_dir.c_str(),W_OK | F_OK)!=0)
        {
            printf("u_dir set error.\n");
            return -1;
        }
        std::string copenstr=(std::string)config_->Value("RecordInfo","camera_open");
        configdata.camera_open=(char *)copenstr.c_str();
        std::string group=(std::string)config_->Value("RecordInfo","group1");
        configdata.group.push_back(group);
        group=(std::string)config_->Value("RecordInfo","group2");
        configdata.group.push_back(group);
        group=(std::string)config_->Value("RecordInfo","group3");
        configdata.group.push_back(group);
    }
    else
    {

        LOG(ERROR)<<"Load config.json error.\n";
        return -1;
    }
    LOG(INFO)<<"Load config.json success.\n";

    std::shared_ptr<MultiCamerasProcess> proc{ new MultiCamerasProcess() };
    if (proc->Init(configdata) != 0) {
        LOG(INFO)<<"Camera initialization error.\n";
        return -1;
    }
    else{
        proc->StartCapturing();
    }

    //wait for camera buffer
    while(proc->GetCameraBufferSize()==0);

    MultiCamerasRecord* record = new MultiCamerasRecord();
    record->setData(proc->cameraBuffer_,proc->bufferstatus);
    record->Init(proc->groupsNum,configdata);

    //    //init camera information
    while(isRun)
    { 
      if (proc->bufferstatus[1]==false)
	  {
     record->Start();
      } 
	}
    printf("stop1...\n");
    record->Stop();
    printf("stop2...\n");
    return 0;
}
