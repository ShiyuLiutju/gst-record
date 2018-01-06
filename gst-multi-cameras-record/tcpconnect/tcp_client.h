/*
 * tcp_client.h
 *
 *  Created on: Jun 3, 2017
 *      Author: chengsq
 */

#ifndef SRC_TCP_TCP_CLIENT_H_
#define SRC_TCP_TCP_CLIENT_H_
#include <vector>
#include <opencv2/core/core.hpp>
class TcpConnection;

class TcpClient {
public:
	TcpClient();
    int ConnectTo(char* ip,int port);
	int ReadFrame(cv::Mat& frame);
	char* GetFrameData();
	virtual ~TcpClient();
	//cv::Mat frame;
	//std::vector<CAN_data> can_data;
private:
	TcpConnection* tcp_;
	int frame_length_;
	int pixel_format_;
	char* read_buffer_;
	int width_;
	int height_;
};

#endif /* SRC_TCP_TCP_CLIENT_H_ */
