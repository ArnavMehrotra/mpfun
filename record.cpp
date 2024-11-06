#include "record.h"
#include "permissions.h"

std::vector<float> audioBuffer;

static OSStatus processAudio(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp,
                             UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {

    AudioUnit audioUnit = *(AudioUnit *)inRefCon;

    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = inNumberFrames * sizeof(float);

    float buffer[inNumberFrames];
    bufferList.mBuffers[0].mData = buffer;

    OSStatus status = AudioUnitRender(audioUnit,
                                      ioActionFlags,
                                      inTimeStamp,
                                      inBusNumber,
                                      inNumberFrames,
                                      &bufferList);
    if (status != noErr) {
        printf("rendering audio unit has error %d", status);
        return status;  // Handle errors properly
    }
    printf("captured %d frames\n", inNumberFrames);
    // Process the captured audio data
    for (UInt32 i = 0; i < inNumberFrames; i++) {
        audioBuffer.push_back(buffer[i]);
    }

    return noErr;
}


AudioUnit setupCoreAudio() {

    RequestMicrophonePermission();

    OSStatus status;

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
    status = AudioUnitSetProperty(audioUnit,
                        kAudioOutputUnitProperty_EnableIO,
                        kAudioUnitScope_Input,
                        1,
                        &enableIO,
                        sizeof(enableIO));
    
    if(status != noErr) {
        printf("could not enable audio unit input, error has status: %d\n", status);
        return NULL;
    }

    UInt32 disableOutput = 0;
    status = AudioUnitSetProperty(audioUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output, // Disable output
                                  0, // Output element
                                  &disableOutput,
                                  sizeof(disableOutput));
    if (status != noErr) {
        printf("Could not disable audio unit output, error has status: %d\n", status);
        return NULL;
    }

    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = processAudio;
    callbackStruct.inputProcRefCon = NULL;
    status = AudioUnitSetProperty(audioUnit,
                        kAudioUnitProperty_SetRenderCallback,
                        kAudioUnitScope_Global,
                        1,
                        &callbackStruct,
                        sizeof(callbackStruct));

    if(status != noErr) {
        printf("could not set audio unit callback function, error has status %d\n", status);
        return NULL;
    }

    // Get the default input device
    AudioObjectPropertyAddress propertyAddress = {
        .mSelector = kAudioHardwarePropertyDefaultInputDevice,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster
    };

    AudioDeviceID deviceID;
    UInt32 propertySize = sizeof(deviceID);

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize, &deviceID);
    if (status != noErr) {
        printf("Error getting default device ID, error has status: %d\n", status);
        return NULL;
    }

    status = AudioUnitSetProperty(audioUnit,
                        kAudioOutputUnitProperty_CurrentDevice,
                        kAudioUnitScope_Global,
                        0,
                        &deviceID,
                        sizeof(deviceID));

    if(status != noErr) {
        printf("Error setting audio device, error has status: %d\n", status);
        return NULL;
    }

    status = AudioUnitInitialize(audioUnit);

    if(status != noErr) {
        printf("could not intialize audio unit, error has status %d\n", status);
        return NULL;
    }
    else {
        printf("successfully intialized audio input!\n");
    }

    return audioUnit;
}