#include <iostream>
#include <vector>
#include <fstream>
#include <memory>


struct Track {
	uint32_t _trackID;
	uint32_t _timescale;
	uint64_t _duration;
		
	std::string _codec;

	std::vector<uint32_t> _sampleSizes;
	std::vector<uint32_t> _chunkOffsets;
	std::vector<uint32_t> _samplesPerChunk;
	std::vector<uint32_t> _timeToSample;
	//std::vector<uint32_t> _compositionOffset;
	//std::vector<uint32_t> _sampleToChunkIndex;
};

class mp4Reader {
private:
	std::vector<Track> _tracks;
	std::vector<uint8_t> _mediaData;
	
public:
	uint64_t readBox(std::ifstream& mp4Stream);

	mp4Reader(std::ifstream &mp4Stream) {		
		//read boxes from mp4
		uint64_t bytesRead = 0;
		while(mp4Stream.peek() != EOF) {
			//printf("Box is size %llu, and type %s\n", newBox.getSize(), newBox.getType().c_str());
			bytesRead += readBox(mp4Stream);
		}

		mp4Stream.seekg(0, std::ios::end);
		uint64_t fileBytes = mp4Stream.tellg();
		if(fileBytes == bytesRead) printf("read bytes and file bytes match\n");
		else printf("Read %llu total bytes compared to %llu bytes in file", bytesRead, fileBytes);
	}
};
