_LDFLAGS := $(LDFLAGS) -I/usr/local/include -L/usr/local/lib -lavformat -lm -latomic -lz -lavcodec -pthread -lm -latomic -lz -lswresample -lm -latomic -lswscale -lm -latomic -lavutil -pthread -lm -latomic #`pkg-config --cflags --libs libavformat libswscale`
_CPPFLAGS := $(CFLAGS)  -std=c++17

all: simple #ffmpeg_decoder

simple: simple.cpp
	$(CXX) -o $@ $^ $(_LDFLAGS) $(_CPPFLAGS)

#ffmpeg_decoder: ffmpeg_decoder.cpp
#	$(CXX) -o $@ $^ $(_LDFLAGS) $(_CPPFLAGS)

clean:
	rm -rf ffmpeg_decoder ffmpeg_decoder.o simple simple.o

# http://dranger.com/ffmpeg/tutorial01.html
#gcc -o decoder ffmpeg_decoder.c -lavutil -lavformat -lavcodec -lz -lavutil -lm

# gcc -o decoder ffmpeg_decoder.c -L./lib/ -lavcodec -lavutil
# gcc -o decoder ffmpeg_decoder.c -I/usr/local/include/ -L/usr/local/lib/ -lavcodec -lavuti

### https://soledadpenades.com/posts/2009/linking-with-ffmpegs-libav/
# gcc -o simple.o -c simple.cpp -std=gnu++11 `pkg-config --cflags --libs libavformat libswscale`