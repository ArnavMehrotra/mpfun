#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#include "permissions.h"

extern "C" void RequestMicrophonePermission() {
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];

    if (status == AVAuthorizationStatusNotDetermined) {
        // Request permission
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
            if (granted) {
                printf("Microphone access granted.\n");
            } else {
                printf("Microphone access denied.\n");
            }
        }];
    } else if (status == AVAuthorizationStatusAuthorized) {
        printf("Microphone access already granted.\n");
    } else {
        printf("Microphone access denied.\n");
    }
}
