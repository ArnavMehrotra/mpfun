#ifndef MP4READ_H
#define MP4READ_H

#include <iostream>
#include <vector>
#include <fstream>
#include <memory>


int readMP4(std::string fName);

struct Track {
	uint32_t _trackID;
	uint32_t _timescale;
	uint64_t _duration;
		
	std::string _codec;

	std::vector<uint32_t> _sampleSizes;
	std::vector<uint32_t> _chunkOffsets;
	std::vector<std::pair<uint32_t, uint32_t>> _samplesPerChunk;
	std::vector<uint32_t> _timeToSample;
	//std::vector<uint32_t> _compositionOffset;
	//std::vector<uint32_t> _sampleToChunkIndex;
};

class mp4Reader {
private:
	std::ifstream _stream;
	std::vector<Track> _tracks;
	std::vector<uint8_t> _mediaData;
	std::vector<std::vector<std::vector<uint8_t>>> _samples;
	int _status = 0;
	
public:

	uint64_t readBox();
	void extractSamples(int trackIndex);


	mp4Reader(std::string fName) {		
		_stream = std::ifstream(fName, std::ios::binary);

		if(!_stream.is_open()) {
			printf("Could not open file %s\n", fName.c_str());
			_status = 1;
			return;
		}
		_stream.seekg(0, std::ios::beg);
		//read boxes from mp4
		uint64_t bytesRead = 0;
		while(_stream.peek() != EOF) {
			//printf("Box is size %llu, and type %s\n", newBox.getSize(), newBox.getType().c_str());
			bytesRead += readBox();
		}

		_stream.seekg(0, std::ios::end);
		uint64_t fileBytes = _stream.tellg();
		if(fileBytes == bytesRead) printf("read bytes and file bytes match\n");
		else {
			printf("Read %llu total bytes compared to %llu bytes in file", bytesRead, fileBytes);
			_status = 1;
			return;
		}

		for(int i = 0; i < _tracks.size(); i++) {
			extractSamples(i);
		}
	}

	~mp4Reader() {
		_stream.close();
	}
	
	int getStatus() { return _status; }

	std::vector<std::vector<uint8_t>> getAudioSamples() {
		for(int i = 0; i < _tracks.size(); i++) {
			if(_tracks[i]._codec == "mp4a") {
				return _samples[i];
			}
		}

		return std::vector<std::vector<uint8_t>>();

	}

};

#endif