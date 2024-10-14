#include <iostream>
#include "mp4Read.h"
#include "dsp.h"


extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/opt.h>
	#include <libavutil/avutil.h>
}

int main(int argc, char** argv) {
	std::string fName("whereisshe.mp4");

	//readMP4(fName);

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

	AVPacket *packet = av_packet_alloc();
	if(!packet) {
		printf("FFmpeg could not allocate packet\n");
		return -1;
	}
	
	std::vector<std::vector<uint8_t>> aacFrames;
	while(av_read_frame(formatCtx, packet) >= 0) {
		if(packet->stream_index == audioStream) {
			std::vector<uint8_t> frame(packet->data, packet->data + packet->size);
			aacFrames.push_back(frame);
			/*
			printf("read frame of size %u\n", packet->size);
			printf("first ten bytes:\n");
			for(int i = 0; i < 10 && i < aacFrames.back().size(); i++) {
				printf("%02x ", aacFrames.back()[i]);
			}
			printf("\n\n");
			*/
		}

		av_packet_unref(packet);
	}

	printf("FFmpeg read %zu total frames\n", aacFrames.size());

	av_packet_free(&packet);
	avformat_close_input(&formatCtx);
	avformat_free_context(formatCtx);
		
	mp4Reader reader(fName);
	if(reader.getStatus()) {
		printf("error reading mp4 data from file %s\n", fName.c_str()); 
	}
	

	auto myFrames = reader.getAudioSamples();
	printf("I read %zu total frames\n", myFrames.size());


	/*
	for(int i = 0; i < myFrames.size() && i < aacFrames.size(); i++) {
		if(myFrames[i].size() != aacFrames[i].size()) {
			printf("frame arrays have different size at frame %d\n", i);
			continue;
		}

		for(int j = 0; j < myFrames.size(); j++) {
			if(myFrames[i][j] != aacFrames[i][j]) {
				printf("frame arrays differ at frame %d  position %d ffmpeg: %02x me: %02x\n", i, j, aacFrames[i][j], myFrames[i][j]);
			}
		}
	}
	*/

	return 0;
}
