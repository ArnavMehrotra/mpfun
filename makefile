CC = /usr/bin/gcc
CXX = /usr/bin/g++
OBJS = main.o mp4Read.o dsp.o codec.o huffman.o record.o
EXE = codec

# Include FFmpeg libraries
FFLIBS = libavformat libavcodec libavutil


LAMELD = -L/opt/homebrew/Cellar/lame/3.100/lib -lmp3lame
LAMECXX = -I/opt/homebrew/include/lame

FFTWLD = -L/opt/homebrew/Cellar/fftw/3.3.10_1/lib -lfftw3
FFTWCXX = -I/opt/homebrew/include/

LDFLAGS = $(shell pkg-config --libs $(FFLIBS)) $(LAMELD) $(FFTWLD)  -framework CoreAudio -framework AudioToolbox
CXXFLAGS = $(shell pkg-config --cflags $(FFLIBS)) $(LAMECXX) $(FFTWCXX) -std=c++11

%.o : %.c
	$(CC) -c $< -o $@

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

codec : $(OBJS)
	$(CXX) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJS) $(EXE)
