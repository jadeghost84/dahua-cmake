#pragma once
#ifndef PUSH_STREAM
#define PUSH_STREAM

#include "heads.h"
#include "streamer.hpp"
using namespace std;
using namespace streamer;
using namespace cv;
using time_point = std::chrono::high_resolution_clock::time_point;
using high_resolution_clock = std::chrono::high_resolution_clock;
class PushStream
{
public:
	PushStream(string url);
	void createStream();
	void doPush(Mat mat);
	string getUrl();

private:
	string url;
	int stream_fps = 25;
    int bitrate = 500000;
	Streamer streamer;
};

#endif // !DH_CONNECTION