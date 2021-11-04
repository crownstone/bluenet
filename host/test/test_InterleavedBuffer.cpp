
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
#define BUFFER_SIZE 200

int main() {

	cout << "Test InterleavedBuffer implementation" << endl;

	AdcBuffer & buffer = AdcBuffer::getInstance();
	adc_buffer_t* buf[4] = { nullptr };
	for (int i = 0; i < NUM_BUFFERS; ++i) {
		buf[i] = new adc_buffer_t();
		buf[i]->samples = new adc_sample_value_t[BUFFER_SIZE];
	}

	cout << "Fill buffers with current and voltage data" << endl;

	for (int i = 0; i < NUM_BUFFERS; ++i) {
		for (int j = 0; j < 100; ++j) {
			buf[i]->samples[j*2] = sin_table[j];
			buf[i]->samples[j*2+1] = 400;
		}
	}

	cout << "Set buffers" << endl;
	for (int i = 0; i < NUM_BUFFERS; ++i) {
		buffer.setBuffer(i, buf[i]);
	}

	cout << "Create circular buffer" << endl;

	CircularBuffer<adc_buffer_id_t> _bufferQueue(NUM_BUFFERS);
	
	cout << "Buffer created of capacity " << _bufferQueue.capacity() << endl;

	if (!_bufferQueue.init()) {
		return ERR_NO_SPACE;
	}
	
	cout << "Buffer contents size is " << _bufferQueue.size() << endl;

	//adc_buffer_id_t bufCount = AdcBuffer::getInstance().getBufferCount();
	adc_buffer_id_t bufCount = NUM_BUFFERS;
	for (adc_buffer_id_t id = 0; id < bufCount; ++id) {
		_bufferQueue.push(id);
	}
	
	cout << "Buffer contents size is " << _bufferQueue.size() << endl;

	// Removed the rest about handling the buffers
	// Next development step is to create one large intermediate buffer with floats
	// This is nice to test on the host here
	// Create separate buffers with interleaved sine wave data from current and voltage
	// Then check if the overall buffer is properly glued together

	return EXIT_SUCCESS;
}
