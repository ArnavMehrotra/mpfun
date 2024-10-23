#include <vector>
#include "codec.h"
#include "dsp.h"

#define MP3_BIT_RATE 128
#define MP3_BUFFER_SIZE 8192


//decode headerless (or headered?) AAC frames into raw PCM data using FFmpeg
std::vector<float> ffmpegDecompress(std::vector<std::vector<uint8_t>> rawFrames, AVCodecContext* codecCtx) {

	std::vector<float> decodedSamples;
	AVFrame* decodedFrame = av_frame_alloc();
	AVPacket* packet = av_packet_alloc();

	for(int i = 0; i < rawFrames.size(); i++) {
		av_new_packet(packet, rawFrames[i].size());
		memcpy(packet->data, rawFrames[i].data(), rawFrames[i].size());
		avcodec_send_packet(codecCtx, packet);

		while(avcodec_receive_frame(codecCtx, decodedFrame) == 0) {			
			//process planar audio
			if(av_sample_fmt_is_planar( (AVSampleFormat) decodedFrame->format)) {
				for(int i = 0; i < decodedFrame->nb_samples; i++) {
					for(int ch = 0; ch < 2; ch++) {
						decodedSamples.push_back(reinterpret_cast<float*>(decodedFrame->data[ch])[i]);
					}
				}
			}
			//process non-planar audio
			else {
				decodedSamples.insert(decodedSamples.end(), decodedFrame->data[0], decodedFrame->data[0] + decodedFrame->nb_samples);
			}
		}
	}

	av_packet_unref(packet);
	av_frame_unref(decodedFrame);

	return decodedSamples;		
}

std::vector<int16_t> convertPCMFloat(const std::vector<float> samples) {
    std::vector<int16_t> convertedSamples;
    for(int i = 0; i < samples.size(); i++) {
        if(samples[i] > 32767.0f) convertedSamples.push_back(32767);
        else if(samples[i] < -32767.0f) convertedSamples.push_back(-32767);
        else convertedSamples.push_back(static_cast<int16_t>(roundf(samples[i])));
        
    }

    return convertedSamples;
}

void scalePCM(std::vector<float>& samples) {
    for(int i = 0; i < samples.size(); i++) {
        samples[i] *= 32767.0f;
    }
}

//compress PCM samples to mp3 using liblamemp3
std::vector<char> lameCompress(std::vector<float> samples, int channels, int sampleRate) {

    //prepare pcm data by scaling and converting to pcm int
    scalePCM(samples);
    std::vector<int16_t> convertedSamples = convertPCMFloat(samples);

    //initialize liblamemp3 
    lame_t lame = lame_init();

    lame_set_num_channels(lame, channels);
    lame_set_in_samplerate(lame, sampleRate);
    lame_set_brate(lame, MP3_BIT_RATE);

    if(channels == 1) lame_set_mode(lame, MONO);
    else lame_set_mode(lame, STEREO);

    lame_set_quality(lame, 2);

    if(lame_init_params(lame) < 0) {
        printf("failed to initialize lame!\n");
        lame_close(lame);
        return {};
    }


    //allocate buffer for lame codec
    unsigned char mp3Buffer[MP3_BUFFER_SIZE];
    int16_t* pcmPtr = convertedSamples.data();

    std::vector<char> mp3Stream;

    //encode PCM samples
    int numSamples = convertedSamples.size() / channels;
    while(numSamples > 0) {
        int encodeSamples = std::min(numSamples, 1152);
        int encodedBytes = lame_encode_buffer_interleaved(lame, pcmPtr, encodeSamples, mp3Buffer, MP3_BUFFER_SIZE);

        if(encodedBytes < 0) {
            printf("error encoding mp3 frame\n");
            return {};
        }

        mp3Stream.insert(mp3Stream.end(), mp3Buffer, mp3Buffer + encodedBytes);
        numSamples -= encodeSamples;
        
        //number of bytes read is samples read when using interleaved stereo
        pcmPtr += encodeSamples * channels;
    }
    

    //flush codec and free up resources
    int encodedBytes = lame_encode_flush(lame, mp3Buffer, MP3_BUFFER_SIZE);
    if(encodedBytes) mp3Stream.insert(mp3Stream.end(), mp3Buffer, mp3Buffer + encodedBytes);

    lame_close(lame);

    return mp3Stream;
}

//compress and code raw PCM samples into a lossy compressed format like MP3, only worse
//TODO we're losing too much data! fix it!
int lossyCompress(std::vector<float>& samples) {
    
    scalePCM(samples);

    float rawEnergy = 0.0f;
    int rawSmall = 0;
    for(int i = 0; i < samples.size(); i++) {
        if(fabs(samples[i]) < 0.01f) rawSmall++;
        rawEnergy += samples[i] * samples[i];
    }

    std::vector<float> mdctCoeffs = mdct(samples);

    std::vector<float> inverse = imdct(mdctCoeffs);

    int wrongCt = 0;
    for(int i = 0; i < samples.size(); i++) {
        float originalPCM = samples[i] /32767.0f;
        float newPCM = inverse[i] /32767.0f;
        if((originalPCM) - (newPCM) > 0.1f) {
            //printf("had: %.2f got: %.2f\n", originalPCM, newPCM);
            wrongCt++;
        }
    }

    printf("%d samples differ when apply mdct and inversing\n", wrongCt);

    samples = inverse;

    //you can apply some scalefactors if you want
    //mdct coefficients are already scaled by a factor of 2 / sqrt(BLOCK_SIZE = 512)
    // std::vector<float> scaleFactors(mdctCoeffs.size(), 0.5f);
    // std::vector<int> quantized = quantize(mdctCoeffs, scaleFactors);

    // int check = 0;
    // int zcount = 0;

    // for(int i = 0; i < quantized.size(); i++) {
    //     if(quantized[i] == 0) zcount++;
    //     check += abs(quantized[i]);
    // }

    // printf("quant sum %d and %d 0s\n", check, zcount);

    
    return 0;
}