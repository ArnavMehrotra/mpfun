#include <iostream>
#include <cmath>
#include "mp4Read.h"
#include "dsp.h"


extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/opt.h>
	#include <libavutil/samplefmt.h>
	#include <libavutil/avutil.h>
}


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

int writeWAV(std::vector<float> data, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
	std::ofstream out("out.wav", std::ios::binary);	

	if(!out.is_open()) return -1;

	uint32_t dataSize = data.size() * sizeof(float);
	uint32_t chunkSize = 16;
	uint32_t fileSize = 4 + (8 + dataSize) + (8 + chunkSize);
	uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
	uint16_t blockAlign = numChannels * bitsPerSample / 8;
	uint16_t audioFormat = 3;

	out.write("RIFF", 4);
	out.write(reinterpret_cast<const char*>(&fileSize), 4);
	out.write("WAVE", 4);

	out.write("fmt ", 4);
	out.write(reinterpret_cast<const char*>(&chunkSize), 4);
	out.write(reinterpret_cast<const char*>(&audioFormat), 2);
	out.write(reinterpret_cast<const char*>(&numChannels), 2);
	out.write(reinterpret_cast<const char*>(&sampleRate), 4);
	out.write(reinterpret_cast<const char*>(&byteRate), 4);
	out.write(reinterpret_cast<const char*>(&blockAlign), 2);
	out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

	out.write("data", 4);
	out.write(reinterpret_cast<const char*>(&dataSize), 4);
	out.write(reinterpret_cast<const char*>(data.data()), dataSize);

	out.close();

	return 0;
}

int main(int argc, char** argv) {
	std::string fName("whereisshe.mp4");

	//initialize ffmpeg
	AVFormatContext *formatCtx = nullptr;
	AVCodecContext *codecCtx = nullptr;
	
	if(avformat_open_input(&formatCtx, fName.c_str(), nullptr, nullptr) < 0) {
		printf("FFmpeg could not open input file %s\n", fName.c_str());
		return -1;
	}

	if(avformat_find_stream_info(formatCtx, nullptr) < 0) {
		printf("FFmpeg could not find stream info\n");
		return -1;
	}

	int audioStream = -1;

	for(int i = 0; i < formatCtx->nb_streams; i++) {
		if(formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStream = i;
			break;
		}
	}

	if(audioStream == -1) {
		printf("No audio stream present in %s\n", fName.c_str());
		return -1;
	}

	AVCodecParameters *params = formatCtx->streams[audioStream]->codecpar;
	const AVCodec *codec = avcodec_find_decoder(params->codec_id);
	if(!codec) {
		printf("File %s is using an unsupported codec\n", fName.c_str());
		return -1;
	}

	codecCtx = avcodec_alloc_context3(codec);
	if(!codecCtx) {
		printf("FFmpeg could not could allocate context\n");
		return -1;
	}

	if(avcodec_parameters_to_context(codecCtx, params) < 0) {
		printf("FFmpeg could not copy codec parameters to context\n");
		return -1;

	}

	if(avcodec_open2(codecCtx, codec, nullptr) < 0) {
		printf("FFmpeg could not open codec\n");
		return -1;
	}
	
	std::vector<float> decodedSamples;

	int sampleRate = params->sample_rate;
	int bitsPerSample = av_get_bytes_per_sample( (AVSampleFormat) params->format) * 8;
	int numChannels = params->ch_layout.nb_channels;
	const char* fmtName = av_get_sample_fmt_name( (AVSampleFormat) params->format);

	printf("num channels %d\n", numChannels);
	printf("sample rate %d\n", sampleRate);
	printf("bits per sample %d\n", bitsPerSample);
	printf("audio is format %s\n", fmtName);

	if(argc > 1 && std::string(argv[1]) == "-ffmpeg") {

		printf("reading file with with ffmpeg\n");
		AVPacket *packet = av_packet_alloc();
		if(!packet) {
			printf("FFmpeg could not allocate packet\n");
			return -1;
		}

		//read coded mp4a with 	ffmpeg instead of my library to check for correctness
		std::vector<std::vector<uint8_t>> aacFrames;
		while(av_read_frame(formatCtx, packet) >= 0) {
			if(packet->stream_index == audioStream) {
				std::vector<uint8_t> frame(packet->data, packet->data + packet->size);
				aacFrames.push_back(frame);
			}

			av_packet_unref(packet);
		}

		av_packet_free(&packet);

		printf("FFmpeg read %zu total frames\n", aacFrames.size());

		//decompress samples with FFmpeg
		decodedSamples = ffmpegDecompress(aacFrames, codecCtx);

		printf("we got %zu total raw samples\n", decodedSamples.size());

	}	

	else {
		//read the mp4 with my library
		mp4Reader reader(fName);
		if(reader.getStatus()) {
			printf("error reading mp4 data from file %s\n", fName.c_str()); 
		}
		
		auto myFrames = reader.getAudioSamples();
		printf("I read %zu total frames\n", myFrames.size());

		//decompress audio samples with FFmpeg
		decodedSamples = ffmpegDecompress(myFrames, codecCtx);
		printf("we got %zu total raw samples\n", decodedSamples.size());

	}	

	writeWAV(decodedSamples, sampleRate, bitsPerSample, numChannels);

	avformat_close_input(&formatCtx);
	avformat_free_context(formatCtx);	

	return 0;
}
