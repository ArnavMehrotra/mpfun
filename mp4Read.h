#include <iostream>
#include <vector>
#include <fstream>
#include <memory>


struct Track {
	const uint32_t _trackID;
	const std::string _type;
	const uint32_t _timescale;
	const uint64_t _duration;
	const std::vector<uint32_t> _sampleSizes;
	const std::vector<uint32_t> _chunkOffsets;
};

class mp4Reader {
private:
	std::vector<Track> tracks;
	
public:
	uint64_t readBox(std::ifstream& mp4Stream);

	mp4Reader(std::ifstream &mp4Stream) {		
		//read boxes from mp4
		uint64_t bytesRead = 0;
		while(mp4Stream.peek() != EOF) {
			//printf("Box is size %llu, and type %s\n", newBox.getSize(), newBox.getType().c_str());
			bytesRead += readBox(mp4Stream);
		}
		printf("Read %llu total bytes\n", bytesRead);
		mp4Stream.close();
	}
};
