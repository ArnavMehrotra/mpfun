#ifndef DSP_H
#define DSP_H

#include <iostream>

//functions for compression pipeline
std::vector<float> mdct(std::vector<float> samples);
std::vector<int> quantize(const std::vector<float> coeffs, const std::vector<float> scaleFactors);
void scaleByConstant(std::vector<float>& samples, float c);
std::vector<float> imdct(std::vector<float> samples);

//audio effects
void filter(std::vector<float>& audio, int sampleRate, float cutoff);
void reverb(std::vector<float>& audio, int sampleRate, float delayTime, float decay);
void chorus(std::vector<float>& audio, int sampleRate, float depth, float rate, float delay, float wet);

//utility
std::vector<float> sanitySin(float frequency, float duration, int sampleRate, int numChannels);

#endif