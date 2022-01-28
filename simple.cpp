// START of file simple.cpp
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <math.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#ifdef __cplusplus
}
#endif

int main(int argc, char **argv){
    //avcodec_register_all();
    const AVCodec *codec;
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    std::cout<<"HELLO\n";
    return 0;
}

// END of file simple.cpp
