#include <iostream>



std::vector<float> sanitySin(float frequency, float duration, int sampleRate, int numChannels);
void filter(std::vector<float>& audio, int sampleRate, float cutoff);
void reverb(std::vector<float>& audio, int sampleRate, float delayTime, float decay);
void chorus(std::vector<float>& audio, int sampleRate, float depth, float rate, float delay, float wet);
