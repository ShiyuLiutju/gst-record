#pragma once
#ifndef APTBASEDEFINE
#define APTBASEDEFINE
#include <exception>
#include <string>


#ifndef APTEXCEPTION
#define APTEXCEPTION
class APTException : public std::exception
{
public:
    APTException(const char* msg) : _msg(msg) {}
    APTException(const std::string& msg) : _msg(msg) {}
    ~APTException() throw() {}
    const char* what() const throw() { return _msg.c_str(); }
private:
    std::string _msg;
};
#endif

#ifndef NETEXCEPTOIN
#define NETEXCEPTION
class NetException
{
public:
    NetException(const char* error,int error_code=-1)
    {
        error_code = error_code;
        _msg=error;
    }
    std::string what(){return _msg.c_str();}
private:
    std::string _msg;
    int error_code;
};
#endif

#ifndef CANEXCEPTION
#define CANEXCEPTION
class CanException :public std::exception
{
public:
    CanException(const char* msg) : _msg(msg) {}
    CanException(const std::string& msg):_msg(msg){}
    ~CanException() throw(){}
    const char* what() const throw(){return _msg.c_str();}
private:
    std::string _msg;
};
#endif

#ifndef APTCARTYPE
#define APTCARTYPE
#define CARTYPE_NISSAN_FUGA 0x0001
#define CARTYPE_MG_GS 0x0002
#define CARTYPE_MG_RX5 0x0003
#define CARTYPE_GOLF 0x0004
#define CARTYPE_MAGOTAN 0x0005
#endif

#ifndef APTBITRATE
#define APTBITRATE
#define APT_SINGLEBITRATE 10000000
#define APT_THREEBITRATE 30000000
#define APT_FOURBITRATE 40000000
#endif

#ifndef APTFPS
#define APTFPS
#define APT_FPS 15
#endif

#ifndef APTRESOLUTION
#define APTRESOLUTION
#define APT_RESOLUTION_WIDTH 1280
#define APT_RESOLUTION_HEIGHT 960
#endif

#ifndef APTRESOLUTION_TEMP
#define APTRESOLUTION_TEMP
#define APT_RESOLUTION_WIDTH_TEMP 1984
#define APT_RESOLUTION_HEIGHT_TEMP 1488
#endif

#ifndef APTVIDEOSTORAGE
#define APTVIDEOSTORAGE
#define APTVIDEOSTORAGEDIR "videostorage/"
#endif

#ifndef APTG//LOGSTORAGE
#define APTG//LOGSTORAGE
#define APTG//LOGSTORAGEDIR "//LOG/"
#endif

#ifndef APTCONFIG
#define APTCONFIG
#define APTCONFIGFILE "config/config.json"
#define APTCORRECTIONFILE "config/correctionmap.xml"
#define APTCONNECTCONFIG "config/NetworkConfig.ini"
#endif

#ifndef APTGETPICSTORAGE
#define APTGETPICSTORAGE
#define APTGETPICSTORAGEDIR "pic/"
#endif

#ifndef APTVERSION
#define APTVERSION "2017-7-11"
#endif

#ifndef APTLABELSTR
#define APTLABELFRONT "front"
#define APTLABELROUND "round"
#endif


#endif // APTBASEDEFINE
