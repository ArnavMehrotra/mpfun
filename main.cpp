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
	avcodec_version();
	std::string fName("whereisshe.mp4");
	readMP4(fName);
	hi();
	return 0;
}
