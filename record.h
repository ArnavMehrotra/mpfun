#include <stdio.h>
#include <vector>


extern "C" {
    #include <AudioToolbox/AudioToolbox.h>
}

#define SAMPLE_RATE 48000.0

extern std::vector<float> audioBuffer;

void getDefaultInputDeviceFormat(AudioStreamBasicDescription *outFormat);

void handleInputBuffer(void *inUserData,
                       AudioQueueRef inAQ,
                       AudioQueueBufferRef inBuffer,
                       const AudioTimeStamp *inStartTime,
                       UInt32 inNumPackets,
                       const AudioStreamPacketDescription *inPacketDesc);
