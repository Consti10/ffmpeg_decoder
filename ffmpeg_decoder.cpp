//
// Created by consti10 on 26.01.22.
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <sstream>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavformat/avio.h>
}


// read the whole input file and store it inside memory
std::vector<uint8_t> readInputFile(const char* in_filename){
    FILE* f = fopen(in_filename, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s\n",in_filename);
        exit(1);
    }
    fseek(f, 0L, SEEK_END);
    const auto sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::cout<<"size:"<<sz<<"\n";
    std::vector<uint8_t> ret(sz);
    const auto data_len=fread(ret.data(), 1, sz, f);
    fclose(f);
    assert(data_len==sz);
    return ret;
}

void save_frame_pgm(const std::string& filename,AVCodecContext* codec_context,AVFrame* picture){
    FILE* f = fopen(filename.c_str(),"wb");
    unsigned char* buf=picture->data[0];
    int wrap=picture->linesize[0];
    int xsize=picture->width;
    int ysize=picture->height;
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (int i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize,f);
    fclose(f);
}


void save_frame(FILE* outf,AVCodecContext* codec_context,AVFrame* picture){
    for(int i=0; i<codec_context->height; i++)
        fwrite(picture->data[0] + i * picture->linesize[0], 1, codec_context->width, outf  );
    for(int i=0; i<codec_context->height/2; i++)
        fwrite(picture->data[1] + i * picture->linesize[1], 1, codec_context->width/2, outf );
    for(int i=0; i<codec_context->height/2; i++)
        fwrite(picture->data[2] + i * picture->linesize[2], 1, codec_context->width/2, outf );
}

/*void print_hw_decoders(){
    fprintf(stderr,"\n hw Decoders\n");
    AVHWAccel *first_hwaccel   = av_hwaccel_next(NULL);
    fprintf(stderr,"%p", first_hwaccel);
    AVHWAccel *hwaccel = first_hwaccel;
    AVHWAccel *h264 = NULL;
    const char * h264_name = "h264_vaapi";
    while (hwaccel != NULL)
    {
        if ( hwaccel != NULL)
        {
            fprintf(stderr,"%s ", hwaccel->name);
            if (strcmp(hwaccel->name, h264_name)== 0)
            {
                h264=hwaccel;
            }
        }
        hwaccel=av_hwaccel_next(hwaccel);

        if (hwaccel == first_hwaccel)
        {
            break;
        }
    }
    fprintf(stderr,"\n");
}*/

void video_decode(const char *in_filename,const char *out_filename)
{

    auto inputBuffer=readInputFile(in_filename);
    FILE* outf = fopen(out_filename, "w");
    if(!outf){
        fprintf(stderr, "could not open %s\n", in_filename);
        exit(1);
    }

    const AVCodec *codec;
    AVCodecContext *codec_context= NULL;
    AVFrame *picture;
    AVPacket avpkt;
    // added
    AVCodecParserContext *m_pCodecPaser;

    av_init_packet(&avpkt);

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    //codec = avcodec_find_encoder_by_name("hevc_v4l2m2m");
    //codec = avcodec_find_encoder_by_name("h264_nvdec");
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }else{
        printf("Found codec %s\n",codec->long_name);
    }

    codec_context = avcodec_alloc_context3(codec);
    if (codec_context->hwaccel != nullptr){
        fprintf(stderr, "HW accel IN USE : %s\n",codec_context->hwaccel->name);
    }else{
        fprintf(stderr, "NO HW accel IN USE\n");
    }

    picture = av_frame_alloc();

    if((codec->capabilities)&AV_CODEC_CAP_TRUNCATED)
        (codec_context->flags) |= AV_CODEC_FLAG_TRUNCATED;

    codec_context->width = 1920;
    codec_context->height = 1080;
    codec_context->time_base=AVRational{120,1};
    codec_context->framerate=AVRational{120,1};

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        fprintf(stderr, "could not open codec\n");
        exit(1);
    }

    m_pCodecPaser = av_parser_init(AV_CODEC_ID_H264);
    if(!m_pCodecPaser){
        std::cout<<"Cannot find parser\n";
    }

    int in_remaining=inputBuffer.size();
    int in_offset=0;
    int frameCount=0;
    while(in_remaining){
        const int len_parse = av_parser_parse2(m_pCodecPaser,codec_context,&avpkt.data, &avpkt.size,
                               &inputBuffer[in_offset],in_remaining,
                               0,0,0);
        std::cout<<"consumed"<<len_parse<<"bytes\n";
        in_offset += len_parse;
        in_remaining  -= len_parse;

        if(avpkt.size){
            std::cout<<"Got packet: "<<avpkt.size<<" bytes\n";
            //decode_frame(data, size);
            const auto before=std::chrono::steady_clock::now();
            int res=avcodec_send_packet(codec_context,&avpkt);
            if(res!=0){
                std::cout<<"avcodec_send_packet returned:"<<res<<"\n";
            }

            int len=-1;
            while (len!=0){
                len = avcodec_receive_frame(codec_context,picture);
                if(len!=0){
                    std::cout<<"no frame yet\n";
                }
            }
            const auto decode_delay=std::chrono::steady_clock::now()-before;
            std::cout<<"Decode delay:"<<((float)std::chrono::duration_cast<std::chrono::microseconds>(decode_delay).count()/1000.0f)<<" ms\n";
            //const int len = avcodec_receive_frame(codec_context,picture);
            //std::cout<<"avcodec_receive_frame returned:"<<len<<"\n";

            if (len < 0) {
                fprintf(stderr, "Error while decoding frame %d\n", frameCount);
                exit(1);
            }
            const bool got_picture = len==0;
            if(got_picture){
                std::cout<<"Got picture "<<frameCount<<"\n";
                std::stringstream ss;
                ss<<"out/pic_"<<frameCount<<".pgm";
                save_frame_pgm(ss.str(),codec_context,picture);
                frameCount++;
            }else{
                std::cout<<"Got no picture\n";
            }
        }
    }


    fclose(outf);

    av_parser_close(m_pCodecPaser);
    avcodec_close(codec_context);
    av_free(codec_context);
    av_frame_free(&picture);
    printf("\n");
}

int main(int argc, char **argv){
    //avcodec_register_all();
    video_decode("dji.h264", "test_out.raw");

    return 0;
}