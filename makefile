CC = /usr/bin/g++
OBJS = main.o mp4Read.o dsp.o codec.o
EXE = codec

# Include FFmpeg libraries
LIBS = libavformat libavcodec libavutil

LDFLAGS = $(shell pkg-config --libs $(LIBS))
CXXFLAGS = $(shell pkg-config --cflags $(LIBS)) -std=c++11

%.o : %.c
	$(CC) -c $(CXXFLAGS) $< -o $@

codec : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJS) $(EXE)

