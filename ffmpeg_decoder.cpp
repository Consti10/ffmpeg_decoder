//
// Created by consti10 on 26.01.22.
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cassert>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavformat/avio.h>
}


#define INBUF_SIZE 4096

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
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    codec_context = avcodec_alloc_context3(codec);
    picture = av_frame_alloc();

    if((codec->capabilities)&AV_CODEC_CAP_TRUNCATED)
        (codec_context->flags) |= AV_CODEC_FLAG_TRUNCATED;

    codec_context->width = 1920;
    codec_context->height = 1080;

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
            std::cout<<"Got packet"<<avpkt.size<<"\n";
            //decode_frame(data, size);
            int res=avcodec_send_packet(codec_context,&avpkt);
            std::cout<<"avcodec_send_packet returned:"<<res<<"\n";

            const int len = avcodec_receive_frame(codec_context,picture);
            std::cout<<"avcodec_receive_frame returned:"<<len<<"\n";

            if (len < 0) {
                fprintf(stderr, "Error while decoding frame %d\n", frameCount);
                exit(1);
            }
            const bool got_picture = len==0;
            if(got_picture){
                std::cout<<"Got picture "<<frameCount<<"\n";
                for(int i=0; i<codec_context->height; i++)
                    fwrite(picture->data[0] + i * picture->linesize[0], 1, codec_context->width, outf  );
                for(int i=0; i<codec_context->height/2; i++)
                    fwrite(picture->data[1] + i * picture->linesize[1], 1, codec_context->width/2, outf );
                for(int i=0; i<codec_context->height/2; i++)
                    fwrite(picture->data[2] + i * picture->linesize[2], 1, codec_context->width/2, outf );
                frameCount++;
            }else{
                std::cout<<"Got no picture\n";
            }
        }
    }

    /*
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    int i;
    int frame, got_picture=false, len,res;
    frame = 0;
    for(;;) {
        avpkt.size = fread(inbuf, 1, INBUF_SIZE, f);
        if (avpkt.size == 0)
            break;

        avpkt.data = inbuf;

        //
        //ret = av_parser_parse2(m_pCodecPaser,c, &avpkt.data,&avpkt.size,
        //                       m_packet.data,m_packet.size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        //


        while (avpkt.size > 0) {

            res=avcodec_send_packet(c,&avpkt);
            std::cout<<"avcodec_send_packet returned:"<<res<<"\n";

            len = avcodec_receive_frame(c,picture);
            std::cout<<"avcodec_receive_frame returned:"<<len<<"\n";

            if (len < 0) {
                fprintf(stderr, "Error while decoding frame %d\n", frame);
                exit(1);
            }
            got_picture = len==0;

            if (got_picture) {
                printf("saving frame %3d\n", frame);
                fflush(stdout);
                for(i=0; i<c->height; i++)
                    fwrite(picture->data[0] + i * picture->linesize[0], 1, c->width, outf  );
                for(i=0; i<c->height/2; i++)
                    fwrite(picture->data[1] + i * picture->linesize[1], 1, c->width/2, outf );
                for(i=0; i<c->height/2; i++)
                    fwrite(picture->data[2] + i * picture->linesize[2], 1, c->width/2, outf );
                frame++;
            }
            avpkt.size -= len;
            avpkt.data += len;
        }
    }

    avpkt.data = NULL;
    avpkt.size = 0;*/

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