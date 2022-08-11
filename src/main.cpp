
#include "simple_yolo.hpp"
#include "DHConnection.h"
#include <ctime>
#include <thread>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <csignal>
#if defined(_WIN32)
#include <Windows.h>
#include <wingdi.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "gdi32.lib")
#undef min
#undef max
#else
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#endif

using namespace std;

static const char *cocolabels[] = {
    "person", "bicycle", "car", "motorcycle", "airplane",
    "bus", "train", "truck", "boat", "traffic light", "fire hydrant",
    "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse",
    "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis",
    "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
    "skateboard", "surfboard", "tennis racket", "bottle", "wine glass",
    "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich",
    "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
    "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv",
    "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave",
    "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
    "scissors", "teddy bear", "hair drier", "toothbrush"};

static std::tuple<uint8_t, uint8_t, uint8_t> hsv2bgr(float h, float s, float v)
{
    const int h_i = static_cast<int>(h * 6);
    const float f = h * 6 - h_i;
    const float p = v * (1 - s);
    const float q = v * (1 - f * s);
    const float t = v * (1 - (1 - f) * s);
    float r, g, b;
    switch (h_i)
    {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    default:
        r = 1;
        g = 1;
        b = 1;
        break;
    }
    return make_tuple(static_cast<uint8_t>(b * 255), static_cast<uint8_t>(g * 255), static_cast<uint8_t>(r * 255));
}

static std::tuple<uint8_t, uint8_t, uint8_t> random_color(int id)
{
    float h_plane = ((((unsigned int)id << 2) ^ 0x937151) % 100) / 100.0f;
    ;
    float s_plane = ((((unsigned int)id << 3) ^ 0x315793) % 100) / 100.0f;
    return hsv2bgr(h_plane, s_plane, 1);
}

static bool exists(const string &path)
{

#ifdef _WIN32
    return ::PathFileExistsA(path.c_str());
#else
    return access(path.c_str(), R_OK) == 0;
#endif
}

std::shared_ptr<SimpleYolo::Infer> engine;
bool requires_model(const string &name)
{

    auto onnx_file = cv::format("%s.onnx", name.c_str());
    if (!exists(onnx_file))
    {
        printf("Auto download %s\n", onnx_file.c_str());
        system(cv::format("wget http://zifuture.com:1556/fs/25.shared/%s", onnx_file.c_str()).c_str());
    }

    bool isexists = exists(onnx_file);
    if (!isexists)
    {
        printf("Download %s failed\n", onnx_file.c_str());
    }
    return isexists;
}

static void createEngine(SimpleYolo::Type type, SimpleYolo::Mode mode, const string &model)
{

    int deviceid = 0;
    auto mode_name = SimpleYolo::mode_string(mode);
    SimpleYolo::set_device(deviceid);

    const char *name = model.c_str();
    printf("===================== test %s %s %s ==================================\n", SimpleYolo::type_name(type), mode_name, name);

    if (!requires_model(name))
        return;

    string onnx_file = cv::format("%s.onnx", name);
    string model_file = cv::format("%s.%s.trtmodel", name, mode_name);
    int test_batch_size = 16;

    if (!exists(model_file))
    {
        SimpleYolo::compile(
            mode, type,      // FP32、FP16、INT8
            test_batch_size, // max batch size
            onnx_file,       // source
            model_file,      // save to
            1 << 30,
            "inference");
    }
    engine = SimpleYolo::create_infer(model_file, type, deviceid, 0.4f, 0.5f);
}

int sockfd;
socklen_t sockaddr_in_len = sizeof(struct sockaddr_in);
struct sockaddr_in serveraddr;
void detect(cv::Mat &mat)
{

    if (engine == nullptr)
    {
        printf("Engine is nullptr\n");
        return;
    }
    // warmup
    auto boxes = engine->commit(mat).get();
    for (auto &obj : boxes)
    {
        uint8_t b, g, r;
        tie(b, g, r) = random_color(obj.class_label);
        cv::rectangle(mat, cv::Point(obj.left, obj.top), cv::Point(obj.right, obj.bottom), cv::Scalar(b, g, r), 5);
        auto name = cocolabels[obj.class_label];
        auto caption = cv::format("%s %.2f", name, obj.confidence);
        int width = cv::getTextSize(caption, 0, 1, 2, nullptr).width + 10;
        cv::rectangle(mat, cv::Point(obj.left - 3, obj.top - 33), cv::Point(obj.left + width, obj.top), cv::Scalar(b, g, r), -1);
        cv::putText(mat, caption, cv::Point(obj.left, obj.top - 5), 0, 1, cv::Scalar::all(0), 2, 16);
    }

    std::string res_str = "ss";
    res_str = "{" + res_str + "}";
    sendto(sockfd, res_str.c_str(), strlen(res_str.c_str()), 0, (struct sockaddr *)&serveraddr, sockaddr_in_len);
}
void getClient(){
    while (1)
    {
        /*********接收来自客户端的数据*********/
        char buffer[1024];           //用于存放对方发送的数据
        struct sockaddr_in src;      //用于保存对端的地址信息
        socklen_t len = sizeof(src); //对端地址信息的大小
        recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&src, &len);
        std::cout << "client# " << buffer << std::endl;

        std::string msg = "收到来自客户端的信息：";
        msg.append(buffer);
        sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr *)&src, len);
    }
}
int initUDP(const char *_udp_ip, int udp_port)
{
    /*********** udp初始化 ***************/
    std::string udp_ip = udp_ip;
    memset(&serveraddr, 0, sockaddr_in_len);
    //填充upd 服务器自身的信息
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8081);
    serveraddr.sin_addr.s_addr = inet_addr("192.168.2.28");
    //注意此处是udp协议,用SOCK_DGRAM
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("fail to socket\n\n");
        return -1;
    }
    // int ret = bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    // if (ret < 0)
    // {
    //     //说明绑定失败
    //     std::cerr << "bind绑定失败" << errno << std::endl;
    //     return 2;
    // }
    // std::thread t1(getClient);
    // t1.join();
}
FILE *fp1 = nullptr;
void handleCallBack1(cv::Mat mat)
{
    detect(mat);
    //  基于当前系统的当前日期/时间
    time_t now = time(0);
    // 把 now 转换为字符串形式
    char *dt = ctime(&now);
    //std::cout << dt << std::endl;
    //fwrite(mat.data, sizeof(char), mat.total() * mat.elemSize(), fp1);
    imshow("hello",mat);
    cv::waitKey(1);
}
FILE *fp2 = nullptr;
void handleCallBack2(cv::Mat mat)
{
    detect(mat);
    //  基于当前系统的当前日期/时间
    time_t now = time(0);
    // 把 now 转换为字符串形式
    char *dt = ctime(&now);
    //std::cout << dt << std::endl;
    //fwrite(mat.data, sizeof(char), mat.total() * mat.elemSize(), fp2);
    imshow("hello1",mat);
    cv::waitKey(1);
}

bool is_running = true;

void OnSignal(int)
{
    is_running = false;
}
int pushStream(FILE *fp ,string rtmp_server_url)
{

    // 触发下面的信号就退出
    signal(SIGINT, OnSignal);
    signal(SIGQUIT, OnSignal);
    signal(SIGTERM, OnSignal);
    std::stringstream command;
    command << "ffmpeg ";
    // infile options
    command << "-y "               // overwrite output files
            << "-an "              // disable audio
            << "-f rawvideo "      // force format to rawvideo
            << "-vcodec rawvideo " // force video rawvideo ('copy' to copy stream)
            << "-pix_fmt bgr24 "   // set pixel format to bgr24
            << "-s 1920x1080 "     // set frame size (WxH or abbreviation)
            << "-r 25 ";           // set frame rate (Hz value, fraction or abbreviation)

    command << "-i - "; //

    // outfile options
    command << "-c:v libx264 "      // Hyper fast Audio and Video encoder
            << "-pix_fmt yuv420p "  // set pixel format to yuv420p
            << "-preset ultrafast " // set the libx264 encoding preset to ultrafast
            << "-f flv "            // force format to flv
            << "-flvflags no_duration_filesize "
            << rtmp_server_url;
    fp1 = popen(command.str().c_str(), "w");
    return EXIT_SUCCESS;
}

Device d1 = { "设备1","192.168.2.102",37777, "admin", "csis0123",0 ,{"model1","model2","model3","model4"},{handleCallBack1} };
Device d2 = { "设备2", "192.168.2.101",37777, "admin", "csis0123",0,{"model1","model2","model3"} ,{handleCallBack2} };
vector<Device> devices = { d1,d2};
int main()
{

    createEngine(SimpleYolo::Type::V7, SimpleYolo::Mode::FP32, "yolov7");
  
    //pushStream(fp1,"rtmp://192.168.2.9:1935/live/1");
    //pushStream(fp2,"rtmp://192.168.2.9:1935/live/2");
    const char *__upd_ip = "192.168.2.28";
    // int __udp_port = 8080;
    // initUDP(__upd_ip, __udp_port);
	DHConnection dHconnect = DHConnection();
	bool initSDKResult = dHconnect.initSDK();
	//添加设备
	vector<Device>::iterator it = devices.begin();
	for (; it != devices.end(); ++it)
	{
		dHconnect.addDevice((*it));
	}
	dHconnect.startPlay();
	sleep(1000000);
	dHconnect.closePlay();
    pclose(fp1);
    pclose(fp2);
    return 0;
}