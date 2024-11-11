#include "record.h"

std::vector<float> audioBuffer;

void getDefaultInputDeviceFormat(AudioStreamBasicDescription *outFormat) {
    AudioDeviceID deviceID = 0;  // Default input device ID
    UInt32 size = sizeof(deviceID);

    // Get the default input device
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDefaultInputDevice, // Default input device selector
        kAudioObjectPropertyScopeGlobal,         // Global scope
        kAudioObjectPropertyElementMain          // Main element
    };

    OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                 &propertyAddress,
                                                 0,
                                                 NULL,
                                                 &size,
                                                 &deviceID);
    if (status != noErr) {
        printf("Error: Unable to get default input device (%d)\n", status);
        return;
    }

    propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
    propertyAddress.mScope = kAudioDevicePropertyScopeInput; // Input scope
    propertyAddress.mElement = kAudioObjectPropertyElementMain;

    size = sizeof(AudioStreamBasicDescription);
    status = AudioObjectGetPropertyData(deviceID,
                                        &propertyAddress,
                                        0,
                                        NULL,
                                        &size,
                                        outFormat);
    if (status != noErr) {
        printf("Error: Unable to get input device format (%d)\n", status);
        return;
    }

    // Print the format for debugging
    printf("Default Input Device Format:\n");
    printf("  Sample Rate: %f\n", outFormat->mSampleRate);
    printf("  Channels: %u\n", outFormat->mChannelsPerFrame);
    printf("  Bits per Channel: %u\n", outFormat->mBitsPerChannel);
    printf("  Bytes per Packet: %u\n", outFormat->mBytesPerPacket);
    printf("  Frames per Packet: %u\n", outFormat->mFramesPerPacket);
}


//Callback funciton for 
void handleInputBuffer(void *inUserData,
                       AudioQueueRef inAQ,
                       AudioQueueBufferRef inBuffer,
                       const AudioTimeStamp *inStartTime,
                       UInt32 inNumPackets,
                       const AudioStreamPacketDescription *inPacketDesc) {
    if (inNumPackets > 0) {
        // Cast mAudioData to int16_t* for 16-bit PCM
        float* pcmData = (float *)inBuffer->mAudioData;
        UInt32 numSamples = inBuffer->mAudioDataByteSize / sizeof(float);

        // Example: Print the first 10 samples
        for (UInt32 i = 0; i < numSamples; i++) {
            audioBuffer.push_back(pcmData[i]);
        }

        // Example: Process PCM data
        // You can send this data to a file, an encoder, or apply DSP effects
    }

    // Re-enqueue the buffer for further recording
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
}
