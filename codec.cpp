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

    float mdctEnergy = 0.0f;
    for(int i = 0; i < mdctCoeffs.size(); i++) {
        mdctEnergy += mdctCoeffs[i] * mdctCoeffs[i];
    }

    printf("old energy: %.2f new energy: %.2f\n", ogEnergy, mdctEnergy);

    //you can apply some scalefactors if you want
    //mdct coefficients are already scaled by a factor of 2 / sqrt(BLOCK_SIZE = 512)
    std::vector<float> scaleFactors(mdctCoeffs.size(), 1.0f);
    std::vector<int> quantized = quantize(mdctCoeffs, scaleFactors);

    return 0;
}