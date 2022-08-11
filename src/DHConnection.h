#pragma once
#ifndef DH_CONNECTION
#define DH_CONNECTION

#include "dhnetsdk.h"
#include "dhplay.h"
#include "dhconfigsdk.h"
#include <iostream>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include<map>
#include <future>
#include <thread>
using namespace std;
typedef void (*matCallbackfun)(cv::Mat);

struct Device {
	char* name;//设备名称
	char* ip;//设备ip地址
	int port; //设备端口号
	char* adminName;//设备用户名
	char* password;//设备用户密码
	int nChannelID;//设备通道号
	vector<string> models;//使用的算法模型
	vector<matCallbackfun> callFuns;//回调函数
	long long lLogin;//设备登录句柄
	long long lRealPlay;//实时播放句柄
	vector<int> playPorts;
};
class DHConnection
{
public:
	DHConnection();
	DHConnection(char*&, char*&, int, char*&, char*&, int playPort, void* pUserData);
	bool addDevice(Device);//添加设备
	bool startPlay();
	void closePlay();
	//int closeConnect();
	bool initSDK();//初始化sdk
	//static IplImage*  YUV420_To_IplImage_Opencv(char* pYUV420, int width, int height);
	//void static CALLBACK DisConnectFunc(LLONG lLoginID, char* pchDVRIP, LONG nDVRPort, LDWORD dwUser);
	//void static CALLBACK AutoReConnectFunc(LLONG lLoginID, char* pchDVRIP, LONG nDVRPort, LDWORD dwUser);
	//void static CALLBACK RealDataCallBackEx(LLONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LLONG param, LDWORD dwUser);
	//void static CALLBACK DecCBFun(LONG nPort, char* pBuf, LONG nSize, FRAME_INFO* pFrameInfo, void* pUserData, LONG nReserved2);

	void connect(char*& ip, int port, char*& adminName, char*& password, int playPort, void* pUserData, int nChannelID);
	void connects();
public:
	map<LONG, matCallbackfun> playPort2callBackFun;
	map<LONG, bool> playPort2allowHandle;
private:
	std::vector<Device> devices;
	
};

#endif // !DH_CONNECTION