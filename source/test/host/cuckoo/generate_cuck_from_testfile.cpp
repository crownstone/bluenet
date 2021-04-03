#include <iostream>
#include <fstream>
#include <iomanip>

#include <set>
#include <string>
#include <sstream>
#include <random>
#include <algorithm>

#include <cuckoo_filter.h>

static auto constexpr BUFF_SIZE = 10000;
uint8_t filterBuffer[BUFF_SIZE] = {};


/**
 * Splits the stringstream on ',' and interprets each part as uint8_t
 */
std::vector<uint8_t> getData(std::stringstream& strim);

/**
 * Reads the line as cuckoo initialisation values.
 * Initializes cuckoo filter in the static buffer with these values and
 * returns pointer on success or nullptr if buffer was too small/an error occured.
 */
CuckooFilter* handleCuckoo(std::stringstream& strim);

/**
 * Add the given data to the cuckoo filter.
 *
 * If filterr == nullptr nothing happens.
 */
void handleAdd(std::vector<uint8_t>& data, CuckooFilter* filter);

/**
 * Transforms the set file from .csv into a .cpp.cuck containing a ascii csv byte array
 * that represents the cuckoo filter obtained by the operations as described by the input file.
 */
int main (int argc, char ** argv) {
	std::string filename = "cuckoo_size_128_4_len_6_20";
	std::string testdatafolder = "./";
	std::string filename_in = testdatafolder + filename + "csv.cuck";
	std::string filename_out = testdatafolder + filename + ".cpp.cuck";
//	std::string filename_expect = testdatafolder + filename + ".cuck";
	
	std::ifstream instream (filename_in);
	std::ofstream outstream (filename_out);
	
	CuckooFilter* filter = nullptr;

	// ----------- Reading the csv in -----------

	std::cout << "reading from file:" << filename_in << std::endl;
	std::string line;
	while (std::getline(instream, line)) {
		if(line == "" || line[0] == '#') {
			continue;
		}
		
		std::stringstream linestream(line);

		// extract operation into string
		std::string operation;
		std::getline(linestream, operation, ',');

		// apply operation if recognized
		if(operation == "cuckoofilter") {
			filter = handleCuckoo(linestream);
		} else if(operation == "add") {
			std::vector<uint8_t> data = getData(linestream);
			handleAdd(data, filter);
		} else {
			std::cout << "operation not recognized" << std::endl;
		}
	}
	
	// ----------- writing the filter -----------

	// write header/meta data part and buffer/fingerprint array part:
	if(filter != nullptr) {
		std::cout << "writing filter contents to file: " << filename_out << std::endl;
		for (size_t i = 0; i < filter->size(); i++) {
			outstream
				<< "0x"
				<< std::hex << std::setfill('0') << std::setw(2) << (int)filterBuffer[i]
				<< ",";
		}
	}

	// ----------- Check equality with the _expect file -----------
	// TODO.

	return 0;

} /* main() */


std::vector<uint8_t> getData(std::stringstream& strim) {
	// extract bytes into vector
	std::string word;
	std::vector<uint8_t> data;
	while (std::getline(strim, word, ',')) {
		data.push_back(strtoul(word.data(), nullptr, 16));
	}

	return data;
}

CuckooFilter* handleCuckoo(std::stringstream& strim) {
	std::string bucks_word;
	std::string nests_word;
	uint32_t bucks;
	uint32_t nests;

	std::cout << "handle cuckoofiler instance construction";

	if (std::getline(strim, bucks_word, ',') &&
		std::getline(strim, nests_word, ',')) {
		bucks = strtoul(bucks_word.data(), nullptr, 16);
		nests = strtoul(nests_word.data(), nullptr, 16);
		std::cout << "(" << bucks << "," << nests << ")" << std::endl;
	} else {
		std::cout << "failed getting params" << std::endl;
		return nullptr;
	}

	if (BUFF_SIZE < CuckooFilter::size(bucks, nests)) {
		std::cout << "buffsize too small" << std::endl;
		return nullptr;
	}

	CuckooFilter* filter = reinterpret_cast<CuckooFilter*>(filterBuffer);
	filter->init(bucks, nests);
	std::cout << "filter: " << +filter << std::endl;
	return filter;
}

void handleAdd(std::vector<uint8_t>& data, CuckooFilter* filter) {
	if (filter == nullptr){
		return;
	}

	filter->add(data.data(), data.size());
}


