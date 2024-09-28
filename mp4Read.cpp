#include "stdio.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "box.h"


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
	return ((uint64_t)bytes[0] << 56) | ((uint64_t)bytes[1] << 48) | ((uint64_t)bytes[2] << 40) |
	       ((uint64_t)bytes[3] << 32) | ((uint64_t)bytes[4] << 24) | ((uint64_t)bytes[5] << 16) |
		((uint64_t)bytes[6] << 8) | (uint64_t)bytes[7];
}

std::string readString(std::ifstream& file, size_t length) {
	std::vector<char> buffer(length);
	file.read(buffer.data(), length);
	return std::string(buffer.begin(), buffer.end());
}

Box readBox(std::ifstream& mp4Stream) {
	uint64_t boxSize = readInt32(mp4Stream);
	std::string boxType = readString(mp4Stream, 4);

	//data size is 8 less than metadata says because of headers
	uint64_t readSize = boxSize - 8; 
	

	//box size of 1 means extended size is being used
	//in case of extended size we need to read a 64 bit number
	if(boxSize == 1) {
		boxSize = readInt64(mp4Stream);
		//subtract another 8 bytes from the data
		readSize -= 8;
	}
	
	//gets freed in Box class destructor (hopefully)
	char* buffer = (char*) malloc(2 * readSize);
	mp4Stream.read(buffer, readSize);

	//mp4Stream.seekg(boxSize - (8 + offset), std::ios::cur);

	//give Box read size because that's the size of data
	return Box(readSize, boxType, buffer);
}

void Box::processMoov() {
	return;
}

int main(int argc, char** argv) {
	std::string fName("whereisshe.mp4");
	std::ifstream mp4Stream(fName, std::ios::binary);

	if(!mp4Stream.is_open()) {
		printf("Could not open file %s\n", fName.c_str());
		return -1;
	}
	
	//read boxes from mp4
	while(mp4Stream.peek() != EOF) {
		Box newBox = readBox(mp4Stream);
		printf("Box is size %llu, and type %s\n", newBox.getSize(), newBox.getType().c_str());
	}

	mp4Stream.close();

	return 0;
}
