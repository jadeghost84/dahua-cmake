#include "PushStream.h"

const AVCodec *codec_h264;
AVCodecContext *codec_ctx_h264;

bool init_YUV_to_H264(int width, int height)
{
    //寻找编码器
    codec_h264 = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec_h264)
    {
        printf("Fail: avcodec_find_encoder\n");
        return false;
    }

    //编码器上下文
    codec_ctx_h264 = avcodec_alloc_context3(codec_h264);
    if (!codec_ctx_h264)
    {
        printf("Fail: avcodec_alloc_context3\n");
        return false;
    }
    codec_ctx_h264->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx_h264->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx_h264->width = width;
    codec_ctx_h264->height = height;
    codec_ctx_h264->channels = 3;
    codec_ctx_h264->time_base = {1, 25};
    codec_ctx_h264->gop_size = 5; //图像组两个关键帧（I帧）的距离
    codec_ctx_h264->max_b_frames = 0;
    // codec_ctx_h264->qcompress = 0.6;
    // codec_ctx_h264->bit_rate = 90000;
    codec_ctx_h264->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //添加PPS、SPS

    av_opt_set(codec_ctx_h264->priv_data, "preset", "ultrafast", 0); //快速编码，但会损失质量
    // av_opt_set(codec_ctx_h264->priv_data, "tune", "zerolatency", 0);  //适用于快速编码和低延迟流式传输,但是会出现绿屏
    // av_opt_set(codec_ctx_h264->priv_data, "x264opts", "crf=26:vbv-maxrate=728:vbv-bufsize=3640:keyint=25", 0);

    //打开编码器
    if (avcodec_open2(codec_ctx_h264, codec_h264, NULL) < 0)
    {
        printf("Fail: avcodec_open2\n");
        return false;
    }

    //用于接收编码好的H264
    auto pkt_h264 = av_packet_alloc();

    return true;
}

void process_frame(const cv::Mat &in, cv::Mat &out)
{
    in.copyTo(out);
}

void stream_frame(Streamer &streamer, const cv::Mat &image)
{
    streamer.stream_frame(image.data);
}

void  stream_frame(Streamer &streamer, const cv::Mat &image, int64_t frame_duration)
{
    streamer.stream_frame(image.data, frame_duration);
}

PushStream::PushStream(string url)
{
    this->url = url;
}
void PushStream::doPush(Mat mat)
{
    Size size = mat.size();
    int cap_frame_width = size.width;
    int cap_frame_height = size.height;

    cv::Mat proc_frame;
    process_frame(mat, proc_frame);
    stream_frame(streamer, proc_frame);
}
void PushStream::createStream()
{
    StreamerConfig streamer_config(1920,1080,
                                   1920,1080,
                                   stream_fps, bitrate, "main", "rtsp://localhost:8554/1");
    streamer.enable_av_debug_log();

    streamer.init(streamer_config);
}
string PushStream::getUrl()
{
    return this->url;
}
