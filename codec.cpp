#include <vector>
#include "codec.h"
#include "dsp.h"

#define MP3_BIT_RATE 128
#define MP3_BUFFER_SIZE 8192

#define BLOCK_SIZE 512.0f

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

std::vector<int16_t> convertSamples(const std::vector<float> samples) {
    std::vector<int16_t> convertedSamples;
    for(int i = 0; i < samples.size(); i++) {
        if(samples[i] > 32767.0f) convertedSamples.push_back(32767);
        else if(samples[i] < -32767.0f) convertedSamples.push_back(-32767);
        else convertedSamples.push_back(static_cast<int16_t>(roundf(samples[i])));
        
    }

    return convertedSamples;
}

template<typename T>
void scalePCM(std::vector<T>& samples) {
    for(int i = 0; i < samples.size(); i++) {
        samples[i] *= 32767.0f;
    }
}

std::vector<int16_t> convertFloat(std::vector<float> samples) {
    scalePCM(samples);
    return convertSamples(samples);
}

std::vector<float> convertPCM(const std::vector<int16_t> samples) {
    std::vector<float> convertedSamples;
    for(int i = 0; i < samples.size(); i++) {
        convertedSamples.push_back(samples[i] / 32767.0f);
    }

    return convertedSamples;
}


std::vector<char> lameCompress(std::vector<int16_t> samples, int channels, int sampleRate) {

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
    int16_t* pcmPtr = samples.data();

    std::vector<char> mp3Stream;

    //encode PCM samples
    int numSamples = samples.size() / channels;
    while(numSamples > 0) {
        int encodeSamples = std::min(numSamples, 1152);
        int encodedBytes = -1;
        if(channels == 1) encodedBytes = lame_encode_buffer(lame, pcmPtr, NULL, encodeSamples, mp3Buffer, MP3_BUFFER_SIZE);
        else encodedBytes = lame_encode_buffer_interleaved(lame, pcmPtr, encodeSamples, mp3Buffer, MP3_BUFFER_SIZE);

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
int lossyCompress(const std::vector<float> samples) {
    
    std::vector<double> sampleCopy;
    for(int i = 0; i < samples.size(); i++) {
        sampleCopy.push_back(samples[i]);
    }

    scalePCM(sampleCopy);

    std::vector<double> output((samples.size() / 2) + 1, 0.0f);

    fftw_plan plan = fftw_plan_r2r_1d(sampleCopy.size(), sampleCopy.data(), output.data(), FFTW_REDFT10, FFTW_ESTIMATE);

    fftw_execute(plan);

    fftw_destroy_plan(plan);

    std::vector<double> backAgain(output.size() * 2, 0.0f);

    fftw_plan inverse = fftw_plan_r2r_1d(output.size(), output.data(), backAgain.data(), FFTW_REDFT01, FFTW_ESTIMATE);

    fftw_execute(inverse);

    fftw_destroy_plan(inverse);

    double mse = 0.0f;

    for(int i = 0; i < samples.size(); i++) {
        double originalPCM = samples[i] * 32767.0f;
        double newPCM = backAgain[i];

        mse += (originalPCM - newPCM) * (originalPCM - newPCM);
    }
    
    mse /= samples.size();

    printf("Mean Squared Error for mdct and imdct: %.2f\n", mse);

    return 0;
}