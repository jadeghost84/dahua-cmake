#pragma once
#ifndef DH_CONNECTION
#define DH_CONNECTION

#include "heads.h"
#include "dhnetsdk.h"
#include "dhplay.h"
#include "dhconfigsdk.h"

using namespace std;
using namespace std::chrono;
typedef void (*matCallbackfun)(cv::Mat, LONG);

struct Job
{
	int id;
}; // 收发数据结构体，可以写视频帧或者其他有用的数据

template<typename T>
class send_receive {
    std::atomic<T *> a_ptr;
    condition_variable cv_;
    mutex lock_;
public:
 
    void send(T const& _obj) {
        T *new_ptr = new T;
        *new_ptr = _obj;
        std::unique_ptr<T> old_ptr(a_ptr.exchange(new_ptr));
        cv_.notify_one();
    }
 
    T receive() {
        std::unique_ptr<T> ptr;
        do {
            std::unique_lock<mutex> l(lock_);
            cv_.wait(l, [&]{return a_ptr != NULL;});
            ptr.reset(a_ptr.exchange(NULL));
        } while (!ptr);
        T obj = *ptr;
        return obj;
    }
    send_receive() : a_ptr(NULL)
    {
        cout << "================> init send_receive" << endl;
    }
};


struct Device
{
	char *name;						 //设备名称
	char *ip;						 //设备ip地址
	int port;						 //设备端口号
	char *adminName;				 //设备用户名
	char *password;					 //设备用户密码
	int nChannelID;					 //设备通道号
	vector<string> models;			 //使用的算法模型
	vector<matCallbackfun> callFuns; //回调函数
	long long lLogin;				 //设备登录句柄
	long long lRealPlay;			 //实时播放句柄
	vector<int> playPorts;
};
struct device_run_prop{
	static send_receive<Job> send_receive_impl;
};
class DHConnection
{
public:
	DHConnection();
	DHConnection(char *&, char *&, int, char *&, char *&, int playPort, void *pUserData);
	bool addDevice(Device); //添加设备
	bool startPlay();
	cv::Mat frame_read(string name);
	void closePlay();
	// int closeConnect();
	bool initSDK(); //初始化sdk
	// static IplImage*  YUV420_To_IplImage_Opencv(char* pYUV420, int width, int height);
	// void static CALLBACK DisConnectFunc(LLONG lLoginID, char* pchDVRIP, LONG nDVRPort, LDWORD dwUser);
	// void static CALLBACK AutoReConnectFunc(LLONG lLoginID, char* pchDVRIP, LONG nDVRPort, LDWORD dwUser);
	// void static CALLBACK RealDataCallBackEx(LLONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LLONG param, LDWORD dwUser);
	// void static CALLBACK DecCBFun(LONG nPort, char* pBuf, LONG nSize, FRAME_INFO* pFrameInfo, void* pUserData, LONG nReserved2);

public:
	map<LONG, matCallbackfun> playPort2callBackFun;
	map<LONG, bool> playPort2allowHandle;
	map<LONG, device_run_prop> device_run_props;

private:
	std::vector<Device> devices;
};

#endif // !DH_CONNECTION