#include "multicamerasprocess.h"
#include <unistd.h>
bool g_enable=true;
void sigRoutine(int number) {
    g_enable = false;
}

void Camera_error_process(V4l2Exception e)
{
    g_enable=false;
    printf("%s\n", e.what());
}

MultiCamerasProcess::MultiCamerasProcess()
{
    signal(SIGHUP, sigRoutine);
    signal(SIGINT, sigRoutine);
    signal(SIGKILL, sigRoutine);


}

MultiCamerasProcess::~MultiCamerasProcess()
{
    delete []cameraOpenIndex;

    //    g_enable = false;
    //    if (thread_handle_ && thread_handle_->joinable())
    //        thread_handle_->join();
    //    delete thread_handle_;
}

int MultiCamerasProcess::Init(configData &_configdata)
{
    mConfigdata = _configdata;
    fps = mConfigdata.default_fps;
    durationTime_ = 1000000/fps;
    bitrate = mConfigdata.default_bitrate;
    bufferstatus = new bool[2]();
    group = new std::vector<int>[mConfigdata.group.size()];

    if(!parseGroupInfo())
    {
        return -1;
    }

    //
    if(v4l2cameraOpenSize + tcpcameraOpenSize == 0)
    {
        printf("v4l2 cameras and tcp camera are not selected.\n");
        return -1;
    }



    sleep(kSleepSecond);

    //v4l2 camer init

    if(!v4l2cameraOpenSize)
    {
        printf("v4l2 cameras are not selected.\n");
    }
    else
    {

        camera_error=Camera_error_process;

        cameras_.resize(v4l2cameraOpenSize);
        data_.resize(v4l2cameraOpenSize);

        std::string uri = mConfigdata.uri_v4l2;

        printf("*** Open cameras ***\n");

        int camera_index = 0;
        for (int i = 1; i < mConfigdata.camera_number; ++i) {
            if(cameraOpenIndex[i]>=0){
                uri[10] = '0' + i-1;
                try {
                    cameras_[camera_index] =
                            std::shared_ptr<CameraV4l2>{new CameraV4l2(uri.c_str())};
                    camera_index++;
                } catch (const V4l2Exception& ex) {
                    printf("Open exception: %s\n", ex.what());
                    return -1;
                }
            }

        }

        sleep(kSleepSecond);

        printf("*** Init cameras ***\n");
        for (int i = 0; i < v4l2cameraOpenSize; ++i) {
            try {
                cameras_[i]->Init();
            } catch (const V4l2Exception& ex) {
                printf("Init exception: %s\n", ex.what());
                return -1;
            }
            data_[i] =
                    cv::Mat(cameras_[i]->height(), cameras_[i]->width(), CV_8UC3);
        }

        sleep(kSleepSecond);

        printf("*** Start capturing ***\n");
        for (int i = 0; i < v4l2cameraOpenSize; ++i) {
            try {
                cameras_[i]->start_capturing();
            } catch (const V4l2Exception& ex) {
                printf("Start capturing exception: %s\n", ex.what());
                return -1;
            }
        }

        sleep(kSleepSecond);
    }


    return 0;
}


bool MultiCamerasProcess::parseGroupInfo()
{

    if(strlen(mConfigdata.camera_open)!=mConfigdata.camera_number)
    {
        printf("camera_open or camera_numer set error.\n");
        return false;
    }


    v4l2cameraOpenSize = 0;

    tcpcameraOpenSize = 0;

    //给摄像头重新标号

    cameraOpenIndex = new int[mConfigdata.camera_number];

    for(unsigned int i = 0;i<mConfigdata.camera_number;i++)
    {
        cameraOpenIndex[i] = -1;
    }

    //tcp camera init
    //需要优先处理
    if(mConfigdata.camera_open[0]=='1')
    {
        mTcpStatus = mTcp->ConnectTo(mConfigdata.default_ip,mConfigdata.default_port);
        if(mTcpStatus == -1)
        {
            printf("Tcp connect failed.\n");
            tcpcameraOpenSize = 0;
            return false;
        }
        else
        {
            tcpcameraOpenSize = 1;
            cameraOpenIndex[0] = 0;
        }

    }
    else
    {
        tcpcameraOpenSize = 0;
        printf("The tcp camera is not selected.\n");
    }



    for(unsigned int i=1;i<mConfigdata.camera_number;i++)
    {
        if(mConfigdata.camera_open[i]=='1')
        {
            cameraOpenIndex[i] = v4l2cameraOpenSize + tcpcameraOpenSize;
            v4l2cameraOpenSize++;
        }
    }


    for(unsigned int i =0;i<mConfigdata.group.size();i++)
    {

        group[i].resize(0);
        std::string group_str = mConfigdata.group[i];

        char* temp_group = new char[group_str.length()];

        strcpy(temp_group,group_str.c_str());

        //设置错误直接跳过
        if(strlen(mConfigdata.camera_open)!=strlen(temp_group))
        {
            printf("group %d set error.\n",i+1);
            continue;
        }

        for(unsigned int j =0 ; j<strlen(temp_group);j++)
        {
            if(temp_group[j] == '1')
            {
                if(cameraOpenIndex[j]!=-1)
                {
                    group[i].push_back(cameraOpenIndex[j]);

                }

            }
        }

    }


    groupsNum = 0;
    for(unsigned int i =0;i<mConfigdata.group.size();i++)
    {
        if(group[i].size()>2)
        {
            group[i].resize(0);
            printf("group: %d set error.\n",i+1);
        }
        else if(group[i].size()>0)
        {
            groupsNum++;

        }

    }


    //如果没有分组，那就默认已打开摄像头前两个为一组
    if(groupsNum==0)
    {
        for(unsigned int i =0;i<mConfigdata.camera_number;i++)
        {
            if(cameraOpenIndex[i]!=-1)
                group[0].push_back(cameraOpenIndex[i]);
            if(group[0].size()==2)
                break;
        }
        groupsNum = 1;
    }
    printf("group: %d .\n",groupsNum);
    for(unsigned int i =0;i<mConfigdata.group.size();i++)
    {
        for(unsigned int j=0;j<group[i].size();j++)
            printf("%d,",group[i][j]);
        printf("\n");
    }
    return true;
}


void MultiCamerasProcess::StartCapturing()
{
    thread_handle_=new std::thread(Update,this);
}

int MultiCamerasProcess::GetCameraBufferSize()
{
    mutex_.lock();
    int size = cameraBuffer_.size();
    mutex_.unlock();
    return size;
}

int MultiCamerasProcess::Update(MultiCamerasProcess* multiCameraProc)
{
    unsigned long long time_stamp;
    while (g_enable) {

        FrameData frameData;

        bool ret = true;
        //get timeval:tv_sec,tv_usec
        timeval tv;
        gettimeofday(&tv,NULL);

        time_stamp = tv.tv_sec*1000000 + tv.tv_usec;
        //tcp push frame data

        if(multiCameraProc->tcpcameraOpenSize)
        {
            cv::Mat frame;
            multiCameraProc->mTcp->ReadFrame(frame);
            if(!frame.data)
            {
                printf("tcp frame error.\n");
                g_enable = false;
                break;
            }
            frameData.cameras.push_back(frame);
        }

        //v4l2 push frame data
        for (int i = 0; i < multiCameraProc->v4l2cameraOpenSize; ++i) {
            ret &= multiCameraProc->cameras_[i]->Update();
            if (!ret) {
                break;
            }
            unsigned long long innerframecount_,innertimestamp_;

            memcpy(multiCameraProc->data_[i].data,
                   (unsigned char*)(multiCameraProc->cameras_[i]->GetFrameData(innerframecount_,
                                                                               innertimestamp_)),
                   multiCameraProc->cameras_[i]->height() * multiCameraProc->cameras_[i]->width() * 3);
            //push data
            frameData.cameras.push_back(multiCameraProc->data_[i]);

        }


        if(frameData.cameras.size()==
                (multiCameraProc->v4l2cameraOpenSize+multiCameraProc->tcpcameraOpenSize))
        {


            //frame data merge
            FrameData frameMergeData;

            for(unsigned int i=0;i<multiCameraProc->mConfigdata.group.size();i++)
            {

                if(multiCameraProc->group[i].size()>0)
                {
                    cv::Mat frame{frameData.cameras[multiCameraProc->group[i][0]]};
                    for (unsigned int j = 1; j < multiCameraProc->group[i].size(); j++) {
                        cv::vconcat(frame, frameData.cameras[multiCameraProc->group[i][j]], frame);
                    }
                    printf("group:%d,width:%d,height:%d\n",i+1,frame.cols,frame.rows);
                    frameMergeData.cameras.push_back(frame);
                }

            }

            gettimeofday(&tv,NULL);

            time_stamp = multiCameraProc->durationTime_ - tv.tv_sec*1000000 - tv.tv_usec + time_stamp;
            time_stamp = (time_stamp > 0 ? time_stamp:0);
            //usleep(time_stamp);

            frameMergeData.timestamp = time_stamp;
            multiCameraProc->mutex_.lock();
            multiCameraProc->cameraBuffer_.push(frameMergeData);//在这一句中将函数中的局部变量framemergedata（这个局部变量存着六个相机读进来的融合过的帧，是一个vector） push进了main函数的proc对象的一个queue成员camerabuffer。
            multiCameraProc->mutex_.unlock();

            if(multiCameraProc->cameraBuffer_.size()>multiCameraProc->fps)
            {
                multiCameraProc->bufferstatus[0] = true;//has Data
            }
            if(multiCameraProc->bufferstatus[1])//isPop
            {
                multiCameraProc->bufferstatus[0] = false;//hasData
                int index = 0;
                multiCameraProc->mutex_.lock();
                while(index < multiCameraProc->fps - 1)
                {
                    multiCameraProc->cameraBuffer_.pop();
                    index ++;
                }
                multiCameraProc->mutex_.unlock();
                multiCameraProc->bufferstatus[1] = false;//isPop
            }

            printf("frame information:%d,%ld,%d\n",
                   frameMergeData.cameras.size(),
                   time_stamp,
                   multiCameraProc->cameraBuffer_.size());
        }
    }
}
