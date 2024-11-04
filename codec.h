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
	#include <lame.h>

	#include <fftw3.h>

	#include "huffman.h"
}

std::vector<int16_t> convertFloat(std::vector<float> samples);
std::vector<char> lameCompress(std::vector<int16_t> samples, int channels, int sampleRate);
std::vector<float> ffmpegDecompress(std::vector<std::vector<uint8_t>> rawFrames, AVCodecContext* codecCtx);
int lossyCompress(const std::vector<float> samples);


#endif