#define SERIAL_VERBOSITY SERIAL_DEBUG

#include <util/cs_BitmaskVarSize.h>

#include <iostream>
#include <cassert>

using namespace std;

BitmaskVarSize bitmask;

void testClearAndSetBit(int bit, int numBits) {
	cout << "Test clearing and setting bit " << bit << "." << endl;

	cout << "Clear bit." << endl;
	assert(bitmask.clearBit(bit) == true);

	cout << "Check if isAllBitsSet is false." << endl;
	assert(bitmask.isAllBitsSet() == false);

	cout << "Check if isSet is false." << endl;
	assert(bitmask.isSet(bit) == false);

	cout << "Check if isSet is true for all other bits." << endl;
	for (int i=0; i<numBits; ++i) {
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

	cout << "Set each bit." << endl;
	for (int i=0; i<numBits; ++i) {
		bitmask.setBit(i);
	}

	cout << "Check if isAllBitsSet is true." << endl;
	assert(bitmask.isAllBitsSet() == true);

	for (int i=0; i<numBits; ++i) {
		testClearAndSetBit(i, numBits);
		cout << endl;
	}
}

int main() {
	cout << "Test BitmaskVarSize implementation" << endl;

	for (int i=1; i<256; ++i) {
		testNumberOfBits(i);
		cout << endl << endl << endl;
	}

	cout << "BitmaskVarSize SUCCESS" << endl;
	return EXIT_SUCCESS; 
}
