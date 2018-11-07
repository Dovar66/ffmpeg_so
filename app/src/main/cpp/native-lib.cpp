#include <jni.h>;
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <unistd.h>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
#include <libswresample/swresample.h>
}
#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG,"DOVAR_FFMPGE------>>",FORMAT,##__VA_ARGS__);


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
                /*  //进行短暂休眠。如果休眠时间太长会导致播放的每帧画面有延迟感，如果短会有加速播放的感觉。
                  //一般一每秒60帧——16毫秒一帧的时间进行休眠
                  usleep(1000 * 20);*/
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



