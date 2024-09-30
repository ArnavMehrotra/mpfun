#include "stdio.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "mp4Read.h"


uint64_t readInt32(std::ifstream& file) {
	uint8_t bytes[4];

	//i really don't get this typecast thing
	file.read(reinterpret_cast<char*>(bytes), 4);
	uint64_t result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3] & 0x00000000FFFFFFFF; 
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
	uint64_t headerSize = 8;
	uint64_t boxSize = readInt32(mp4Stream);
	std::string boxType = readString(mp4Stream, 4);
	

	//box size of 1 means extended size is being used
	//in case of extended size we need to read a 64 bit number
	if(boxSize == 1) {
		boxSize = readInt64(mp4Stream);
		//add another 8 bytes to header size
		headerSize += 8;
	}

	//data size is 8 less than metadata says because of headers
	uint64_t readSize = boxSize - headerSize; 
	
	//gets freed in Box class destructor (hopefully)
	//char* buffer = (char*) malloc(2 * readSize);
	//mp4Stream.read(buffer, readSize);

	printf("box size %llu type %s\n", , boxType.c_str());

	if(boxType == "moov") {
		int bytesRead = 0;
		while(bytesRead <= readSize) {
			bytesRead += readBox(mp4Stream);
		}
	}
	else {
		mp4Stream.seekg(readSize, std::ios::cur);
	}

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
