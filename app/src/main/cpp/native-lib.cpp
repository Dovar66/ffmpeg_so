#include <jni.h>;
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <unistd.h>
#include "AVpacket_queue.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <pthread.h>
}
#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG,"DOVAR_FFMPGE------>>",FORMAT,##__VA_ARGS__);


typedef struct MPlayer {
    AVFormatContext *pFormatContext;
    int video_stream_index;
    int audio_stream_index;
    AVCodecContext *pCodecContext_video;
    AVCodecContext *pCodecContext_audio;
    AVCodec *video_codec;
    AVCodec *audio_codec;
    ANativeWindow *native_window;

    uint8_t *buffer;
    AVFrame *yuv_frame;
    AVFrame *rgba_frame;
    int video_width;
    int video_height;
    SwrContext *swrContext;
    int out_channel_nb;
    int out_sample_rate;
    enum AVSampleFormat out_sample_fmt;
    jobject audio_track;
    jmethodID audio_track_write_mid;
    uint8_t *audio_buffer;
    AVFrame *audio_frame;
    AVPacketQueue *packets[2];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int64_t start_time;
    int64_t audio_clock;
    pthread_t write_thread;
    pthread_t video_thread;
    pthread_t audio_thread;
}MPlayer;

typedef struct Decoder {
    MPlayer *player;
    int stream_index;
} Decoder;

extern "C" JNIEXPORT jstring

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_urlprotocolinfo(
        JNIEnv *env, jobject) {
    char info[40000] = {0};
    av_register_all();

    struct URLProtocol *pup = NULL;

    struct URLProtocol **p_temp = &pup;
    avio_enum_protocols((void **) p_temp, 0);

    while ((*p_temp) != NULL) {
        sprintf(info, "%sInput: %s\n", info, avio_enum_protocols((void **) p_temp, 0));
    }
    pup = NULL;
    avio_enum_protocols((void **) p_temp, 1);
    while ((*p_temp) != NULL) {
        sprintf(info, "%sInput: %s\n", info, avio_enum_protocols((void **) p_temp, 1));
    }
    return env->NewStringUTF(info);
}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_avformatinfo(
        JNIEnv *env, jobject) {
    char info[40000] = {0};

    av_register_all();

    AVInputFormat *if_temp = av_iformat_next(NULL);
    AVOutputFormat *of_temp = av_oformat_next(NULL);
    while (if_temp != NULL) {
        sprintf(info, "%sInput: %s\n", info, if_temp->name);
        if_temp = if_temp->next;
    }
    while (of_temp != NULL) {
        sprintf(info, "%sOutput: %s\n", info, of_temp->name);
        of_temp = of_temp->next;
    }
    return env->NewStringUTF(info);
}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_avcodecinfo(
        JNIEnv *env, jobject) {
    char info[40000] = {0};

    av_register_all();

    AVCodec *c_temp = av_codec_next(NULL);

    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%sdecode:", info);
        } else {
            sprintf(info, "%sencode:", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s(video):", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s(audio):", info);
                break;
            default:
                sprintf(info, "%s(other):", info);
                break;
        }
        sprintf(info, "%s[%10s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }

    return env->NewStringUTF(info);
}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_avfilterinfo(
        JNIEnv *env, jobject) {
    char info[40000] = {0};
    avfilter_register_all();

    AVFilter *f_temp = (AVFilter *) avfilter_next(NULL);
    while (f_temp != NULL) {
        sprintf(info, "%s%s\n", info, f_temp->name);
        f_temp = f_temp->next;
    }
    return env->NewStringUTF(info);
}

/*int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codecpar->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                    NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        *//* mux encoded frame *//*
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}*/


/*
extern "C" JNIEXPORT jstring

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_muxing(JNIEnv *env, jobject) {
    AVFormatContext *pFormatCtx;
    AVOutputFormat *fmt;
    AVStream *video_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket pkt;
    uint8_t *picture_buf;
    AVFrame *pFrame;
    int picture_size;
    int y_size;
    int framecnt = 0;
    //FILE *in_file = fopen("src01_480x272.yuv", "rb");	//Input raw YUV data
    FILE *in_file = fopen("../ds_480x272.yuv", "rb");   //Input raw YUV data
    int in_w = 480, in_h = 272;                              //Input data's width and height
    int framenum = 100;                                   //Frames to encode
    //const char* out_file = "src01.h264";              //Output Filepath
    //const char* out_file = "src01.ts";
    //const char* out_file = "src01.hevc";
    const char *out_file = "ds.h264";

    //1.注册FFmpeg的编解码器
    av_register_all();
    //Method1.
    pFormatCtx = avformat_alloc_context();
    //Guess Format
    fmt = av_guess_format(NULL, out_file, NULL);
    pFormatCtx->oformat = fmt;

    //Method 2.
    //avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
    //fmt = pFormatCtx->oformat;


    //Open output URL
    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        return env->NewStringUTF("-1");
    }

    video_st = avformat_new_stream(pFormatCtx, 0);
    //video_st->time_base.num = 1;
    //video_st->time_base.den = 25;

    if (video_st == NULL) {
        return env->NewStringUTF("-1");
    }
    //Param that must set
    pCodecCtx = video_st->codec;
    //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 250;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;

    //H264
    //pCodecCtx->me_range = 16;
    //pCodecCtx->max_qdiff = 4;
    //pCodecCtx->qcompress = 0.6;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;

    //Optional Param
    pCodecCtx->max_b_frames = 3;

    // Set Option
    AVDictionary *param = 0;
    //H.264
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        //av_dict_set(¶m, "profile", "main", 0);
    }
    //H.265
    if (pCodecCtx->codec_id == AV_CODEC_ID_H265) {
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zero-latency", 0);
    }

    //Show some Information
    av_dump_format(pFormatCtx, 0, out_file, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        printf("Can not find encoder! \n");
        return env->NewStringUTF("-1");
    }
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        printf("Failed to open encoder! \n");
        return env->NewStringUTF("-1");
    }


    pFrame = av_frame_alloc();
    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *) av_malloc(picture_size);
    avpicture_fill((AVPicture *) pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width,
                   pCodecCtx->height);

    //Write File Header
    avformat_write_header(pFormatCtx, NULL);

    av_new_packet(&pkt, picture_size);

    y_size = pCodecCtx->width * pCodecCtx->height;

    for (int i = 0; i < framenum; i++) {
        //Read raw YUV data
        if (fread(picture_buf, 1, y_size * 3 / 2, in_file) <= 0) {
            printf("Failed to read raw data! \n");
            return env->NewStringUTF("-1");
        } else if (feof(in_file)) {
            break;
        }
        pFrame->data[0] = picture_buf;              // Y
        pFrame->data[1] = picture_buf + y_size;      // U
        pFrame->data[2] = picture_buf + y_size * 5 / 4;  // V
        //PTS
        //pFrame->pts=i;
        pFrame->pts = i * (video_st->time_base.den) / ((video_st->time_base.num) * 25);
        int got_picture = 0;
        //Encode
        int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
        if (ret < 0) {
            printf("Failed to encode! \n");
            return env->NewStringUTF("-1");
        }
        if (got_picture == 1) {
            printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
            framecnt++;
            pkt.stream_index = video_st->index;
            ret = av_write_frame(pFormatCtx, &pkt);
            av_packet_unref(&pkt);
        }
    }
    //Flush Encoder
    int ret = flush_encoder(pFormatCtx, 0);
    if (ret < 0) {
        printf("Flushing encoder failed\n");
        return env->NewStringUTF("-1");
    }

    //Write file trailer
    av_write_trailer(pFormatCtx);

    //Clean
    if (video_st) {
        avcodec_close(video_st->codec);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    fclose(in_file);

    return env->NewStringUTF("0");
}
*/

/*int size = 1;//随便写的一个数，实际使用时需要修改

void start() {
    //1.注册FFmpeg的所有东西(硬件加速器、编解码器、封装解封装、协议处理器)
    av_register_all();
    //注册所有编解码器，在av_register_all()中有调用
    avcodec_register_all();

    //编解码器注册函数
    avcodec_register();
    //注册硬件加速器
    av_register_hwaccel();
    //注册parser
    av_register_codec_parser();
    //bitstream_filter注册函数
    av_register_bitstream_filter();
    //注册复用器的函数
    av_register_output_format();
    //注册解复用器的函数
    av_register_input_format();
    //注册协议处理器的函数
    ffurl_register_protocol();

    //内存操作的常见函数位于libavutil\mem.c中
    //释放内存
    av_free();
    av_malloc();

    *//*常见结构体的初始化和销毁*//*

//    AVFormatContext：统领全局的基本结构体。主要用于处理封装格式（FLV/MKV/RMVB等）
    //AVFormatContext的初始化
    AVFormatContext *avFormatContext = avformat_alloc_context();
    //AVFormatContext的销毁
    avformat_free_context(avFormatContext);
//    AVStream，AVCodecContext：视音频流对应的结构体，用于视音频编解码。
    AVStream *avStream = avformat_new_stream(avFormatContext, NULL);
    AVCodecContext *avCodecContext = avcodec_alloc_context3(NULL);
//    AVFrame：存储非压缩的数据（视频对应RGB/YUV像素数据，音频对应PCM采样数据）
    AVFrame *avFrame = av_frame_alloc();
//    av_frame_alloc()函数并没有为AVFrame的像素数据分配空间。因此AVFrame中的像素数据的空间需要自行分配空间，例如使用avpicture_fill()，av_image_fill_arrays()等函数
    avpicture_fill();
    av_image_fill_arrays();
    //销毁
    av_frame_free(avFrame);
//    AVPacket：存储压缩数据（视频对应H.264等码流数据，音频对应AAC/MP3等码流数据）
    AVPacket *avPacket;
    av_new_packet(avPacket, size);
    av_packet_unref(avPacket);
    //AVIOContext输入输出对应的结构体，用于输入输出（读写文件，RTMP协议等）
    avio_alloc_context();


    //用于打开FFmpeg的输入输出文件
    avio_open2();
    //查找FFmpeg的编码器
    avcodec_find_encoder();
    //查找FFmpeg的解码器
    avcodec_find_decoder();
    //初始化一个视音频编解码器的AVCodecContext
    avcodec_open2();
    //用于关闭编码器
    avcodec_close();

    //用于初始化一个用于输出的AVFormatContext结构体,内部会调用avformat_alloc_context()
    *//* ctx：函数调用成功之后创建的AVFormatContext结构体。
     oformat：指定AVFormatContext中的AVOutputFormat，用于确定输出格式。如果指定为NULL，可以设定后两个参数（format_name或者filename）由FFmpeg猜测输出格式。
     PS：使用该参数需要自己手动获取AVOutputFormat，相对于使用后两个参数来说要麻烦一些。
     format_name：指定输出格式的名称。根据格式名称，FFmpeg会推测输出格式。输出格式可以是“flv”，“mkv”等等。
     filename：指定输出文件的名称。根据文件名称，FFmpeg会推测输出格式。文件名称可以是“xx.flv”，“yy.mkv”等等。

     函数执行成功的话，其返回值大于等于0。*//*
    avformat_alloc_output_context2();
    //函数正常执行后返回值等于0,用于写视频文件头
    avformat_write_header(avFormatContext, NULL);
    //用于编码一帧视频数据
    avcodec_encode_video2();
    //函数正常执行后返回值等于0,用于写视频数据
    av_write_frame(avFormatContext,);
    //函数正常执行后返回值等于0，用于写视频文件尾
    av_write_trailer(avFormatContext);


    *//* 用于打开多媒体数据并且获得一些相关的信息
     ps：函数调用成功之后处理过的AVFormatContext结构体。
     file：打开的视音频流的URL。
     fmt：强制指定AVFormatContext中AVInputFormat的。这个参数一般情况下可以设置为NULL，这样FFmpeg可以自动检测AVInputFormat。
     dictionay：附加的一些选项，一般情况下可以设置为NULL。*//*
    avformat_open_input(avFormatContext, , NULL, NULL);

//    读取一部分视音频数据并且获得一些相关的信息
    avformat_find_stream_info(avFormatContext, NULL);


    avcodec_send_packet();
    avcodec_receive_frame();
}*/

/*//视频解码
extern "C" JNIEXPORT void

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_startPlay(JNIEnv *env, jobject,
                                                 jstring videoPath, jobject surface) {
    if (surface == NULL) {
        LOGD("surface==null\n");
        return;
    }
    const char *input = env->GetStringUTFChars(videoPath, NULL);
    if (input == NULL) {
        LOGD("字符串转换失败......\n");
        return;
    }
//    const char *output = "test.yuv";
    AVFormatContext *avFormatContext;
    AVCodecContext *avCodecContext;
    AVCodec *avCodec;
    //注册
    av_register_all();
//    avformat_network_init();
    //为码流结构体分配内存
    avFormatContext = avformat_alloc_context();
    //打开流地址
    if (avformat_open_input(&avFormatContext, input, NULL, NULL) != 0) {
        LOGD("Couldn't open input stream.\n");
        return;
    }
    //查找流信息
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGD("Couldn't find stream information.\n");
        return;
    }
    //查找视频流，一个多媒体文件中可能含有音频流、视频流、字幕流等
    int videoIndex = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }
    if (videoIndex == -1) {
        LOGD("Didn't find a video stream.\n");
        return;
    }

    //通过视频流中编解码器参数中的——codec_id 获取对应（视频）流解码器
    AVCodecParameters *codecParameters = avFormatContext->streams[videoIndex]->codecpar;
    avCodec = avcodec_find_decoder(codecParameters->codec_id);
    if (!avCodec) {
        LOGD("Codec not found.\n");
        return;
    }
    *//*  AVCodecParserContext *parserContext = av_parser_init(codecParameters->codec_id);
      if (!parserContext) {
          LOGD("Parser not found\n");
          return;
      }*//*
    //分配解码器上下文
    avCodecContext = avFormatContext->streams[videoIndex]->codec;;
    if (!avCodecContext) {
        LOGD("分配 解码器上下文失败\n");
        return;
    }
    //初始化解码器上下文
    if (avcodec_open2(avCodecContext, avCodec, NULL) != 0) {
        LOGD("Could not open codec.\n");
        return;
    }
//    av_dump_format(avFormatContext, 0, input, 0);
    //输出视频信息
    LOGD("视频的文件格式：%s", avFormatContext->iformat->name);
    LOGD("视频时长：%d", static_cast<int>((avFormatContext->duration) / 1000000));
    LOGD("视频的宽高：%d,%d", avCodecContext->width, avCodecContext->height);
    LOGD("解码器的名称：%s", avCodec->name);

    //内存分配
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    AVPacket *avPacket = av_packet_alloc();
//    av_init_packet()
    //AVFrame用于存储解码后的像素数据(YUV)
    AVFrame *avFrame = av_frame_alloc();

    *//*** 转码相关BEGIN ***//*
    //用于存储转码后的像素数据
    AVFrame *avFrameYUV = av_frame_alloc();
    LOGD("内存分配完成")
    AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_malloc((size_t) (av_image_get_buffer_size(dstFormat,
                                                                                   avCodecContext->width,
                                                                                   avCodecContext->height,
                                                                                   1)));
    LOGD("缓冲区分配完成")
    //初始化缓冲区
    av_image_fill_arrays(avFrameYUV->data, avFrameYUV->linesize,
                         out_buffer, dstFormat,
                         avCodecContext->width, avCodecContext->height, 1);
    LOGD("缓冲区初始化完成")

    //创建转码（缩放）上下文
    struct SwsContext *sws_ctx = sws_getContext(avCodecContext->width,
                                                avCodecContext->height,//源图的宽高
                                                avCodecContext->pix_fmt,//源图像的格式
                                                avCodecContext->width,
                                                avCodecContext->height,//目标图像的宽高
                                                dstFormat,//目标图像格式
                                                SWS_BICUBIC,//指定用于转码的算法
                                                NULL, NULL,
                                                NULL);
    if (sws_ctx == NULL) {
        LOGD("sws_ctx==null\n");
        return;
    }
    *//*** 转码相关END ***//*

//    FILE *fp_yuv = fopen(input, "rb");
    LOGD("开始准备原生绘制工具")
    //android原生绘制工具
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    //定义绘图缓冲区
    ANativeWindow_Buffer outBuffer_na;
    //通过设置宽高限制缓冲区中的像素数量，而非屏幕的物流显示尺寸。
    //如果缓冲区与物理屏幕的显示尺寸不相符，则实际显示可能会是拉伸，或者被压缩的图像
    ANativeWindow_setBuffersGeometry(nativeWindow, avCodecContext->width, avCodecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    LOGD("原生绘制工具已准备")

    //一帧一帧的读取压缩数据
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
//        AVPacket orig_pkt = avPacket;
        //只要视频压缩数据（根据流的索引位置判断）
        if ((avPacket)->stream_index == videoIndex) {
            LOGD("读取到视频流数据")
            //发送压缩数据
            int rec = avcodec_send_packet(avCodecContext, avPacket);
            if (rec != 0) {
                if (rec == AVERROR(EAGAIN)) {
                    LOGD("%s", "解码错误:AVERROR(EAGAIN)");
                } else if (rec == AVERROR_EOF) {
                    LOGD("%s", "解码错误:AVERROR_EOF");
                } else if (rec == AVERROR(EINVAL)) {
                    LOGD("%s", "解码错误:AVERROR(EINVAL)");
                } else if (rec == AVERROR(ENOMEM)) {
                    LOGD("%s", "解码错误:AVERROR(ENOMEM)");
                } else {
                    LOGD("%s", "解码错误:legitimate decoding errors");
                }
                continue;
            }
            LOGD("成功发送视频流数据")
            //读取到一帧音频或者视频
            while (avcodec_receive_frame(avCodecContext, avFrame) == 0) {
                LOGD("成功接收到视频帧")
                //锁定窗口绘图界面
                ANativeWindow_lock(nativeWindow, &outBuffer_na, NULL);
                LOGD("开始画面转换")
                //AVFrame转为像素格式YUV420，宽高
                //2 6输入、输出数据
                //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
                //4 输入数据第一列要转码的位置 从0开始
                //5 输入画面的高度
                //执行转码
                *//* sws_scale(sws_ctx, (const uint8_t *const *) (avFrame->data),//源数据
                           avFrame->linesize,
                           0,
                           avCodecContext->height,//源切片的高度
                           avFrameYUV->data,//转码后数据
                           avFrameYUV->linesize);
                 *//**//* int y_size = avCodecContext->width * avCodecContext->height;
                 fwrite(avFrameYUV->data[0], 1, y_size, fp_yuv);//Y
                 fwrite(avFrameYUV->data[1], 1, y_size / 4, fp_yuv);//U
                 fwrite(avFrameYUV->data[2], 1, y_size / 4, fp_yuv);//V*//**//*
                LOGD("转换完成，开始解析转换后数据")
                uint8_t *dst = (uint8_t *) outBuffer_na.bits;
                //解码后的像素数据首地址
                //这里由于使用的是RGBA格式，所以解码图像数据只保存在data[0]中。但如果是YUV就会有data[0]
                //data[1],data[2]
                uint8_t *src = avFrameYUV->data[0];
                //获取一行字节数
                int oneLineByte = outBuffer_na.stride * 4;
                //复制一行内存的实际数量
                int srcStride = avFrameYUV->linesize[0];
                for (int i = 0; i < avCodecContext->height; i++) {
                    memcpy(dst + i * oneLineByte, src + i * srcStride, srcStride);
                }*//*

                //解锁窗口
                ANativeWindow_unlockAndPost(nativeWindow);
                //进行短暂休眠。如果休眠时间太长会导致播放的每帧画面有延迟感，如果短会有加速播放的感觉。
                //一般一每秒60帧——16毫秒一帧的时间进行休眠
                usleep(1000 * 20);
            }
        }

        //重置packet
        av_packet_unref(avPacket);
        av_frame_unref(avFrame);
//        av_frame_unref();
    }
    LOGD("开始释放资源");
//    sws_freeContext(sws_ctx);
    ANativeWindow_release(nativeWindow);
//    fclose(fp_yuv);
//    av_parser_close(parserContext);
    av_packet_free(&avPacket);
    av_frame_free(&avFrame);
//    av_frame_free(&avFrameYUV);
    avcodec_close(avCodecContext);
    avformat_close_input(&avFormatContext);
    avformat_free_context(avFormatContext);
}

#define INBUF_SIZE 4096

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename) {
    FILE *f;
    int i;
    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   const char *filename) {
    char buf[1024];
    int ret;
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }
    LOGD("发送数据成功")
    while ((ret = avcodec_receive_frame(dec_ctx, frame)) >= 0) {
        LOGD("开始读取数据:%d", ret)
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            continue;
        else if (ret < 0) {
            LOGD("Error during decoding\n");
            continue;
        }
        LOGD("读取数据成功")
        printf("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);
        *//* the picture is allocated by the decoder. no need to
           free it *//*
        snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
        pgm_save(frame->data[0], frame->linesize[0],
                 frame->width, frame->height, buf);
    }
}

extern "C" JNIEXPORT void

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_startPlayPure(JNIEnv *env, jobject,
                                                     jstring videoPath, jobject surface) {
    if (surface == NULL) {
        LOGD("surface==null\n");
        return;
    }
    const char *input = env->GetStringUTFChars(videoPath, NULL);
    if (input == NULL) {
        LOGD("字符串转换失败......\n");
        return;
    }
    const char *outfilename = "/storage/emulated/0/Movies/vcamera/test.yuv";
    AVFormatContext *avFormatContext;
    AVCodecContext *avCodecContext;
    AVCodec *avCodec;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    //注册
    av_register_all();
    *//* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) *//*
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    //分配码流结构体
    avFormatContext = avformat_alloc_context();
    //打开流地址
    if (avformat_open_input(&avFormatContext, input, NULL, NULL) != 0) {
        LOGD("Couldn't open input stream.111\n");
        return;
    }
    //查找流信息
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGD("Couldn't find stream information.\n");
        return;
    }
    //查找视频流，一个多媒体文件中可能含有音频流、视频流、字幕流等
    int videoIndex = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }
    if (videoIndex == -1) {
        LOGD("Didn't find a video stream.\n");
        return;
    }

    //通过视频流中编解码器参数中的——codec_id 获取对应（视频）流解码器
    AVCodecParameters *codecParameters = avFormatContext->streams[videoIndex]->codecpar;
    avCodec = avcodec_find_decoder(codecParameters->codec_id);
    if (!avCodec) {
        LOGD("Codec not found.\n");
        return;
    }
    AVCodecParserContext *parserContext = av_parser_init(codecParameters->codec_id);
    if (!parserContext) {
        LOGD("Parser not found\n");
        return;
    }
    //分配解码器上下文
    avCodecContext = avcodec_alloc_context3(avCodec);
    if (!avCodecContext) {
        LOGD("分配 解码器上下文失败\n");
        return;
    }
    //初始化解码器上下文
    if (avcodec_open2(avCodecContext, avCodec, NULL) != 0) {
        LOGD("Could not open codec.\n");
        return;
    }
    //输出视频信息
    LOGD("视频的文件格式：%s", avFormatContext->iformat->name);
    LOGD("视频时长：%d", static_cast<int>((avFormatContext->duration) / 1000000));
    LOGD("视频的宽高：%d,%d", avCodecContext->width, avCodecContext->height);
    LOGD("解码器的名称：%s", avCodec->name);

    //内存分配
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    AVPacket *avPacket = av_packet_alloc();
    //AVFrame用于存储解码后的像素数据(YUV)
    AVFrame *avFrame = av_frame_alloc();

    FILE *f = fopen(input, "rb");
    if (!f) {
        LOGD("Couldn't open input stream.\n");
        return;
    }
    uint8_t *data;
    size_t data_size;
    while (!feof(f)) {
        *//* read raw data from the input file *//*
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (!data_size)
            break;
        *//* use the parser to split the data into frames *//*
        data = inbuf;
        while (data_size > 0) {
            int ret = av_parser_parse2(parserContext, avCodecContext, &avPacket->data,
                                       &avPacket->size,
                                       data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data += ret;
            data_size -= ret;
            if (avPacket->size)
                decode(avCodecContext, avFrame, avPacket, outfilename);
        }
    }

    *//* flush the decoder *//*
    decode(avCodecContext, avFrame, NULL, outfilename);

    fclose(f);
    av_parser_close(parserContext);
    avcodec_free_context(&avCodecContext);
    av_frame_free(&avFrame);
    av_packet_free(&avPacket);
//    avformat_close_input()
}*/





//音频解码
extern "C" JNIEXPORT void

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_decodeAudio(JNIEnv *env, jobject obj,
                                                   jstring audioPath) {
    const char *file_name = (*env).GetStringUTFChars(audioPath, JNI_FALSE);
    //1.注册组件
    av_register_all();
    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入音频文件
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {
        LOGD("%s", "打开输入音频文件失败");
        return;
    }
    //3.获取音频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGD("%s", "获取音频信息失败");
        return;
    }

    //音频解码，需要找到对应的AVStream所在的pFormatCtx->streams的索引位置
    int audio_stream_idx = -1;
    int i = 0;
    for (; i < pFormatCtx->nb_streams; i++) {
        //根据类型判断是否是音频流
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    //4.获取解码器
    AVCodec *pCodec = avcodec_find_decoder(
            pFormatCtx->streams[audio_stream_idx]->codecpar->codec_id);
    //根据索引拿到对应的流,根据流拿到解码器上下文
    AVCodecContext *pCodeCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodeCtx, pFormatCtx->streams[audio_stream_idx]->codecpar);
    if (pCodec == NULL) {
        LOGD("%s", "无法解码");
        return;
    }
    //5.打开解码器
    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        LOGD("%s", "编码器无法打开");
        return;
    }

    //输出视频信息
    LOGD("音频的文件格式：%s", pFormatCtx->iformat->name);
    LOGD("音频时长：%d", static_cast<int>((pFormatCtx->duration) / 1000000));
    LOGD("音频的宽高：%d,%d", pCodeCtx->width, pCodeCtx->height);
    LOGD("解码器的名称：%s", pCodec->name);

    //编码数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();

    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrCtx = swr_alloc();
    //重采样设置选项-----------------------------------------------------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = pCodeCtx->sample_fmt;
    //输出的采样格式 16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入的采样率
    int in_sample_rate = pCodeCtx->sample_rate;
    //输出的采样率
    int out_sample_rate = 44100;
    //输入的声道布局
    uint64_t in_ch_layout = pCodeCtx->channel_layout;
    //输出的声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_MONO;

    swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt,
                       in_sample_rate, 0, NULL);
    swr_init(swrCtx);
    //重采样设置选项-----------------------------------------------------------end
    //获取输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    //存储pcm数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(2 * 44100);

    jclass player_class = (env)->GetObjectClass(obj);
    jmethodID audio_track_method = (*env).GetMethodID(player_class, "createAudioTrack",
                                                      "(II)Landroid/media/AudioTrack;");
    if (!audio_track_method) {
        LOGD("audio_track_method not found...")
    }
    jobject audio_track = (env)->CallObjectMethod(obj, audio_track_method, out_sample_rate,
                                                  out_channel_nb);
    //调用play方法
    jclass audio_track_class = (*env).GetObjectClass(audio_track);
    jmethodID audio_track_play_mid = (*env).GetMethodID(audio_track_class, "play", "()V");
    (*env).CallVoidMethod(audio_track, audio_track_play_mid);

    //获取write()方法
    jmethodID audio_track_write_mid = (*env).GetMethodID(audio_track_class, "write", "([BII)I");

    int framecount = 0;
    //6.一帧一帧读取压缩的音频数据AVPacket
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            //解码AVPacket->AVFrame
            //发送压缩数据
            if (avcodec_send_packet(pCodeCtx, packet) != 0) {
                LOGD("%s", "解码错误");
                continue;
            }

            //读取到一帧音频或者视频
            while (avcodec_receive_frame(pCodeCtx, frame) == 0) {
                LOGD("解码%d帧", framecount++);
                swr_convert(swrCtx, &out_buffer, 2 * 44100,
                            (const uint8_t **) (frame->data), frame->nb_samples);
                //获取sample的size
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                 frame->nb_samples,
                                                                 out_sample_fmt, 1);

                jbyteArray audio_sample_array = (*env).NewByteArray(out_buffer_size);
                jbyte *sample_byte_array = (*env).GetByteArrayElements(audio_sample_array, NULL);
                //拷贝缓冲数据
                memcpy(sample_byte_array, out_buffer, (size_t) out_buffer_size);
                //释放数组
                (*env).ReleaseByteArrayElements(audio_sample_array, sample_byte_array, 0);
                //调用AudioTrack的write方法进行播放
                (*env).CallIntMethod(audio_track, audio_track_write_mid,
                                     audio_sample_array, 0, out_buffer_size);
                //释放局部引用
                (*env).DeleteLocalRef(audio_sample_array);
                usleep(1000 * 16);
            }
        }
        av_packet_unref(packet);
    }
    av_frame_free(&frame);
    av_free(out_buffer);
    swr_free(&swrCtx);
    avcodec_close(pCodeCtx);
    avformat_close_input(&pFormatCtx);
}

//视频解码
extern "C" JNIEXPORT void

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_decodeVideo(JNIEnv *env, jobject,
                                                   jstring videoPath, jobject surface) {
    const char *file_name = (*env).GetStringUTFChars(videoPath, JNI_FALSE);

    //注册
    av_register_all();
    //如果是网络流，则需要初始化网络相关
    avformat_network_init();

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开视频文件
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {
        LOGD("Could not open file:%s\n", file_name);
        return;
    }

    //检索流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGD("Could not find stream information.\n");
        return;
    }

    //查找视频流，一个多媒体文件中可能含有音频流、视频流、字幕流等
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        LOGD("Didn't find a video stream.\n");
        return;
    }

    //获取解码器
    AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
    if (pCodec == NULL) {
        LOGD("Codec not found.\n");
        return;
    }
    //初始化解码器上下文
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGD("Could not open codec.\n");
    }

    //输出视频信息
    LOGD("视频的文件格式：%s", pFormatCtx->iformat->name);
    LOGD("视频时长：%d", static_cast<int>((pFormatCtx->duration) / 1000000));
    LOGD("视频的宽高：%d,%d", pCodecCtx->width, pCodecCtx->height);
    LOGD("解码器的名称：%s", pCodec->name);

    LOGD("开始准备原生绘制工具")
    //获取NativeWindow，用于渲染视频
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_setBuffersGeometry(nativeWindow, pCodecCtx->width, pCodecCtx->height,
                                     WINDOW_FORMAT_RGBA_8888);
    //定义绘图缓冲区
    ANativeWindow_Buffer windowBuffer;
    LOGD("原生绘制工具准备完成")

    /*** 转码相关BEGIN ***/
    AVFrame *pFrameOut = av_frame_alloc();
    if (pFrameOut == NULL) {
        LOGD("Could not allocate video frame.\n");
        return;
    }
    int num = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1);
    uint8_t *buffer = (uint8_t *) (av_malloc(num * sizeof(uint8_t)));
    av_image_fill_arrays(pFrameOut->data, pFrameOut->linesize, buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                                pCodecCtx->pix_fmt, pCodecCtx->width,
                                                pCodecCtx->height, AV_PIX_FMT_RGBA, SWS_BILINEAR,
                                                NULL, NULL, NULL);
    if (sws_ctx == NULL) {
        LOGD("sws_ctx==null\n");
        return;
    }
    /*** 转码相关END ***/

    AVFrame *pFrame = av_frame_alloc();
    AVPacket packet;
    //读取帧数据
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == videoStream) {
            //解码AVPacket->AVFrame
            //发送读取到的压缩数据（每次发送可能包含一帧或多帧数据）
            if (avcodec_send_packet(pCodecCtx, &packet) != 0) {
                continue;
            }

            //读取到一帧视频
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                //锁定窗口绘图界面
                ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

                //执行转码
                sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data, pFrame->linesize, 0,
                          pCodecCtx->height, pFrameOut->data, pFrameOut->linesize);

//                LOGD("转码完成，开始渲染数据.\n")
                //获取stride
                uint8_t *dst = (uint8_t *) windowBuffer.bits;
                int dstStride = windowBuffer.stride * 4;
                uint8_t *src = pFrameOut->data[0];
                int srcStride = pFrameOut->linesize[0];
                //由于窗口的stride和帧的stride不同，因此需要逐行复制
                int h;
                for (h = 0; h < pCodecCtx->height; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, (size_t) (srcStride));
                }

                //解锁窗口
                ANativeWindow_unlockAndPost(nativeWindow);
                //进行短暂休眠。如果休眠时间太长会导致播放的每帧画面有延迟感，如果短会有加速播放的感觉。
                //一般一每秒60帧——16毫秒一帧的时间进行休眠
                usleep(1000 * 60);
            }
        }

        //重置packet
        av_packet_unref(&packet);
    }

    //回收资源
    //释放图像帧
    av_frame_free(&pFrame);
    av_frame_free(&pFrameOut);
    av_free(buffer);
    //关闭转码上下文
    sws_freeContext(sws_ctx);
    //关闭解码器
    avcodec_close(pCodecCtx);
    //关闭视频文件
    avformat_close_input(&pFormatCtx);
    //注销网络相关
    avformat_network_deinit();

    avformat_free_context(pFormatCtx);
}

int findVideoStream(AVFormatContext *pFormatCtx) {
    //查找视频流，一个多媒体文件中可能含有音频流、视频流、字幕流等
    int videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        LOGD("Didn't find a video stream.\n");
    }
    return videoStream;
}

int findAudioStream(AVFormatContext *pFormatCtx) {
    //音频解码，需要找到对应的AVStream所在的pFormatCtx->streams的索引位置
    int audio_stream_idx = -1;
    int i = 0;
    for (; i < pFormatCtx->nb_streams; i++) {
        //根据类型判断是否是音频流
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    if (audio_stream_idx == -1) {
        LOGD("Didn't find a audio stream.\n");
    }
    return audio_stream_idx;
}

AVCodecContext *openAVCodecContext(AVFormatContext *pFormatCtx, int stream_index) {
    //4.获取解码器
    AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[stream_index]->codecpar->codec_id);
    if (pCodec == NULL) {
        LOGD("%s", "无法解码");
        return NULL;
    }
    //根据索引拿到对应的流,根据流拿到解码器上下文
    AVCodecContext *pCodeCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodeCtx, pFormatCtx->streams[stream_index]->codecpar);
    //5.打开解码器
    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        LOGD("%s", "解码器无法打开");
        return NULL;
    }

    //输出视频信息
    LOGD("文件格式：%s", pFormatCtx->iformat->name);
    LOGD("时长：%d", static_cast<int>((pFormatCtx->duration) / 1000000));
    LOGD("宽高信息：%d,%d", pCodeCtx->width, pCodeCtx->height);
    LOGD("解码器的名称：%s", pCodec->name);
    return pCodeCtx;
}

double video_clock;
AVStream *video;
double audio_clock;
AVStream *audio;

double synchronize(AVFrame *srcFrame, double pts) {
    double frame_delay;

    if (pts != 0)
        video_clock = pts; // Get pts,then set video clock to it
    else
        pts = video_clock; // Don't get pts,set it to video clock

    frame_delay = av_q2d(video->time_base);
    frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

    video_clock += frame_delay;

    return pts;
}

ANativeWindow *nativeWindow;
AVFrame *pFrameOut;
struct SwsContext *sws_ctx;

//刷新画面
void frame_refresh(AVCodecContext *pCodecCtx_video, AVFrame *pFrame) {
    //定义绘图缓冲区
    ANativeWindow_Buffer windowBuffer;
    //锁定窗口绘图界面
    ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

    //执行转码
    sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data, pFrame->linesize, 0,
              pCodecCtx_video->height, pFrameOut->data, pFrameOut->linesize);

//                LOGD("转码完成，开始渲染数据.\n")
    //获取stride
    uint8_t *dst = (uint8_t *) windowBuffer.bits;
    int dstStride = windowBuffer.stride * 4;
    uint8_t *src = pFrameOut->data[0];
    int srcStride = pFrameOut->linesize[0];
    //由于窗口的stride和帧的stride不同，因此需要逐行复制
    int h;
    for (h = 0; h < pCodecCtx_video->height; h++) {
        memcpy(dst + h * dstStride, src + h * srcStride, (size_t) (srcStride));
    }

    //解锁窗口
    ANativeWindow_unlockAndPost(nativeWindow);
    //进行短暂休眠。如果休眠时间太长会导致播放的每帧画面有延迟感，如果短会有加速播放的感觉。
    //一般一每秒60帧——16毫秒一帧的时间进行休眠
//    usleep(1000 * 60);
}

int videoStream;
int audioStream;

//全局变量
JavaVM *g_jvm = NULL;
jobject obj_out;
pthread_t p_audio;
AVCodecContext *pCodecCtx_audio;
AVFormatContext *pFormatCtx;
const char *file_name;


void *playAudio(void *arg) {
    JNIEnv *env_out;
    //JNIEnv*env 是一个线程对应一个env，线程间不可以共享同一个env变量
    //Attach主线程，通过JavaVM给env_out赋值
    if ((g_jvm)->AttachCurrentThread(&env_out, NULL) != JNI_OK) {
        LOGD("%s: AttachCurrentThread() failed", __FUNCTION__);
        return NULL;
    }
    if (pFormatCtx == NULL) {
        LOGD("pFormatCtx==NULL 1")
        return NULL;
    }
    //打开视频文件
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {
        LOGD("Could not open file:%s\n", file_name);
        return NULL;
    }

    //检索流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGD("Could not find stream information.\n");
        return NULL;
    }

    videoStream = findVideoStream(pFormatCtx);
    audioStream = findAudioStream(pFormatCtx);

    if (audioStream != -1) {
        //获取音频解码器上下文
        pCodecCtx_audio = openAVCodecContext(pFormatCtx, audioStream);
        audio = pFormatCtx->streams[audioStream];
        if (pFormatCtx == NULL) {
            LOGD("pFormatCtx==NULL 2")
        }

        //编码数据
        AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
        //解压缩数据
        AVFrame *frame = av_frame_alloc();
        //frame->16bit 44100 PCM 统一音频采样格式与采样率
        SwrContext *swrCtx = swr_alloc();
        //重采样设置选项-----------------------------------------------------------start
        //输入的采样格式
        enum AVSampleFormat in_sample_fmt = pCodecCtx_audio->sample_fmt;
        //输出的采样格式 16bit PCM
        enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
        //输入的采样率
        int in_sample_rate = pCodecCtx_audio->sample_rate;
        //输出的采样率
        int out_sample_rate = 44100;
        //输入的声道布局
        uint64_t in_ch_layout = pCodecCtx_audio->channel_layout;
        //输出的声道布局
        uint64_t out_ch_layout = AV_CH_LAYOUT_MONO;

        swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                           in_sample_fmt,
                           in_sample_rate, 0, NULL);
        swr_init(swrCtx);
        //重采样设置选项-----------------------------------------------------------end
        //获取输出的声道个数
        int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
        //存储pcm数据
        uint8_t *out_buffer = (uint8_t *) av_malloc(2 * 44100);

        jclass player_class = (env_out)->GetObjectClass(obj_out);
        jmethodID audio_track_method = (*env_out).GetMethodID(player_class, "createAudioTrack",
                                                              "(II)Landroid/media/AudioTrack;");
        if (!audio_track_method) {
            LOGD("audio_track_method not found...")
        }
        jobject audio_track = (env_out)->CallObjectMethod(obj_out, audio_track_method,
                                                          out_sample_rate,
                                                          out_channel_nb);
        //调用play方法
        jclass audio_track_class = (*env_out).GetObjectClass(audio_track);
        jmethodID audio_track_play_mid = (*env_out).GetMethodID(audio_track_class, "play", "()V");
        (*env_out).CallVoidMethod(audio_track, audio_track_play_mid);

        //获取write()方法
        jmethodID audio_track_write_mid = (*env_out).GetMethodID(audio_track_class, "write",
                                                                 "([BII)I");

        int framecount = 0;
        if (pFormatCtx == NULL) {
            LOGD("pFormatCtx==NULL")
            (g_jvm)->DetachCurrentThread();
            return NULL;
        }
        //6.一帧一帧读取压缩的音频数据AVPacket
        while (av_read_frame(pFormatCtx, packet) >= 0) {
            if (packet->stream_index == audioStream) {
                //解码AVPacket->AVFrame
                //发送压缩数据
                if (avcodec_send_packet(pCodecCtx_audio, packet) != 0) {
                    LOGD("%s", "解码错误");
                    continue;
                }

                //读取到一帧音频或者视频
                while (avcodec_receive_frame(pCodecCtx_audio, frame) == 0) {
                    audio_clock = av_q2d(audio->time_base) * frame->pts;
                    LOGD("解码%d帧", framecount++);
                    swr_convert(swrCtx, &out_buffer, 2 * 44100,
                                (const uint8_t **) (frame->data), frame->nb_samples);
                    //获取sample的size
                    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                     frame->nb_samples,
                                                                     out_sample_fmt, 1);

                    jbyteArray audio_sample_array = (*env_out).NewByteArray(out_buffer_size);
                    jbyte *sample_byte_array = (*env_out).GetByteArrayElements(audio_sample_array,
                                                                               NULL);
                    //拷贝缓冲数据
                    memcpy(sample_byte_array, out_buffer, (size_t) out_buffer_size);
                    //释放数组
                    (*env_out).ReleaseByteArrayElements(audio_sample_array, sample_byte_array, 0);
                    //调用AudioTrack的write方法进行播放
                    (*env_out).CallIntMethod(audio_track, audio_track_write_mid,
                                             audio_sample_array, 0, out_buffer_size);
                    //释放局部引用
                    (*env_out).DeleteLocalRef(audio_sample_array);
                    usleep(1000 * 16);
                }
            }
            av_packet_unref(packet);
        }

        av_frame_free(&frame);
        av_free(out_buffer);
        swr_free(&swrCtx);
        //关闭解码器
        avcodec_close(pCodecCtx_audio);
    }

    (g_jvm)->DetachCurrentThread();

//    pthread_exit(0);
    pthread_exit(&p_audio);//与上面这行区别在哪里
}



//音视频同步
/*
extern "C" JNIEXPORT void

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_play(JNIEnv *env, jobject obj,
                                            jstring videoPath, jobject surface) {
    obj_out = (env)->NewGlobalRef(obj);
    env->GetJavaVM(&g_jvm);//获取JavaVM
    file_name = (*env).GetStringUTFChars(videoPath, JNI_FALSE);
    //注册
    av_register_all();
    //如果是网络流，则需要初始化网络相关
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    pthread_create(&p_audio, NULL, playAudio, pFormatCtx);


    */
/* if (videoStream != -1) {
         video = pFormatCtx->streams[videoStream];

         //获取视频解码器上下文
         AVCodecContext *pCodecCtx_video = openAVCodecContext(pFormatCtx, videoStream);
         LOGD("开始准备原生绘制工具")
         //获取NativeWindow，用于渲染视频
         nativeWindow = ANativeWindow_fromSurface(env, surface);
         ANativeWindow_setBuffersGeometry(nativeWindow, pCodecCtx_video->width,
                                          pCodecCtx_video->height,
                                          WINDOW_FORMAT_RGBA_8888);

         LOGD("原生绘制工具准备完成")

         *//*
*/
/*** 转码相关BEGIN ***//*
*/
/*
        pFrameOut = av_frame_alloc();
        if (pFrameOut == NULL) {
            LOGD("Could not allocate video frame.\n");
            return;
        }
        int num = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx_video->width,
                                           pCodecCtx_video->height, 1);
        uint8_t *buffer = (uint8_t *) (av_malloc(num * sizeof(uint8_t)));
        av_image_fill_arrays(pFrameOut->data, pFrameOut->linesize, buffer, AV_PIX_FMT_RGBA,
                             pCodecCtx_video->width, pCodecCtx_video->height, 1);
        sws_ctx = sws_getContext(pCodecCtx_video->width, pCodecCtx_video->height,
                                 pCodecCtx_video->pix_fmt,
                                 pCodecCtx_video->width,
                                 pCodecCtx_video->height, AV_PIX_FMT_RGBA,
                                 SWS_BILINEAR,
                                 NULL, NULL, NULL);
        if (sws_ctx == NULL) {
            LOGD("sws_ctx==null\n");
            return;
        }
        *//*
*/
/*** 转码相关END ***//*
*/
/*

        AVFrame *pFrame = av_frame_alloc();
        AVPacket packet_video;
        //读取帧数据
        while (av_read_frame(pFormatCtx, &packet_video) >= 0) {
            if (packet_video.stream_index == videoStream) {
                //解码AVPacket->AVFrame
                //发送读取到的压缩数据（每次发送可能包含一帧或多帧数据）
                if (avcodec_send_packet(pCodecCtx_video, &packet_video) != 0) {
                    continue;
                }

                double last_duration;
                double last_delay;
                //读取到一帧视频
                while (avcodec_receive_frame(pCodecCtx_video, pFrame) == 0) {
                    int64_t pts = av_frame_get_best_effort_timestamp(pFrame);
                    double duration = av_q2d(video->time_base) * pts;
                    duration = synchronize(pFrame, duration);
                    pFrame->opaque = &duration;

                    double delay = duration - last_duration;
                    if (delay <= 0) {
                        delay = last_delay;
                    }
                    last_duration = duration;
                    last_delay = delay;

                    double diff =
                            duration - audio_clock;// diff < 0 => video slow,diff > 0 => video quick
                    double threshold = (delay > 0.02) ? delay : 0.02;
                    if (fabs(diff) < 0.02) {

                    }

                    frame_refresh(pCodecCtx_video, pFrame);
                }
            }

            //重置packet
            av_packet_unref(&packet_video);
        }

        //回收资源
        //释放图像帧
        av_frame_free(&pFrame);
        av_frame_free(&pFrameOut);
        av_free(buffer);
        //关闭转码上下文
        sws_freeContext(sws_ctx);
        //关闭解码器
        avcodec_close(pCodecCtx_video);
    }*//*


    //关闭视频文件
    avformat_close_input(&pFormatCtx);
    //注销网络相关
    avformat_network_deinit();

    avformat_free_context(pFormatCtx);
}
*/

#define MAX_AUDIO_FRAME_SIZE 48000 * 4
#define PACKET_SIZE 50
#define MIN_SLEEP_TIME_US 1000ll
#define AUDIO_TIME_ADJUST_US -200000ll

MPlayer *player;
JavaVM *javaVM;

//读取AVPacket线程(生产者)
void *write_packet_to_queue(void *arg) {
    MPlayer *player = (MPlayer *) arg;
    AVPacket packet, *pkt = &packet;
    int ret;
    for (;;) {
        ret = av_read_frame(player->pFormatContext, pkt);
        if (ret < 0) {
            break;
        }
        if (pkt->stream_index == player->video_stream_index ||
            pkt->stream_index == player->audio_stream_index) {
            //根据AVPacket->stream_index获取对应的队列
            AVPacketQueue *queue = player->packets[pkt->stream_index];
            pthread_mutex_lock(&player->mutex);
            AVPacket *data = static_cast<AVPacket *>(queue_push(queue, &player->mutex,
                                                                &player->cond));
            pthread_mutex_unlock(&player->mutex);
            //拷贝（间接赋值，拷贝结构体数据）
            *data = packet;
        }
    }
    return NULL;
}

//获取当前播放时间
int64_t get_play_time(MPlayer *player) {
    return (int64_t) (av_gettime() - player->start_time);
}

/**
 * 延迟等待，音视频同步
 */
void player_wait_for_frame(MPlayer *player, int64_t stream_time) {
    pthread_mutex_lock(&player->mutex);
    for (;;) {
        int64_t current_video_time = get_play_time(player);
        int64_t sleep_time = stream_time - current_video_time;
        if (sleep_time < -300000ll) {
            // 300 ms late
            int64_t new_value = player->start_time - sleep_time;
            player->start_time = new_value;
            pthread_cond_broadcast(&player->cond);
        }

        if (sleep_time <= MIN_SLEEP_TIME_US) {
            // We do not need to wait if time is slower then minimal sleep time
            break;
        }

        if (sleep_time > 500000ll) {
            // if sleep time is bigger then 500ms just sleep this 500ms
            // and check everything again
            sleep_time = 500000ll;
        }

        //等待指定时长
        struct timeval now;
        gettimeofday(&now, NULL);
        struct timespec outtime;
        outtime.tv_sec = now.tv_sec + (sleep_time / 1000ll);
        outtime.tv_nsec = now.tv_usec * 1000 + sleep_time * 1000;
        pthread_cond_timedwait(&player->cond, &player->mutex, &outtime);
    }
    pthread_mutex_unlock(&player->mutex);
}

//视频解码
int decode_video(MPlayer *player, AVPacket *packet) {
    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(player->native_window, player->video_width,
                                     player->video_height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer windowBuffer;
    //申请内存
    player->yuv_frame = av_frame_alloc();
    player->rgba_frame = av_frame_alloc();
    if (player->rgba_frame == NULL || player->yuv_frame == NULL) {
        LOGD("Couldn't allocate video frame.");
        return -1;
    }

    // buffer中数据用于渲染,且格式为RGBA
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, player->video_width,
                                            player->video_height, 1);

    player->buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(player->rgba_frame->data, player->rgba_frame->linesize, player->buffer,
                         AV_PIX_FMT_RGBA,
                         player->video_width, player->video_height, 1);

    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(
            player->video_width,
            player->video_height,
            player->pCodecContext_video->pix_fmt,
            player->video_width,
            player->video_height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL);

    int frameFinished;
    //对该帧进行解码
    int ret = avcodec_decode_video2(player->pCodecContext_video, player->yuv_frame, &frameFinished,
                                    packet);
    if (ret < 0) {
        LOGD("avcodec_decode_video2 error...");
        return -1;
    }
    if (frameFinished) {
        // lock native window
        ANativeWindow_lock(player->native_window, &windowBuffer, 0);
        // 格式转换
        sws_scale(sws_ctx, (uint8_t const *const *) player->yuv_frame->data,
                  player->yuv_frame->linesize, 0, player->video_height,
                  player->rgba_frame->data, player->rgba_frame->linesize);
        // 获取stride
        uint8_t *dst = static_cast<uint8_t *>(windowBuffer.bits);
        int dstStride = windowBuffer.stride * 4;
        uint8_t *src = player->rgba_frame->data[0];
        int srcStride = player->rgba_frame->linesize[0];
        // 由于window的stride和帧的stride不同,因此需要逐行复制
        int h;
        for (h = 0; h < player->video_height; h++) {
            memcpy(dst + h * dstStride, src + h * srcStride, (size_t) srcStride);
        }

        //计算延迟
        int64_t pts = av_frame_get_best_effort_timestamp(player->yuv_frame);
        AVStream *stream = player->pFormatContext->streams[player->video_stream_index];
        //转换（不同时间基时间转换）
        int64_t time = av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
        //音视频帧同步
        player_wait_for_frame(player, time);

        ANativeWindow_unlockAndPost(player->native_window);
    }
//    //延迟等待
//    usleep(1000 * 16);
    return 0;
}

//音频解码
int decode_audio(MPlayer *player, AVPacket *packet) {
    int got_frame = 0, ret;
    //解码
    ret = avcodec_decode_audio4(player->pCodecContext_audio, player->audio_frame, &got_frame,
                                packet);
    if (ret < 0) {
        LOGD("avcodec_decode_audio4 error...");
        return -1;
    }
    //解码一帧成功
    if (got_frame > 0) {
        //音频格式转换
        swr_convert(player->swrContext, &player->audio_buffer, MAX_AUDIO_FRAME_SIZE,
                    (const uint8_t **) player->audio_frame->data, player->audio_frame->nb_samples);
        int out_buffer_size = av_samples_get_buffer_size(NULL, player->out_channel_nb,
                                                         player->audio_frame->nb_samples,
                                                         player->out_sample_fmt, 1);

        //音视频帧同步
        int64_t pts = packet->pts;
        if (pts != AV_NOPTS_VALUE) {
            AVStream *stream = player->pFormatContext->streams[player->audio_stream_index];
            player->audio_clock = av_rescale_q(pts, stream->time_base, AV_TIME_BASE_Q);
            player_wait_for_frame(player, player->audio_clock + AUDIO_TIME_ADJUST_US);
        }

        if (javaVM != NULL) {
            JNIEnv *env;
            (javaVM)->AttachCurrentThread(&env, NULL);
            jbyteArray audio_sample_array = (env)->NewByteArray(out_buffer_size);
            jbyte *sample_byte_array = (env)->GetByteArrayElements(audio_sample_array, NULL);
            //拷贝缓冲数据
            memcpy(sample_byte_array, player->audio_buffer, (size_t) out_buffer_size);
            //释放数组
            (env)->ReleaseByteArrayElements(audio_sample_array, sample_byte_array, 0);
            //调用AudioTrack的write方法进行播放
            (env)->CallIntMethod(player->audio_track, player->audio_track_write_mid,
                                 audio_sample_array, 0, out_buffer_size);
            //释放局部引用
            (env)->DeleteLocalRef(audio_sample_array);
//        usleep(1000 * 16);
        }
    }
    if (javaVM != NULL) {
        (javaVM)->DetachCurrentThread();
    }
    return 0;
}

//音视频解码线程(消费者)
void *decode_func(void *arg) {
    Decoder *decoder_data = (Decoder *) arg;
    MPlayer *player = decoder_data->player;
    int stream_index = decoder_data->stream_index;
    //根据stream_index获取对应的AVPacket队列
    AVPacketQueue *queue = player->packets[stream_index];
    int ret = 0;
    int video_frame_count = 0, audio_frame_count = 0;
    for (;;) {
        pthread_mutex_lock(&player->mutex);
        AVPacket *packet = (AVPacket *) queue_pop(queue, &player->mutex, &player->cond);
        pthread_mutex_unlock(&player->mutex);

        if (stream_index == player->video_stream_index) {//视频流
            ret = decode_video(player, packet);
            LOGD("decode video stream = %d", video_frame_count++);
        } else if (stream_index == player->audio_stream_index) {//音频流
            ret = decode_audio(player, packet);
            LOGD("decode audio stream = %d", audio_frame_count++);
        }
        av_packet_unref(packet);
        if (ret < 0) {
            break;
        }
    }

    return NULL;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_dovar_ffmpeg_1so_MainActivity_play(JNIEnv *env, jclass clazz) {
    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);

    //生产者线程
    pthread_create(&player->write_thread, NULL, write_packet_to_queue, (void *) player);
    sleep(1);
    player->start_time = 0;

    //消费者线程
    Decoder data1 = {player, player->video_stream_index}, *decoder_data1 = &data1;
    pthread_create(&player->video_thread, NULL, decode_func, (void *) decoder_data1);

    Decoder data2 = {player, player->audio_stream_index}, *decoder_data2 = &data2;
    pthread_create(&player->audio_thread, NULL, decode_func, (void *) decoder_data2);


    pthread_join(player->write_thread, NULL);
    pthread_join(player->video_thread, NULL);
    pthread_join(player->audio_thread, NULL);

    return 0;
}

//初始化输入格式上下文
int init_input_format_context(MPlayer *player, const char *file_name) {
    //注册所有组件
    av_register_all();
    //分配上下文
    player->pFormatContext = avformat_alloc_context();
    //打开视频文件
    if (avformat_open_input(&player->pFormatContext, file_name, NULL, NULL) != 0) {
        LOGD("Couldn't open file:%s\n", file_name);
        return -1;
    }
    //检索多媒体流信息
    if (avformat_find_stream_info(player->pFormatContext, NULL) < 0) {
        LOGD("Couldn't find stream information.");
        return -1;
    }
    //寻找音视频流索引位置
    int i;
    player->video_stream_index = -1;
    player->audio_stream_index = -1;
    for (i = 0; i < player->pFormatContext->nb_streams; i++) {
        if (player->pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
            && player->video_stream_index < 0) {
            player->video_stream_index = i;
        } else if (player->pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
                   && player->audio_stream_index < 0) {
            player->audio_stream_index = i;
        }
    }
    if (player->video_stream_index == -1) {
        LOGD("couldn't find a video stream.");
        return -1;
    }
    if (player->audio_stream_index == -1) {
        LOGD("couldn't find a audio stream.");
        return -1;
    }
    LOGD("video_stream_index=%d", player->video_stream_index);
    LOGD("audio_stream_index=%d", player->audio_stream_index);
    return 0;
}

//打开音视频解码器
int init_condec_context(MPlayer *player) {
    //获取codec上下文指针
    player->pCodecContext_video = player->pFormatContext->streams[player->video_stream_index]->codec;
    //寻找视频流的解码器
    player->video_codec = avcodec_find_decoder(player->pCodecContext_video->codec_id);
    if (player->video_codec == NULL) {
        LOGD("couldn't find video Codec.");
        return -1;
    }
    if (avcodec_open2(player->pCodecContext_video, player->video_codec, NULL) < 0) {
        LOGD("Couldn't open video codec.");
        return -1;
    }
    player->pCodecContext_audio = player->pFormatContext->streams[player->audio_stream_index]->codec;
    player->audio_codec = avcodec_find_decoder(player->pCodecContext_audio->codec_id);
    if (player->audio_codec == NULL) {
        LOGD("couldn't find audio Codec.");
        return -1;
    }
    if (avcodec_open2(player->pCodecContext_audio, player->audio_codec, NULL) < 0) {
        LOGD("Couldn't open audio codec.");
        return -1;
    }
    // 获取视频宽高
    player->video_width = player->pCodecContext_video->width;
    player->video_height = player->pCodecContext_video->height;
    return 0;
}

//视频解码
void video_player_prepare(MPlayer *player, JNIEnv *env, jobject surface) {
    // 获取native window
    player->native_window = ANativeWindow_fromSurface(env, surface);
}

//音频解码初始化
void audio_decoder_prepare(MPlayer *player) {
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    player->swrContext = swr_alloc();

    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = player->pCodecContext_audio->sample_fmt;
    //输出采样格式16bit PCM
    player->out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = player->pCodecContext_audio->sample_rate;
    //输出采样率
    player->out_sample_rate = in_sample_rate;
    //声道布局（2个声道，默认立体声stereo）
    uint64_t in_ch_layout = player->pCodecContext_audio->channel_layout;
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(player->swrContext,
                       out_ch_layout, player->out_sample_fmt, player->out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate,
                       0, NULL);
    swr_init(player->swrContext);
    //输出的声道个数
    player->out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
}

//音频播放器
void audio_player_prepare(MPlayer *player, JNIEnv *env, jclass jthiz) {
    jclass player_class = (env)->GetObjectClass(jthiz);
    if (!player_class) {
        LOGD("player_class not found...");
    }
    //AudioTrack对象
    jmethodID audio_track_method = (env)->GetMethodID(
            player_class, "createAudioTrack", "(II)Landroid/media/AudioTrack;");
    if (!audio_track_method) {
        LOGD("audio_track_method not found...");
    }
    jobject audio_track = (env)->CallObjectMethod(
            jthiz, audio_track_method, player->out_sample_rate, player->out_channel_nb);

    //调用play方法
    jclass audio_track_class = (env)->GetObjectClass(audio_track);
    jmethodID audio_track_play_mid = (env)->GetMethodID(audio_track_class, "play", "()V");
    (env)->CallVoidMethod(audio_track, audio_track_play_mid);

    player->audio_track = (env)->NewGlobalRef(audio_track);
    //获取write()方法
    player->audio_track_write_mid = (env)->GetMethodID(audio_track_class, "write", "([BII)I");

    //16bit 44100 PCM 数据
    player->audio_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRAME_SIZE);
    //解压缩数据
    player->audio_frame = av_frame_alloc();
}

//初始化队列
void init_queue(MPlayer *player, int size) {
    int i;
    for (i = 0; i < 2; ++i) {
        AVPacketQueue *queue = queue_init(size);
        player->packets[i] = queue;
    }
}

//释放队列
void delete_queue(MPlayer *player) {
    int i;
    for (i = 0; i < 2; ++i) {
        queue_free(player->packets[i]);
    }
}

extern "C" JNIEXPORT jint

JNICALL
Java_com_dovar_ffmpeg_1so_MainActivity_setup(JNIEnv *env, jclass clazz, jstring filePath, jobject surface) {

    env->GetJavaVM(&javaVM);//获取JavaVM
    const char *file_name = (env)->GetStringUTFChars(filePath, JNI_FALSE);
    int ret;
    player = static_cast<MPlayer *>(malloc(sizeof(MPlayer)));
    if (player == NULL) {
        return -1;
    }
    //初始化输入格式上下文
    ret = init_input_format_context(player, file_name);
    if (ret < 0) {
        return ret;
    }
    //初始化音视频解码器
    ret = init_condec_context(player);
    if (ret < 0) {
        return ret;
    }
    //初始化视频surface
    video_player_prepare(player, env, surface);
    //初始化音频相关参数
    audio_decoder_prepare(player);
    //初始化音频播放器
    audio_player_prepare(player, env, clazz);
    //初始化音视频packet队列
    init_queue(player, PACKET_SIZE);

    return 0;
}







