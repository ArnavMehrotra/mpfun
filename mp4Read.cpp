#include "stdio.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include "mp4Read.h"


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

uint64_t mp4Reader::readBox(std::ifstream &mp4Stream) {
	uint64_t boxSize = (uint64_t) readInt32(mp4Stream);
	std::string boxType = readString(mp4Stream, 4);
	uint64_t bytesRead = 8;
	

	//box size of 1 means extended size is being used
	//in case of extended size we need to read a 64 bit number
	if(boxSize == 1) {
		boxSize = readInt64(mp4Stream);
		bytesRead += 8;
	}

	//data size is 8 less than metadata says because of headers
	
	//gets freed in Box class destructor (hopefully)
	//char* buffer = (char*) malloc(2 * readSize);
	//mp4Stream.read(buffer, readSize);

	//printf("box size %llu type %s\n", boxSize, boxType.c_str());

	if(boxType == "moov") {
		while(bytesRead < boxSize) {
			bytesRead += readBox(mp4Stream);
		}
	}
	else if(boxType == "trak") {
		Track newTrack;
		_tracks.push_back(newTrack);
		while(bytesRead < boxSize) {
			bytesRead += readBox(mp4Stream);
		}

		size_t sampleCount  = _tracks.back()._sampleSizes.size();
		size_t cOffsetCount = _tracks.back()._chunkOffsets.size();
		size_t ttsCount = _tracks.back()._timeToSample.size();
		size_t spcCount = _tracks.back()._samplesPerChunk.size();

		if(sampleCount == ttsCount) printf("read time to samples for trak %u\n", _tracks.back()._trackID);
		else printf("TTS COUNT AND SAMPLE COUNT DIFFER FOR TRAK %u : tts count %zu sample count %zu\n", _tracks.back()._trackID, ttsCount, sampleCount);

		if(cOffsetCount == spcCount) printf("read samples per chunk for trak %u\n", _tracks.back()._trackID);
		else printf("SAMPLES PER CHUNK AND CHUNK OFFSETS DIFFER FOR TRAK %u : spc %zu chunk offsets %zu\n", _tracks.back()._trackID, spcCount, cOffsetCount);
		
		
		/*
		printf("trak box is ID %u, format %s, timescale %u, and duration %llu with %zu samples, %zu chunk offsets %zu tts entries, %zu spc entries\n",
		_tracks.back()._trackID, _tracks.back()._codec.c_str(), _tracks.back()._timescale, _tracks.back()._duration,
		sampleCount, cOffsetCount, ttsCount, spcCount);
		*/
	}
	else if(boxType == "tkhd") {
		uint8_t version = mp4Stream.get();
		mp4Stream.seekg(11, std::ios::cur);
		bytesRead += 12;

		uint32_t trackID = readInt32(mp4Stream);
		mp4Stream.seekg(4, std::ios::cur);
		bytesRead += 8;

		uint32_t duration = readInt32(mp4Stream);
		bytesRead += 4;

		_tracks.back()._trackID = trackID;
		_tracks.back()._duration = (uint64_t) duration & 0x00000000FFFFFFFF;
	}
	else if(boxType == "mdia") {
		while(bytesRead < boxSize) {
			bytesRead += readBox(mp4Stream);
		}
	}
	else if(boxType == "mdhd") {	
		uint8_t version = mp4Stream.get();
		bytesRead++;
		//printf("mdhd version %u\n", version);
		if(version == 0) {	
			mp4Stream.seekg(11, std::ios::cur);
			_tracks.back()._timescale = readInt32(mp4Stream);
			bytesRead += 15;
		}
	}
	else if(boxType == "minf") {
		while(bytesRead < boxSize) {
			bytesRead += readBox(mp4Stream);
		}
	}
	else if(boxType == "stbl") {
		while(bytesRead < boxSize) {
			bytesRead += readBox(mp4Stream);
		}
	}
	else if(boxType == "stsd") {	
		mp4Stream.seekg(12, std::ios::cur);
		_tracks.back()._codec = readString(mp4Stream, 4);
		bytesRead += 16;
	}
	else if(boxType == "stsz") {
		mp4Stream.seekg(4, std::ios::cur);
		uint32_t sampleSize = readInt32(mp4Stream);
		uint32_t sampleCount = readInt32(mp4Stream);
		bytesRead += 12;

		std::vector<uint32_t>& sampleSizes = _tracks.back()._sampleSizes;

		if(sampleSize == 0) {
			while(bytesRead < boxSize) {
				sampleSizes.push_back(readInt32(mp4Stream));
				bytesRead += 4;
			}
		}
		else {
			sampleSizes.insert(sampleSizes.end(), sampleCount, sampleSize);
		}

		if(sampleSizes.size() == sampleCount) printf("read sample count for trak %u\n", _tracks.back()._trackID);
		else printf("SAMPLE COUNT AND SAMPLE ARRAY DIFFER FOR TRAK %u : sample count %u sample array %zu\n", _tracks.back()._trackID, sampleCount, sampleSizes.size());	
	}
	else if(boxType == "stco") {	
		mp4Stream.seekg(4, std::ios::cur);
		uint32_t entryCount = readInt32(mp4Stream);
		bytesRead += 8;
		std::vector<uint32_t>& chunkOffsets = _tracks.back()._chunkOffsets;
		while(bytesRead < boxSize) {
			chunkOffsets.push_back(readInt32(mp4Stream));
			bytesRead += 4;
		}

		if(chunkOffsets.size() == entryCount) printf("read chunk offsets for trak %u\n", _tracks.back()._trackID);
		else printf("CHUNK OFFSET COUNT AND CHUNK OFFSET ARRAY DIFFER FOR TRAK %u : chunk offset count %u, chunk offset array %zu\n", _tracks.back()._trackID, entryCount, chunkOffsets.size());
	}
	else if(boxType == "stsc") {
		mp4Stream.seekg(4, std::ios::cur);
		uint32_t entryCount = readInt32(mp4Stream);
		bytesRead += 8;
		uint32_t prevChunk = 0;
		while(bytesRead < boxSize) {
			uint32_t firstChunk = readInt32(mp4Stream);
			uint32_t samplesPerChunk = readInt32(mp4Stream);
			uint32_t sampleDescriptionIndex = readInt32(mp4Stream);
			bytesRead += 12;

			for(uint32_t chunk = prevChunk; chunk < firstChunk; chunk++) {
				_tracks.back()._samplesPerChunk.push_back(samplesPerChunk);
			}
			
			prevChunk = firstChunk;
		}
	}
	else if(boxType == "stts") {
		mp4Stream.seekg(4, std::ios::cur);
		uint32_t entryCount = readInt32(mp4Stream);
		bytesRead += 8;
		std::vector<uint32_t>& tts = _tracks.back()._timeToSample;
		while(bytesRead < boxSize) {
			uint32_t sampleCount = readInt32(mp4Stream);	
			uint32_t sampleDelta = readInt32(mp4Stream);
			bytesRead += 8;

			for(int i = 0; i < sampleCount; i++) {
				tts.push_back(sampleDelta);
			}
		}	
	}

	mp4Stream.seekg(boxSize - bytesRead, std::ios::cur);

	return boxSize;
}

int main(int argc, char** argv) {
	std::string fName("whereisshe.mp4");
	std::ifstream mp4Stream(fName, std::ios::binary);

	if(!mp4Stream.is_open()) {
		printf("Could not open file %s\n", fName.c_str());
		return -1;
	}
	mp4Reader reader(mp4Stream);
	
	mp4Stream.close();
	return 0;
}
