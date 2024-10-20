#include <vector>
#include "codec.h"
#include "dsp.h"

struct pairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};


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

//compress and code raw PCM samples into bitstream for MP3 file data
//TODO:: we're losing too much data! fix it!
int mp3Compress(std::vector<float> samples) {

    scaleByConstant(samples, 10.0f);

    float rawEnergy = 0.0f;
    int rawSmall = 0;
    for(int i = 0; i < samples.size(); i++) {
        if(fabs(samples[i]) < 0.01f) rawSmall++;
        rawEnergy += samples[i] * samples[i];
    }

    printf("raw samples have energy %.2f, %d samples are < 0.01\n", rawEnergy, rawSmall);

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

    int check = 0;
    int zcount = 0;

    for(int i = 0; i < quantized.size(); i++) {
        if(quantized[i] == 0) zcount++;
        check += abs(quantized[i]);
    }

    printf("quant sum %d and %d 0s\n", check, zcount);

    int code = 0;
    
    std::unordered_map<std::pair<int, int>, int, pairHash> codeCache;

    for(int i = 0; i < quantized.size(); i += 2) {
        std::pair<int, int> key = std::make_pair(abs(quantized[i]), abs(quantized[i + 1]));
        if(codeCache.find(key) != codeCache.end()) {
            code += codeCache[key];
            continue;
        }

        printf("coding %d and %d\n", quantized[i], quantized[i + 1]);

        int newCode = huffmanCode(quantized[i], quantized[i + 1]);

        code += newCode;
        codeCache[key] = newCode;
    }

    printf("code sum %d\n", code);

    return 0;
}