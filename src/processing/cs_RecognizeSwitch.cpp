
#include <processing/cs_RecognizeSwitch.h>

RecognizeSwitch::RecognizeSwitch() {
	// Skip the first N cycles
	_skipSwitchDetectionTriggers = 200;
}

/**
 * The recognizeSwitch function goes through a sequence of buffers to detect if a switch event happened in the buffer
 * in the middle. It uses the InterleavedBuffer to get the previous buffers.
 */
bool RecognizeSwitch::detect(buffer_id_t bufIndex, channel_id_t voltageChannelId) {
	bool result = false;
	if (_skipSwitchDetectionTriggers > 0) {
		_skipSwitchDetectionTriggers--;
		return result;
	}

	InterleavedBuffer & ib = InterleavedBuffer::getInstance();

	// get previous buffer indices
	buffer_id_t bufIndex_tmin0 = bufIndex;
	buffer_id_t bufIndex_tmin1 = ib.getPrevious(bufIndex);
	buffer_id_t bufIndex_tmin2 = ib.getPrevious(bufIndex_tmin1);

	int16_t sum02 = 0, sum01 = 0, sum12 = 0;
	// sum all difference between curve and 0 and 1
	for (int i = 0; i < ib.getChannelLength(); ++i) {
		sum02 += (ib.getValue(bufIndex_tmin0, voltageChannelId, i) - ib.getValue(bufIndex_tmin2, voltageChannelId, i));
		sum01 += (ib.getValue(bufIndex_tmin0, voltageChannelId, i) - ib.getValue(bufIndex_tmin1, voltageChannelId, i));
		sum12 += (ib.getValue(bufIndex_tmin1, voltageChannelId, i) - ib.getValue(bufIndex_tmin2, voltageChannelId, i));
	}

#ifdef VERBOSE_SWITCH
	LOGd("02: %i", sum02);	
	LOGd("01: %i", sum01);	
	LOGd("12: %i", sum12);	
#endif 
	// When beyond certain threshold register 
	int16_t upper_threshold = 400;
	bool cond1 = (abs(sum12) > upper_threshold);
//	bool cond2 = (sum12 > 0);
//	bool cond3 = cond2 ? (sum01 < -upper_threshold) : (sum01 > upper_threshold);
	bool cond3 = (abs(sum01) > upper_threshold);

	int16_t lower_threshold = abs(sum12) / 2;
	bool cond0 = (abs(sum02) < lower_threshold);
	if (cond0 && cond1 && cond3) {
#ifdef VERBOSE_SWITCH
		LOGd("State switch recognized");
#endif
		_skipSwitchDetectionTriggers = 50;
		result = true;
	}
	return result;
}



