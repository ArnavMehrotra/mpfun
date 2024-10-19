#ifndef CODEC_H
#define CODEC_H

#include <vector>

//include necessary c headers from ffmpeg and mp3 huffman coder
extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/opt.h>
	#include <libavutil/samplefmt.h>
	#include <libavutil/avutil.h>
	#include "huffman.h"
}

std::vector<float> ffmpegDecompress(std::vector<std::vector<uint8_t>> rawFrames, AVCodecContext* codecCtx);
int mp3Compress(std::vector<float> samples);


#endif