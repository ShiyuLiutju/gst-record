
LDFLAGS= `pkg-config --libs gstreamer-1.0`  
CFLAGS = `pkg-config --cflags gstreamer-1.0` 

main: main.cpp test_process.cpp gst_record.cpp driver/camerav4l2.cpp
	nvcc -O3 -std=c++11 -o camera_record \
	main.cpp test_process.cpp driver/camerav4l2.cpp \
	-I/usr/local/cuda/include \
	-lstdc++ -lopencv_core -lopencv_highgui \
	-lopencv_imgproc -lopencv_gpu -L/usr/local/cuda/lib64 \
	-lcuda -lcudart -lnppi -lprotobuf -lglog -lgflags `pkg-config --cflags --libs gstreamer-1.0` 

 

clean:
	rm -r main
