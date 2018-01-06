PROJECT(gst_record)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_BUILD_TYPE "Debug")
ADD_DEFINITIONS("-O0 -Wall -g -ggdb")
FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
include_directories("/usr/include/gstreamer-1.0"
    "/usr/local/include/gstreamer-1.0"
    "/usr/include/glib-2.0"
    "/usr/lib/aarch64-linux-gnu/glib-2.0/include"
    "/usr/lib/aarch64-linux-gnu/gstreamer-1.0/include"
    "/usr/local/cuda/include")
ADD_COMPILE_OPTIONS(-std=c++11)
SET(src main.cpp driver/camerav4l2.cpp
    gst_record.cpp
    config.cpp
    anyconversion.cpp
    json.cpp
    config.json
    multicamerasprocess.cpp
    multicamerasrecord.cpp
    tcpconnect/calmcar.prototxt.pb.cpp
    tcpconnect/tcp_client.cpp
    tcpconnect/tcp_connection.cpp)
ADD_EXECUTABLE(${PROJECT_NAME} ${src})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} gstreamer-1.0 gobject-2.0 glib-2.0 pthread gstnet-1.0
    cuda cudart nppi protobuf glog gflags opencv_core opencv_highgui opencv_imgproc opencv_gpu)
