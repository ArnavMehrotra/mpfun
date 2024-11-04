#include <stdio.h>
#include <vector>


extern "C" {
    #include <AudioToolbox/AudioToolbox.h>
}

#define SAMPLE_RATE 44100

extern std::vector<short> audioBuffer;

AudioUnit setupCoreAudio();