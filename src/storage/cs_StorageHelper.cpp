/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 5, 2017
 * License: LGPLv3+
 */
#include <storage/cs_StorageHelper.h>

#include <cfg/cs_Config.h>
#include <climits>

////////////////////////////////////////////////////////////////////////////////////////
//! helper functions
////////////////////////////////////////////////////////////////////////////////////////

pstorage_size_t StorageHelper::getOffset(ps_storage_base_t* storage, uint8_t* var) {
	uint32_t p_storage = (uint32_t)storage;
	uint32_t p_var = (uint32_t)var;
	pstorage_size_t offset = p_var - p_storage;

#ifdef PRINT_ITEMS
	LOGd("p_storage: %p", p_storage);
	LOGd("var: %p", p_var);
	LOGd("offset: %d", offset);
#endif

	return offset;
}

void StorageHelper::setString(std::string value, char* target) {
	setString(value.c_str(), value.length(), target);
}

void StorageHelper::setString(const char* value, uint16_t length, char* target) {
	if (length <= MAX_STRING_STORAGE_SIZE) {
		memset(target, 0, MAX_STRING_STORAGE_SIZE+1);
		memcpy(target, value, length);
	} else {
		LOGe("string too long!!");
	}
}

//// helper function to get std::string from char array, or default value
//// if the value read is empty, unassigned (filled with FF) or too long
//void StorageHelper::getString(char* value, std::string& target, std::string default_value) {
//
//#ifdef PRINT_ITEMS
//	_log(SERIAL_INFO, "get string (raw): \r\n");
//	BLEutil::printArray((uint8_t*)value, MAX_STRING_STORAGE_SIZE+1);
//#endif
//
//	target = std::string(value);
//	// if the last char is equal to FF that means the memory
//	// is new and has not yet been written to, so we use the
//	// default value. same if the stored value is an empty string
//	if (target == "" || value[MAX_STRING_STORAGE_SIZE] == 0xFF) {
//#ifdef PRINT_ITEMS
//		LOGd(FMT_USE_DEFAULT_VALUE);
//#endif
//		target = default_value;
//	} else {
//#ifdef PRINT_ITEMS
//		LOGd("found stored value: %s", target.c_str());
//#endif
//	}
//}

// helper function to get std::string from char array, or default value
// if the value read is empty, unassigned (filled with FF) or too long
void StorageHelper::getString(char* value, char* target, char* default_value, uint16_t& size) {

#ifdef PRINT_ITEMS
	_log(SERIAL_INFO, "get string (raw): \r\n");
	BLEutil::printArray((uint8_t*)value, MAX_STRING_STORAGE_SIZE+1);
#endif

	std::string stringValue(value);
	// if the last char is equal to FF that means the memory
	// is new and has not yet been written to, so we use the
	// default value. same if the stored value is an empty string
	if (stringValue == "" || value[MAX_STRING_STORAGE_SIZE] == 0xFF) {
		std::string stringDefault(default_value);
#ifdef PRINT_ITEMS
		LOGd("use default value: %s, len: %d", stringDefault.c_str(), stringDefault.length());
#endif
		memcpy(target, stringDefault.c_str(), stringDefault.length());
		size = stringDefault.length();
	} else {
#ifdef PRINT_ITEMS
		LOGd("found stored value: %s, len: %d", stringValue.c_str(), stringValue.length());
#endif
		memcpy(target, stringValue.c_str(), stringValue.length());
		size = stringValue.length();
	}
}

void StorageHelper::setUint8(uint8_t value, uint32_t& target) {
	target = value;
}

void StorageHelper::getUint8(uint32_t value, uint8_t* target, uint8_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGd(FMT_RAW_VALUE, tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if last byte is FF which means that memory is unassigned
	// and value has to be ignored
	if (value == UINT32_MAX) {
#ifdef PRINT_ITEMS
		LOGd(FMT_USE_DEFAULT_VALUE);
#endif
		*target = default_value;
	} else {
		*target = value;
#ifdef PRINT_ITEMS
		LOGd(FMT_FOUND_STORED_VALUE, *target);
#endif
	}
}

void StorageHelper::setInt8(int8_t value, int32_t& target) {
	target = value;
//	target &= 0x000000FF;
}

void StorageHelper::getInt8(int32_t value, int8_t* target, int8_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGd(FMT_RAW_VALUE, tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if last byte is FF which means that memory is unassigned
	// and value has to be ignored
	if ((uint32_t)value == UINT32_MAX) {
#ifdef PRINT_ITEMS
		LOGd(FMT_USE_DEFAULT_VALUE);
#endif
		*target = default_value;
	} else {
		*target = value;
#ifdef PRINT_ITEMS
		LOGd(FMT_FOUND_STORED_VALUE, *target);
#endif
	}
}

void StorageHelper::setUint16(uint16_t value, uint32_t& target) {
	target = value;
}

void StorageHelper::getUint16(uint32_t value, uint16_t* target, uint16_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGd(FMT_RAW_VALUE, tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if last byte is FF which means that memory is unassigned
	// and value has to be ignored
	if (value == UINT32_MAX) {
#ifdef PRINT_ITEMS
		LOGd(FMT_USE_DEFAULT_VALUE);
#endif
		*target = default_value;
	} else {
		*target = value;

#ifdef PRINT_ITEMS
		LOGd(FMT_FOUND_STORED_VALUE, *target);
#endif
	}
}



void StorageHelper::setInt16(int16_t value, int32_t& target) {
	target = value;
}

void StorageHelper::getInt16(int32_t value, int16_t* target, int16_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGd(FMT_RAW_VALUE, tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	if ((uint32_t)value == UINT32_MAX) {
#ifdef PRINT_ITEMS
		LOGd(FMT_USE_DEFAULT_VALUE);
#endif
		*target = default_value;
	} else {
		*target = value;

#ifdef PRINT_ITEMS
		LOGd(FMT_FOUND_STORED_VALUE, *target);
#endif
	}
}

void StorageHelper::setUint32(uint32_t value, uint32_t& target) {
	if (value == UINT32_MAX) {
		LOGe("value %d too big, can only write max %d", value, INT_MAX-1);
	} else {
		target = value;
	}
}

void StorageHelper::getUint32(uint32_t value, uint32_t* target, uint32_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGd(FMT_RAW_VALUE, tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	// check if value is equal to INT_MAX (FFFFFFFF) which means that memory is
	// unassigned and value has to be ignored
	if (value == UINT32_MAX) {
#ifdef PRINT_ITEMS
		LOGd(FMT_USE_DEFAULT_VALUE);
#endif
		*target = default_value;
	} else {
		*target = value;
#ifdef PRINT_ITEMS
		LOGd(FMT_FOUND_STORED_VALUE, *target);
#endif
	}
}


void StorageHelper::setInt32(int32_t value, int32_t& target) {
	if ((uint32_t)value == UINT32_MAX) {
		LOGe("value %d too big, can only write max %d", value, INT_MAX-1);
	} else {
		target = value;
	}
}

void StorageHelper::getInt32(int32_t value, int32_t* target, int32_t default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	LOGd(FMT_RAW_VALUE, tmp[3], tmp[2], tmp[1], tmp[0]);
#endif

	if ((uint32_t)value == UINT32_MAX) {
#ifdef PRINT_ITEMS
		LOGd(FMT_USE_DEFAULT_VALUE);
#endif
		*target = default_value;
	} else {
		*target = value;

#ifdef PRINT_ITEMS
		LOGd(FMT_FOUND_STORED_VALUE, *target);
#endif
	}
}

//void StorageHelper::setDouble(double value, double& target) {
//	if (value == DBL_MAX) {
//		LOGe("value %d too big", value);
//	} else {
//		target = value;
//	}
//}
//
//void StorageHelper::getDouble(double value, double* target, double default_value) {
//
//#ifdef PRINT_ITEMS
//	uint8_t* tmp = (uint8_t*)&value;
//	log(SERIAL_DEBUG, "raw value:", tmp[3], tmp[2], tmp[1], tmp[0]);
//	BLEutil::printArray(tmp, sizeof(double));
//#endif
//
//	// check if value is equal to DBL_MAX (FFFFFFFF) which means that memory is
//	// unassigned and value has to be ignored
//	if (value == DBL_MAX) {
//#ifdef PRINT_ITEMS
//		LOGd(FMT_USE_DEFAULT_VALUE);
//#endif
//		*target = default_value;
//	} else {
//		*target = value;
//#ifdef PRINT_ITEMS
//		LOGd("found stored value: %f", *target);
//#endif
//	}
//}


void StorageHelper::setFloat(float value, float& target) {
	uint8_t* byteP = (uint8_t*)&value;
	if (byteP[0] == 0xFF && byteP[1] == 0xFF && byteP[2] == 0xFF && byteP[3] == 0xFF) {
		LOGe("Invalid value");
	} else {
		target = value;
	}
}

void StorageHelper::getFloat(float value, float* target, float default_value) {

#ifdef PRINT_ITEMS
	uint8_t* tmp = (uint8_t*)&value;
	log(SERIAL_DEBUG, "raw value:", tmp[3], tmp[2], tmp[1], tmp[0]);
	BLEutil::printArray(tmp, sizeof(float));
#endif

	// check if value is equal to (0xFFFFFFFF) which means that memory is
	// unassigned and value has to be ignored
//	if (*(uint32_t*)&value == UINT32_MAX) {
	uint8_t* byteP = (uint8_t*)&value;
	if (byteP[0] == 0xFF && byteP[1] == 0xFF && byteP[2] == 0xFF && byteP[3] == 0xFF) {
//	if ((uint32_t)value == UINT32_MAX) { // This does not work!
#ifdef PRINT_ITEMS
		LOGd(FMT_USE_DEFAULT_VALUE);
#endif
		*target = default_value;
	} else {
		*target = value;
#ifdef PRINT_ITEMS
		LOGd("found stored value: %f", *target);
#endif
	}
}
