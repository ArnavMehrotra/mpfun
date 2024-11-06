#include <stdio.h>
#include <vector>


extern "C" {
    #include <AudioToolbox/AudioToolbox.h>
}

#define SAMPLE_RATE 48000.0

extern std::vector<float> audioBuffer;

AudioUnit setupCoreAudio();