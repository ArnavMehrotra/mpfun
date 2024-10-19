#include <iostream>

void filter(std::vector<float>& audio, int sampleRate, float cutoff);
void reverb(std::vector<float>& audio, int sampleRate, float delayTime, float decay);
void chorus(std::vector<float>& audio, int sampleRate, float depth, float rate, float delay, float wet);
std::vector<float> sanitySin(float frequency, float duration, int sampleRate, int numChannels);
std::vector<float> mdct(std::vector<float>& samples);
std::vector<int> quantize(const std::vector<float> coeffs, const std::vector<float> scaleFactors);