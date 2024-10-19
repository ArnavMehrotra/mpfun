#include <vector>
#include "codec.h"
#include "dsp.h"

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

int mp3Compress(std::vector<float> samples) {
    
    std::vector<float> mdctCoeffs = mdct(samples);

    float ogEnergy = 0.0f;
    for(int i = 0; i < samples.size(); i++) {
        ogEnergy += samples[i] * samples[i];
    }

    int smolct = 0;

    float mdctEnergy = 0.0f;
    for(int i = 0; i < mdctCoeffs.size(); i++) {
        mdctEnergy += mdctCoeffs[i] * mdctCoeffs[i];
        if(fabs(mdctCoeffs[i]) < 1) smolct++;
    }


    printf("old energy: %.2f new energy: %.2f %d small terms\n", ogEnergy, mdctEnergy, smolct);

    //you can apply some scalefactors if you want
    //mdct coefficients are already scaled by a factor of 2 / sqrt(BLOCK_SIZE = 512)
    std::vector<float> scaleFactors(mdctCoeffs.size(), 0.5f);
    std::vector<int> quantized = quantize(mdctCoeffs, scaleFactors);

    int code = 0;
    int check = 0;
    int zcount = 0;

    for(int i = 0; i < quantized.size(); i++) {
        if(quantized[i] == 0) zcount++;
        check += abs(quantized[i]);
    }

    printf("quant sum %d and %d 0s\n", check, zcount);

    for(int i = 0; i < quantized.size(); i += 2) {
        code += huffmanCode(quantized[i], quantized[i + 1]);
    }

    printf("code sum %d\n", code);

    return 0;
}