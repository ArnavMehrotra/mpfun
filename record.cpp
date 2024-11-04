#include "record.h"

std::vector<int16_t> audioBuffer;

static OSStatus processAudio(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp,
                    UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {

     // Get the audio unit
    AudioUnit audioUnit = *(AudioUnit *)inRefCon;

    // Prepare an AudioBufferList to hold the captured audio
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = inNumberFrames * sizeof(int16_t);
    bufferList.mBuffers[0].mData = malloc(inNumberFrames * sizeof(int16_t));

    // Render the audio data
    AudioUnitRender(audioUnit,
                    ioActionFlags,
                    inTimeStamp,
                    inBusNumber,
                    inNumberFrames,
                    &bufferList);

    for(int i = 0; i < inNumberFrames; i++) {
        audioBuffer.push_back(((short*)bufferList.mBuffers[0].mData)[i]);
    }

    free(bufferList.mBuffers[0].mData);
    return noErr;
}

AudioUnit setupCoreAudio() {
    AudioStreamBasicDescription fmt;
    fmt.mSampleRate = SAMPLE_RATE;
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    fmt.mFramesPerPacket  = 1;
    fmt.mChannelsPerFrame = 1; // Mono input
    fmt.mBitsPerChannel   = 16;
    fmt.mBytesPerPacket   = 2;
    fmt.mBytesPerFrame    = 2;


    AudioComponentDescription desc;
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags        = 0;
    desc.componentFlagsMask    = 0;

    AudioComponent inputComponent = AudioComponentFindNext(NULL, &desc);
    AudioUnit audioUnit;
    AudioComponentInstanceNew(inputComponent, &audioUnit);

    UInt32 enableIO = 1;
    AudioUnitSetProperty(audioUnit,
                        kAudioOutputUnitProperty_EnableIO,
                        kAudioUnitScope_Input,
                        1,
                        &enableIO,
                        sizeof(enableIO));

    AudioUnitSetProperty(audioUnit,
                     kAudioUnitProperty_StreamFormat,
                     kAudioUnitScope_Output,
                     1,
                     &fmt,
                     sizeof(fmt));

    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = processAudio;
    callbackStruct.inputProcRefCon = &audioUnit;
    AudioUnitSetProperty(audioUnit,
                        kAudioUnitProperty_SetRenderCallback,
                        kAudioUnitScope_Global,
                        0,
                        &callbackStruct,
                        sizeof(callbackStruct));


    AudioUnitInitialize(audioUnit);


    return audioUnit;
}