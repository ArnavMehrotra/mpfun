#include "stdio.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include "mp4Read.h"



std::vector<uint8_t> readBytes(std::ifstream& file, std::streamsize n) {
	std::vector<uint8_t> result(n);
	file.read(reinterpret_cast<char*>(result.data()), n);
	return result;
}

uint32_t readInt32(std::ifstream& file) {
	uint8_t bytes[4];

	//i don't really get this typecast thing
	file.read(reinterpret_cast<char*>(bytes), 4);
	uint64_t result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]; 
	return result;
}

uint64_t readInt64(std::ifstream& file) {
	uint8_t bytes[8];
	file.read(reinterpret_cast<char*>(bytes), 8);
	uint64_t result = ((uint64_t)bytes[0] << 56) | ((uint64_t)bytes[1] << 48) | ((uint64_t)bytes[2] << 40) |
	       ((uint64_t)bytes[3] << 32) | ((uint64_t)bytes[4] << 24) | ((uint64_t)bytes[5] << 16) |
		((uint64_t)bytes[6] << 8) | (uint64_t)bytes[7];

	return result;
}

std::string readString(std::ifstream& file, size_t length) {
	std::vector<char> buffer(length);
	file.read(buffer.data(), length);
	return std::string(buffer.begin(), buffer.end());
}

uint64_t mp4Reader::readBox() {
	uint64_t boxSize = (uint64_t) readInt32(_stream);
	std::string boxType = readString(_stream, 4);
	uint64_t bytesRead = 8;
	

	//box size of 1 means extended size is being used (mdat box)
	//in case of extended size we need to read a 64 bit number
	if(boxSize == 1) {
		boxSize = readInt64(_stream);
		bytesRead += 8;
	}

	//data size is 8 less than metadata says because of headers
	
	//gets freed in Box class destructor (hopefully)
	//char* buffer = (char*) malloc(2 * readSize);
	//_stream.read(buffer, readSize);

	//printf("box size %llu type %s\n", boxSize, boxType.c_str());

	if(boxType == "moov") {
		while(bytesRead < boxSize) {
			bytesRead += readBox();
		}
	} else if(boxType == "trak") {
		Track newTrack;
		_tracks.push_back(newTrack);

		while(bytesRead < boxSize) {
			bytesRead += readBox();
		}

		size_t sampleCount  = _tracks.back()._sampleSizes.size();
		size_t cOffsetCount = _tracks.back()._chunkOffsets.size();
		size_t ttsCount = _tracks.back()._timeToSample.size();
		size_t spcCount = _tracks.back()._samplesPerChunk.size();
		std::string fmt = _tracks.back()._codec;
		
		if(sampleCount == ttsCount) printf("read time to samples for trak %u\n", _tracks.back()._trackID);
		else printf("TTS COUNT AND SAMPLE COUNT DIFFER FOR TRAK %u : tts count %zu sample count %zu\n", _tracks.back()._trackID, ttsCount, sampleCount);

		if(cOffsetCount == spcCount) printf("read samples per chunk for trak %u\n", _tracks.back()._trackID);
		else printf("SAMPLES PER CHUNK AND CHUNK OFFSETS DIFFER FOR TRAK %u : spc %zu chunk offsets %zu\n", _tracks.back()._trackID, spcCount, cOffsetCount);

		printf("trak %u is format %s\n", _tracks.back()._trackID, fmt.c_str());
		
		/*
		printf("trak box is ID %u, format %s, timescale %u, and duration %llu with %zu samples, %zu chunk offsets %zu tts entries, %zu spc entries\n",
		_tracks.back()._trackID, _tracks.back()._codec.c_str(), _tracks.back()._timescale, _tracks.back()._duration,
		sampleCount, cOffsetCount, ttsCount, spcCount);
		*/
	} else if(boxType == "tkhd") {
		uint8_t version = _stream.get();
		_stream.seekg(11, std::ios::cur);
		bytesRead += 12;

		uint32_t trackID = readInt32(_stream);
		_stream.seekg(4, std::ios::cur);
		bytesRead += 8;

		uint32_t duration = readInt32(_stream);
		bytesRead += 4;

		_tracks.back()._trackID = trackID;
		_tracks.back()._duration = (uint64_t) duration & 0x00000000FFFFFFFF;
	} else if(boxType == "mdia") {
		while(bytesRead < boxSize) {
			bytesRead += readBox();
		}
	} else if(boxType == "mdhd") {
		uint8_t version = _stream.get();
		bytesRead++;
		//printf("mdhd version %u\n", version);
		if(version == 0) {	
			_stream.seekg(11, std::ios::cur);
			_tracks.back()._timescale = readInt32(_stream);
			bytesRead += 15;
		}
	} else if(boxType == "minf") {
		while(bytesRead < boxSize) {
			bytesRead += readBox();
		}
	} else if(boxType == "stbl") {
		while(bytesRead < boxSize) {
			bytesRead += readBox();
		}
	} else if(boxType == "stsd") {	
		_stream.seekg(12, std::ios::cur);
		_tracks.back()._codec = readString(_stream, 4);
		bytesRead += 16;
	} else if(boxType == "stsz") {
		_stream.seekg(4, std::ios::cur);
		uint32_t sampleSize = readInt32(_stream);
		uint32_t sampleCount = readInt32(_stream);
		bytesRead += 12;

		std::vector<uint32_t>& sampleSizes = _tracks.back()._sampleSizes;

		if(sampleSize == 0) {
			while(bytesRead < boxSize) {
				sampleSizes.push_back(readInt32(_stream));
				bytesRead += 4;
			}
		}
		else {
			sampleSizes.insert(sampleSizes.end(), sampleCount, sampleSize);
		}

		if(sampleSizes.size() == sampleCount) printf("read sample count for trak %u\n", _tracks.back()._trackID);
		else printf("SAMPLE COUNT AND SAMPLE ARRAY DIFFER FOR TRAK %u : sample count %u sample array %zu\n", _tracks.back()._trackID, sampleCount, sampleSizes.size());	
	} else if(boxType == "stco") {	
		_stream.seekg(4, std::ios::cur);
		uint32_t entryCount = readInt32(_stream);
		bytesRead += 8;
		std::vector<uint32_t>& chunkOffsets = _tracks.back()._chunkOffsets;
		while(bytesRead < boxSize) {
			chunkOffsets.push_back(readInt32(_stream));
			bytesRead += 4;
		}

		if(chunkOffsets.size() == entryCount) printf("read chunk offsets for trak %u\n", _tracks.back()._trackID);
		else printf("CHUNK OFFSET COUNT AND CHUNK OFFSET ARRAY DIFFER FOR TRAK %u : chunk offset count %u, chunk offset array %zu\n", _tracks.back()._trackID, entryCount, chunkOffsets.size());
	} else if(boxType == "stsc") {
		_stream.seekg(4, std::ios::cur);
		uint32_t entryCount = readInt32(_stream);
		bytesRead += 8;
		uint32_t prevChunk = 0;
		while(bytesRead < boxSize) {
			uint32_t firstChunk = readInt32(_stream);
			uint32_t samplesPerChunk = readInt32(_stream);
			uint32_t sampleDescriptionIndex = readInt32(_stream);
			bytesRead += 12;

			for(uint32_t chunk = prevChunk; chunk < firstChunk; chunk++) {
				_tracks.back()._samplesPerChunk.push_back(samplesPerChunk);
			}
			
			prevChunk = firstChunk;
		}
	} else if(boxType == "stts") {
		_stream.seekg(4, std::ios::cur);
		uint32_t entryCount = readInt32(_stream);
		bytesRead += 8;
		std::vector<uint32_t>& tts = _tracks.back()._timeToSample;
		while(bytesRead < boxSize) {
			uint32_t sampleCount = readInt32(_stream);
			uint32_t sampleDelta = readInt32(_stream);
			bytesRead += 8;

			for(int i = 0; i < sampleCount; i++) {
				tts.push_back(sampleDelta);
			}
		}
	}
	

	_stream.seekg(boxSize - bytesRead, std::ios::cur);
	extractSamples();

	return boxSize;
}

std::vector<std::vector<uint8_t>> mp4Reader::extractSamples() {
	
	return std::vector<std::vector<uint8_t>>();
}

int main(int argc, char** argv) {
	std::string fName("whereisshe.mp4");
	mp4Reader reader(fName);
	if(reader.getStatus()) {
		printf("error reading mp4 data from file %s\n", fName.c_str()); 
	}
	
	return 0;
}
