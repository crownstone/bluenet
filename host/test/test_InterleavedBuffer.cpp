
//#define SERIAL_VERBOSITY SERIAL_DEBUG

#ifndef HOST_TARGET
#error "This should only be compiled for the host"
#endif

#include <structs/buffer/cs_AdcBuffer.h>
#include <structs/buffer/cs_CircularBuffer.h>

#include <iostream>

using namespace std;

const uint8_t sin_table[]= {0,0,1,2,4,6,9,12,16,20,24,29,35,40,46,53,59,66,73,81,88,96,104,112,119,128,136,143,151,159,167,174,182,189,196,202,209,215,220,226,231,235,239,243,246,249,251,253,254,255,255,255,254,253,251,249,246,243,239,235,231,226,220,215,209,202,196,189,182,174,167,159,151,143,136,128,119,112,104,96,88,81,73,66,59,53,46,40,35,29,24,20,16,12,9,6,4,2,1,0};

#define NUM_BUFFERS 4

typedef adc_sample_value_t value_t;

int main() {

	cout << "Test InterleavedBuffer implementation" << endl;

	AdcBuffer & buffer = AdcBuffer::getInstance();
	value_t buf[4][200];

	cout << "Fill buffers with current and voltage data" << endl;

	for (int i = 0; i < NUM_BUFFERS; ++i) {
		for (int j = 0; j < 100; ++j) {
			buf[i][j*2] = sin_table[j];
			buf[i][j*2+1] = 400;
		}
	}

	cout << "Set buffers" << endl;
	for (int i = 0; i < NUM_BUFFERS; ++i) {
		buffer.setBuffer(i, buf[i]);
	}

	cout << "Retrieve values from buffers" << endl;

	int cb_size = 10;
	CircularBuffer<value_t> *circBuffer = new CircularBuffer<value_t>(cb_size);
	circBuffer->init();

	int channel = 100;
	int32_t vdifftot0 = 0, vdifftot1 = 0;
	value_t value0, value1, value2, vdiff0, vdiff1, last;
	adc_buffer_id_t buffer_id = 0;

	bool full = false;
	adc_channel_id_t channel_id = 0;
//	value0 = buffer.getValue(buffer_id, channel_id, 200);
	for (int i = -channel + 1; i < 0; ++i) {
		value0 = buffer.getValue(buffer_id, channel_id, i);
		value1 = buffer.getValue(buffer_id, channel_id, i + channel);
		value2 = buffer.getValue(buffer_id, channel_id, i + 2 * channel);

		vdiff0 = abs(value0 - value1);
		vdiff1 = abs(value0 - value2);

		/*
		if (circBuffer->full()) {
			full = true;
		}
		if (full) {
			last = circBuffer->pop();
			vdifftot0 -= last;
		}*/
		//cout << "Add to circular buffer: " << vdiff << endl;
//		circBuffer->push(vdiff);

		vdifftot0 += vdiff0;
		vdifftot1 += vdiff1;

	}
	cout << "Differences (total sum): " << vdifftot0 << endl;
	cout << "Differences (total sum): " << vdifftot1 << endl;

	delete circBuffer;

	return EXIT_SUCCESS; 
}
