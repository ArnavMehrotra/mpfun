#include <iostream>
#include <cstring>
#include <cmath>
#include "mp4Read.h"
#include "dsp.h"
#include "codec.h"
#include "record.h"

#define DURATION 1
#define BUFF_SIZE (SAMPLE_RATE* DURATION * sizeof(short))


int writeMp3(std::string fName, std::vector<char> data) {
	std::ofstream out(fName, std::ios::binary);


	if(!out.is_open()) return -1;

	out.write(data.data(), data.size());

	int bytesWritten = out.tellp();

	out.close();

	return bytesWritten;
}

//necessary FFmpeg includes are in codec.h
int writeWAV(std::string fName, std::vector<float> data, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
	std::ofstream out(fName, std::ios::binary);	

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

//TODO remove opening ffmpeg avcodec with file, pass the information from my mp4 parser instead
int main(int argc, char** argv) {	
	if(argc > 1 && !strcmp("-record", argv[1])) {	
		AudioUnit unit = setupCoreAudio();

		AudioStreamBasicDescription fmt;
		uint32_t fmtSize = sizeof(fmt);
		OSStatus status = AudioUnitGetProperty(unit, kAudioUnitProperty_StreamFormat,
												kAudioUnitScope_Output, 1, &fmt, &fmtSize);

		int channels = fmt.mChannelsPerFrame;
		printf("audio recording device has %d channels\n", channels);

		AudioOutputUnitStart(unit);

		sleep(5);

		AudioOutputUnitStop(unit);

		int16_t maxVal = 0, minVal = 0;
		for(int i = 0; i < audioBuffer.size(); i++) {
			maxVal = std::max(maxVal, audioBuffer[i]);
			minVal = std::min(minVal, audioBuffer[i]);	
		}

		printf("audio buffer has size %zu and range %d - %d\n", audioBuffer.size(), minVal, maxVal);
		
		std::vector<char> mp3Frames = lameCompress(audioBuffer, channels, SAMPLE_RATE);

		writeMp3("recording.mp3", mp3Frames);
	} else {
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


		if(argc > 1 && !strcmp(argv[1], "-ffmpeg")) {
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
			writeWAV("out.wav", decodedSamples, sampleRate, bitsPerSample, numChannels);
		} else {
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

			//writeWAV("og.wav", decodedSamples, sampleRate, bitsPerSample, numChannels);

			//apply some effects and write to wav
			filter(decodedSamples, sampleRate, 100.0f);
			chorus(decodedSamples, sampleRate, 0.002f, 1.0f, 0.01f, 0.5f);
			reverb(decodedSamples, sampleRate, 0.5f, 0.5f);

			writeWAV("filtered.wav", decodedSamples, sampleRate, bitsPerSample, numChannels);

			//compress and write to mp3
			//prepare pcm data by scaling and converting to pcm int
			std::vector<int16_t> convertedSamples = convertFloat(decodedSamples);

			//mp3Compress(decodedSamples);
			std::vector<char> mp3Bytes = lameCompress(convertedSamples, numChannels, sampleRate);


			std::string mp3Name = "out.mp3";
			int mp3Size = writeMp3("out.mp3", mp3Bytes);
			printf("wrote %d bytes to %s\n", mp3Size, mp3Name.c_str());
		}

		avformat_close_input(&formatCtx);
		avformat_free_context(formatCtx);	
	}

	return 0;
}
