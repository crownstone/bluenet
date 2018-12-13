/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 5, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <string>

#include <ble/cs_Nordic.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Storage.h>

class StorageHelper {
public:
	StorageHelper() {};

	////////////////////////////////////////////////////////////////////////////////////////
	//! helper functions
	////////////////////////////////////////////////////////////////////////////////////////

	/** Helper function to calculate the offset of a variable inside a storage struct
	 * @storage pointer to the storage struct where the variable is stored. (storage struct is a
	 * 1 to 1 representation of the FLASH memory)
	 * @variable pointer to the variable inside the struct
	 * @return returns the offset in bytes
	 */
	static pstorage_size_t getOffset(ps_storage_base_t* storage, uint8_t* variable);

	/** Helper function to convert std::string to char array
	 * @value the input string
	 * @target pointer to the output char array
	 */
	static void setString(std::string value, char* target);
	static void setString(const char* value, uint16_t length, char* target);

	/** Helper function to get std::string from char array in the struct
	 * @value pointer the input char array
	 * @target the output string
	 * @default_value the default string to be used if no valid string found
	 *   in the char array
	 *
	 * In order to show that the field of the struct is unassigned, we use
	 * the fact that the last byte of the char array is set to FF.
	 * To show that the char array is empty, we fill it with 0.
	 *
	 * If the value read from the char array is empty or unassigned the default
	 * value will be returned instead
	 */
//	static void getString(char* value, std::string& target, std::string default_value);
	static void getString(char* value, char* target, char* default_value, uint16_t& size, bool getDefaultValue);

	/** Helper function to set a byte in the field of a struct
	 * @value the byte value to be copied to the struct
	 * @target pointer the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 3 bytes of the field
	 * are set to 0
	 */
	static void setUint8(uint8_t value, uint32_t& target);

	/** Helper function to read a byte from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer to the byte where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint8(uint32_t value, uint8_t* target, uint8_t default_value, bool getDefaultValue);

	/** Helper function to set a short (uint16_t) in the field of a struct
	 * @value the value to be copied to the struct
	 * @target pointer to the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 2 bytes of the field
	 * are set to 0
	 */
	static void setUint16(uint16_t value, uint32_t& target);

	/** Helper function to read a short (uint16_t) from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer the uint16_t variable where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint16(uint32_t value, uint16_t* target, uint16_t default_value, bool getDefaultValue);

	/** Helper function to set a signed short (int16_t) in the field of a struct
	 * @value the value to be copied to the struct
	 * @target pointer the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 2 bytes of the field
	 * are set to 0
	 */
	static void setInt16(int16_t value, int32_t& target);

	/** Helper function to read a signed short (int16_t) from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer to the int16_t variable where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getInt16(int32_t value, int16_t* target, int16_t default_value, bool getDefaultValue);


	/** Helper function to set an integer (uint32_t) in the field of a struct
	 * @value the value to be copied to the struct
	 * @target pointer to the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 */
	static void setUint32(uint32_t value, uint32_t& target);

	/** Helper function to read an integer (uint32_t) from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer the uint32_t varaible where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint32(uint32_t value, uint32_t* target, uint32_t default_value, bool getDefaultValue);

	/** Helper function to set an integer (int32_t) in the field of a struct
	 * @value the value to be copied to the struct
	 * @target pointer to the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 */
	static void setInt32(int32_t value, int32_t& target);

	/** Helper function to read an integer (int32_t) from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer the int32_t variable where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getInt32(int32_t value, int32_t* target, int32_t default_value, bool getDefaultValue);

	/** Helper function to set a signed byte in the field of a struct
	 * @value the byte value to be copied to the struct
	 * @target pointer the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 3 bytes of the field
	 * are set to 0
	 */
	static void setInt8(int8_t value, int32_t& target);

	/** Helper function to read a signed byte from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer to the byte where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getInt8(int32_t value, int8_t* target, int8_t default_value, bool getDefaultValue);

//	static void setDouble(double value, double& target);

//	static void getDouble(double value, double* target, double default_value);

	static void setFloat(float value, float& target);

	static void getFloat(float value, float* target, float default_value, bool getDefaultValue);

	/** Helper function to write/copy an array to the field of a struct
	 * @T primitive type, such as uint8_t
	 * @src pointer to the array to be written
	 * @dest pointer to the array field of the struct
	 * @length the number of elements in the source array. the destination
	 *   array needs to have space for at least length elements
	 *
	 * Copies the data contained in the src array to the destination
	 * array. It is not possible to write an array containing only FF
	 * to memory, since that is used to show that the array is
	 * uninitialized.
	 */
	template<typename T>
	static void setArray(T* src, T* dest, uint16_t length) {
		bool isUnassigned = true;
		uint8_t* ptr = (uint8_t*)src;
		for (uint32_t i = 0; i < length * sizeof(T); ++i) {
			if (ptr[i] != 0xFF) {
				isUnassigned = false;
				break;
			}
		}

		if (isUnassigned) {
			LOGe("cannot write all FF");
		} else {
			memcpy(dest, src, length * sizeof(T));
		}
	}

	/** Helper function to read an array from a field of a struct
	 * @T primitive type, such as uint8_t
	 * @src pointer to the array field of the struct
	 * @dest pointer to the destination array
	 * @default_value pointer to an array containing the default values
	 *   can be NULL pointer
	 * @length number of elements in the array (all arrays need to have
	 *   the same length!)
	 * @return returns true if an array was found in storage, false otherwise
	 *
	 * Checks the memory of the source array field. If it is all FF, that means
	 * the memory is unassigned. If an array is provided as default_value,
	 * that array will be copied to the destination array, if no default_value
	 * array is provided (NULL pointer), nothing happens and the destination
	 * array will remain as it was. Otherwise, the data from the source array
	 * field will be copied to the destination array
	 */
	template<typename T>
	static bool getArray(T* src, T* dest, T* default_value, uint16_t length, bool getDefaultValue) {

#ifdef PRINT_ITEMS
		_logSerial(SERIAL_INFO, "raw value: \r\n");
		BLEutil::printArray((uint8_t*)src, length * sizeof(T));
#endif

		bool isUnassigned = true;
		if (!getDefaultValue) {
			uint8_t* ptr = (uint8_t*)src;
			for (uint32_t i = 0; i < length * sizeof(T); ++i) {
				if (ptr[i] != 0xFF) {
					isUnassigned = false;
					break;
				}
			}
		}

		if (isUnassigned) {
#ifdef PRINT_ITEMS
			LOGd("use default value");
#endif
			if (default_value != NULL) {
				memcpy(dest, default_value, length * sizeof(T));
			} else {
				memset(dest, 0, length * sizeof(T));
			}
		} else {
			memcpy(dest, src, length * sizeof(T));
		}

		return !isUnassigned;
	}


};

