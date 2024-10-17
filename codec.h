#include <vector>

#pragma once
extern "C" {
    struct AVCodecContext;
}

std::vector<float> ffmpegDecompress(std::vector<std::vector<uint8_t>> rawFrames, AVCodecContext* codecCtx);