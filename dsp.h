#include <iostream>



std::vector<float> sanitySin(float frequency, float duration, int sampleRate, int numChannels);
void filter(std::vector<float>& audio, int sampleRate, float cutoff);
