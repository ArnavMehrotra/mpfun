#include <iostream>
#include <fstream>
#include <memory>


class Box {
protected:
	uint64_t _size;
	std::string _type;
	char* _data;
public:
	Box(const uint64_t size, const std::string& type, char* data) {
		_type = type;
		_size = size;
		_data = data;
	};
	//add 'virtual' before ~Box if you want to make abstract class
	~Box() {
		if(_data) free(_data);
	}


	const uint64_t getSize() { return _size; };
	const std::string getType() { return _type; };

	void processMoov();
	//virtual void parseData(std::ifstream& f);
};

