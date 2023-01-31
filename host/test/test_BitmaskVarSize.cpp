//#define SERIAL_VERBOSITY SERIAL_DEBUG

#include <util/cs_BitmaskVarSize.h>

#include <cassert>
#include <iostream>

using namespace std;

// Derived class that exposes some more functions.
class TestBitmask : public BitmaskVarSize {
public:
	uint8_t getBitmaskSizeBytes() { return getNumBytes(_numBits); }

	uint8_t* getBitmask() { return _bitmask; }
};

const uint8_t num_bytes[] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};

TestBitmask bitmask;

void testClearAndSetBit(int bit, int numBits) {
	cout << "Test clearing and setting bit " << bit << "." << endl;

	cout << "Clear bit." << endl;
	assert(bitmask.clearBit(bit) == true);

	cout << "Check if isAllBitsSet is false." << endl;
	assert(bitmask.isAllBitsSet() == false);

	cout << "Check if isSet is false." << endl;
	assert(bitmask.isSet(bit) == false);

	cout << "Check if isSet is true for all other bits." << endl;
	for (int i = 0; i < numBits; ++i) {
		if (i != bit) {
			assert(bitmask.isSet(i) == true);
		}
	}

	cout << "Set bit." << endl;
	assert(bitmask.setBit(bit) == true);

	cout << "Check if isAllBitsSet is true." << endl;
	assert(bitmask.isAllBitsSet() == true);

	cout << "Check if isSet is true." << endl;
	assert(bitmask.isSet(bit) == true);
}

void testNumberOfBits(int numBits) {
	cout << "Test with " << numBits << " bits." << endl;

	cout << "Set number of bits." << endl;
	assert(bitmask.setNumBits(numBits) == true);

	cout << "Check if all bits are cleared." << endl;
	for (int i = 0; i < numBits; ++i) {
		assert(bitmask.isSet(i) == false);
	}

	cout << "Set each bit." << endl;
	for (int i = 0; i < numBits; ++i) {
		bitmask.setBit(i);
	}

	cout << "Check if isAllBitsSet is true." << endl;
	assert(bitmask.isAllBitsSet() == true);

	for (int i = 0; i < numBits; ++i) {
		testClearAndSetBit(i, numBits);
		cout << endl;
	}
}

int main() {
	cout << "Test BitmaskVarSize implementation" << endl;

	for (int i = 0; i < 256; ++i) {
		testNumberOfBits(i);
		cout << endl << endl << endl;
	}

	for (int i = 0; i < sizeof(num_bytes); ++i) {
		cout << "Check number of allocated bytes for " << i << " bits." << endl;
		bitmask.setNumBits(i);
		cout << "  expected: " << (int)num_bytes[i] << ", got: " << (int)bitmask.getBitmaskSizeBytes() << endl;
		assert(bitmask.getBitmaskSizeBytes() == num_bytes[i]);
		cout << "Bitmask:";
		for (int j = 0; j < num_bytes[i]; ++j) {
			cout << " " << (int)bitmask.getBitmask()[j];
		}
		cout << endl;
	}
	cout << endl << endl << endl;

	cout << "BitmaskVarSize SUCCESS" << endl;
	return EXIT_SUCCESS;
}
