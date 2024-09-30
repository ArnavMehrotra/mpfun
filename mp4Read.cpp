#include "stdio.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "mp4Read.h"


uint32_t readInt32(std::ifstream& file) {
	uint8_t bytes[4];

	//i really don't get this typecast thing
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

	printf("box size %llu type %s\n", boxSize, boxType.c_str());

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
		printf("trak box is ID %u, format %s, timescale %u, and duration %llu\n", _tracks.back()._trackID, _tracks.back()._codec.c_str(), _tracks.back()._timescale, _tracks.back()._duration);
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

	return 0;
}
