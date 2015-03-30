/**
 * Author: Christopher Mason
 * Author: Anne van Rossum
 * License: TODO
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "cs_Handlers.h"

extern "C" {

// The header files for the SoftDevice. We're trying really hard to only them only here.
// The user should not need to include files under a Nordic license. 
// This means that we can distribute our files in the end without the corresponding Nordic header files.
#include "ble_gap.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_gatts.h"
#include "ble_gatt.h"

// Refering to files in the Nordic SDK
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_sdm.h"

}

// Local wrapper files
#include <drivers/cs_PWM.h>

#include <util/cs_BleError.h>
#include <drivers/cs_Serial.h>
#include <cs_UUID.h>
#include <cs_AllocatedBuffer.h>
#include <cs_iBeacon.h>

#include <third/std/function.h>


// TODO: replace std::vector with a fixed, in place array of size capacity.

/* A tuple is a vector with a templated type and a public constructor.
 */
template<typename T> class tuple : public std::vector<T> {
  public:
    tuple() {}
};

/* A fixed tuple is a vector with a templated type and a reserved capacity.
 */
template<typename T, uint8_t capacity> class fixed_tuple : public tuple<T> {
  public:

    fixed_tuple<T, capacity>() : tuple<T>() {this->reserve(capacity);}

};

extern "C" {
//void ble_error_handler (std::string msg, uint32_t line_num, const char * p_file_name);
//extern void softdevice_assertion_handler(uint32_t pc, uint16_t line_number, const uint8_t * p_file_name);

/* Interrupt request for SoftDevice
 */
void SWI1_IRQHandler(void);
}

namespace BLEpp {

    class Service;
    class BLEStack;

    /* A value which can be written to or read from a characteristic 
     */
    struct CharacteristicValue {
		uint16_t length;
		uint8_t* data; // TODO use refcount to manage lifetime?  current assumes it to be valid for as long as needed to send to stack.
		// Dominik: for the case of having a serialized object as a value, a buffer is allocated
		// which has to be freed somewhere. It can't be freed at the same point where it is
		// allocated or it will be freed before it can be sent t the stack, so the parameter
		// free can be used to tell the CharacteristicValue to free the data when it is
		// destroyed.
		bool freeOnDestroy;
		CharacteristicValue(uint16_t _length, uint8_t * const _data, bool _free = false) :
					length(_length), data(_data), freeOnDestroy(_free) {
		}
		CharacteristicValue() :
				length(0), data(0), freeOnDestroy(false) {
		}
		~CharacteristicValue() {
			if (freeOnDestroy) {
				free(data);
			}
		}
		bool operator==(const CharacteristicValue& rhs) {
			return rhs.data == data && rhs.length == length;
		}
		bool operator!=(const CharacteristicValue& rhs) {
			return !operator==(rhs);
		}
		CharacteristicValue operator=(const CharacteristicValue& rhs) {
			data = rhs.data;
			length = rhs.length;
			return *this;
		}
	};

    struct CharacteristicInit {
        ble_gatts_attr_t          attr_char_value;
        ble_gatts_char_pf_t       presentation_format;
        ble_gatts_char_md_t       char_md;
        ble_gatts_attr_md_t       cccd_md;
        ble_gatts_attr_md_t       sccd_md;
        ble_gatts_attr_md_t       attr_md;
        ble_gatts_attr_md_t       user_desc_metadata_md;

        CharacteristicInit() : presentation_format({}), char_md({}), cccd_md({}), attr_md({}) {}
    };

    /* Non-template base class for Characteristics.
     *
     * A non-templated base class saves on code size. Note that every characteristic however does still 
     * contribute to code size. 
     */
    class CharacteristicBase {

      public:

      protected:
	// Universally Unique Identifier (8 bytes)
        UUID                      _uuid; 
	// Name (4 bytes)
        std::string               _name;  
	// Read permission (1 byte)
        ble_gap_conn_sec_mode_t   _readperm;  
	// Write permission (1 byte)
        ble_gap_conn_sec_mode_t   _writeperm; 
	// Handles (8 bytes)
        ble_gatts_char_handles_t  _handles;   
	// Reference to corresponding service (4 bytes)
        Service*                  _service; 

        bool                      _inited;
	
	/* This characteristic can be set to notify at regular intervals.
	 *
	 * This interval cannot be set from the client side.
	 */
        bool                      _notifies;
	
	/* This characteristic can be written by another device.
	 */
        bool                      _writable;
	
	// Unit
        uint16_t                  _unit;

	/* Interval for updates (4 bytes), 0 means don't update
	 *
	 * TODO: Currently, this is not in use. 
	 */
        uint32_t                  _updateIntervalMsecs; 
	
	/* If this characteristic can notify a listener (<_notifies>), this field enables it.
	 */
        bool                      _notifyingEnabled;
	
	/* This characteristic can be set to indicate at regular intervals.
	 *
	 * Indication is different from notification, in the sense that it requires ACKs.
	 * https://devzone.nordicsemi.com/question/310/notificationindication-difference/
	 */
        bool                      _indicates;

      public:

        CharacteristicBase();
        virtual ~CharacteristicBase() {}

	/* Initialize the characteristic.
	 * @param svc BLE service this characteristic will belong to.
	 *
	 * Defaults: 
	 *
	 * + readable: true
	 * + notifies: true
	 * + broadcast: false
	 * + indicates: false
	 *
	 * Side effect: sets member field <_inited>.
	 */
        void init(Service* svc);

	/* Set this characteristic to be writable.
	 */
        CharacteristicBase& setWritable(bool writable) {
            _writable = writable;
            setWritePermission(1, 1);
            return *this;
        }

        void setupWritePermissions(CharacteristicInit& ci);

	/* Set this characteristic to be notifiable.
	 */
        CharacteristicBase& setNotifies(bool notifies) {
            _notifies = notifies;
            return *this;
        }

        bool isNotifyingEnabled() {
        	return _notifyingEnabled;
        }

        void setNotifyingEnabled(bool enabled) {
//        	LOGd("[%s] notfying enabled: %s", _name.c_str(), enabled ? "true" : "false");
        	_notifyingEnabled = enabled;
        }

        CharacteristicBase& setIndicates(bool indicates) {
            _indicates = indicates;
            return *this;
        }

        /* Security Mode 0 Level 0: No access permissions at all (this level is not defined by the Bluetooth Core specification).\n
         * Security Mode 1 Level 1: No security is needed (aka open link).\n
         * Security Mode 1 Level 2: Encrypted link required, MITM protection not necessary.\n
         * Security Mode 1 Level 3: MITM protected encrypted link required.\n
         * Security Mode 2 Level 1: Signing or encryption required, MITM protection not necessary.\n
         * Security Mode 2 Level 2: MITM protected signing required, unless link is MITM protected encrypted.\n
        */
        CharacteristicBase& setWritePermission(uint8_t securityMode, uint8_t securityLevel) {
            _writeperm.sm = securityMode;
            _writeperm.lv = securityLevel;
            return *this;
        }

        CharacteristicBase& setUUID(const UUID& uuid) {
            if (_inited) BLE_THROW("Already inited.");
            _uuid = uuid;
            return *this;
        }

        CharacteristicBase& setName(const std::string& name);

        CharacteristicBase& setUnit(uint16_t unit);

        const UUID & getUUID() {
            return _uuid;
        }

        uint16_t getValueHandle() {
            return _handles.value_handle;
        }

        uint16_t getCccdHandle() {
        	return _handles.cccd_handle;
        }

        CharacteristicBase& setUpdateIntervalMSecs(uint32_t msecs);

        virtual uint16_t getValueMaxLength() = 0;

        virtual CharacteristicValue getCharacteristicValue() = 0;
        virtual void setCharacteristicValue(const CharacteristicValue& value) = 0;
        virtual void read() = 0;
        virtual void written(uint16_t len, uint16_t offset, uint8_t* data) = 0;

        virtual void onTxComplete(ble_common_evt_t * p_ble_evt);

      protected:

        virtual void configurePresentationFormat(ble_gatts_char_pf_t &) {}

	/* Notify any listening party.
	 *
	 * If this characteristic can notify, and if notification is enabled, and if there is a connection calling
	 * this function will notify the listening party.
	 *
	 * Notification quickly fills an outgoing buffer in BLE. When this buffer gets full, an error code 
	 * <BLE_ERROR_NO_TX_BUFFERS> is generated by the SoftDevice. In that case <onNotifyTxError> is called.
	 *
	 * @return err_code (which should be NRF_SUCCESS if no error occurred)
	 */
        uint32_t notify();

	/* Any error in <notify> evokes onNotifyTxError.
	 */
        virtual void onNotifyTxError();
    };

    // The default ble_type
    template<typename T> inline  uint8_t ble_type() {
        return BLE_GATT_CPF_FORMAT_STRUCT;
    }
    // A ble_type for strings
    template<> inline uint8_t ble_type<std::string>() {
        return BLE_GATT_CPF_FORMAT_UTF8S;
    }
    // A ble_type for 8-bit unsigned chars
    template<> inline uint8_t ble_type<uint8_t>() {
        return BLE_GATT_CPF_FORMAT_UINT8;
    }
    template<> inline uint8_t ble_type<uint16_t>() {
        return BLE_GATT_CPF_FORMAT_UINT16;
    }
    template<> inline uint8_t ble_type<uint32_t>() {
        return BLE_GATT_CPF_FORMAT_UINT32;
    }
    template<> inline uint8_t ble_type<int8_t>() {
        return BLE_GATT_CPF_FORMAT_SINT8;
    }
    template<> inline uint8_t ble_type<int16_t>() {
        return BLE_GATT_CPF_FORMAT_SINT16;
    }
    template<> inline uint8_t ble_type<int32_t>() {
        return BLE_GATT_CPF_FORMAT_SINT32;
    }
    template<> inline uint8_t ble_type<float>() {
        return BLE_GATT_CPF_FORMAT_FLOAT32;
    }
    template<> inline uint8_t ble_type<double>() {
        return BLE_GATT_CPF_FORMAT_FLOAT64;
    }
    template<> inline uint8_t ble_type<bool>() {
        return BLE_GATT_CPF_FORMAT_BOOLEAN;
    }

    /* Characteristic of generic type T
     *
     * A characteristic first of all contains a templated "value" which might be a string, an integer, or a
     * buffer, depending on the need at hand.
     * It allows also for callbacks to be defined on writing to the characteristic, or reading from the
     * characteristic.
     */
    template<class T>
    class Characteristic : public CharacteristicBase {

        friend class Service;

      public:
        typedef function<void(const T&)> callback_on_write_t;
        typedef function<T()> callback_on_read_t;

      protected:

        T                          _value;
        callback_on_write_t        _callbackOnWrite;
        callback_on_read_t         _callbackOnRead;

      public:
        Characteristic() {
        }
        virtual ~Characteristic() {}

        virtual uint16_t getValueLength() {
            return sizeof(T);
        }

        virtual uint16_t getValueMaxLength() {
            return sizeof(T);
        }

        const T&  __attribute__((optimize("O0"))) getValue() const {
            return _value;
        }

        void setValue(const T& value) {
            _value = value;
        }

        Characteristic<T>& setUUID(const UUID& uuid) {
            CharacteristicBase::setUUID(uuid);
            return *this;
        }

        Characteristic<T>& setName(const std::string& name) {
            CharacteristicBase::setName(name);
            return *this;
        }

        Characteristic<T>& setUnit(uint16_t unit) {
            CharacteristicBase::setUnit(unit);
            return *this;
        }

        Characteristic<T>& setWritable(bool writable) {
            CharacteristicBase::setWritable(writable);
            return *this;
        }

        Characteristic<T>& setNotifies(bool notifies) {
            CharacteristicBase::setNotifies(notifies);
            return *this;
        }

        Characteristic<T>& onWrite(const callback_on_write_t& closure) {
            _callbackOnWrite = closure;
            return *this;
        }

        Characteristic<T>& onRead(const callback_on_read_t& closure) {
            _callbackOnRead = closure;
            return *this;
        }

        Characteristic<T>& setUpdateIntervalMSecs(uint32_t msecs) {
            CharacteristicBase::setUpdateIntervalMSecs(msecs);
            return *this;
        }

        operator T&() const {
            return _value;
        }

        Characteristic<T>& operator=(const T& val) {
            _value = val;

            notify();

            return *this;
        }

#ifdef ADVANCED_OPERATORS
        Characteristic<T>& operator+=(const T& val) {
            _value += val;

            notify();

            return *this;
        }

        Characteristic<T>& operator-=(const T& val) {
            _value -= val;

            notify();

            return *this;
        }

        Characteristic<T>& operator*=(const T& val) {
            _value *= val;

            notify();

            return *this;
        }

        Characteristic<T>& operator/=(const T& val) {
            _value /= val;

            notify();

            return *this;
        }
#endif

        Characteristic<T>& setDefaultValue(const T& t) {
            if (_inited) BLE_THROW("Already inited.");
            _value = t;
            return *this;
        }

      protected:

        void written(uint16_t len, uint16_t offset, uint8_t* data) {
            CharacteristicValue value(len, data+offset);
            setCharacteristicValue(value);

            LOGd("%s: onWrite()", _name.c_str());
            _callbackOnWrite(getValue());
        }

        void read() {
            LOGd("%s: onRead()", _name.c_str());
            T newValue = _callbackOnRead();
            if (newValue != _value) {
                operator=(newValue);
            }
        }


        virtual void configurePresentationFormat(ble_gatts_char_pf_t& presentation_format) {
            presentation_format.format = ble_type<T>();
            presentation_format.name_space = BLE_GATT_CPF_NAMESPACE_BTSIG;
            presentation_format.desc = BLE_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN;
            presentation_format.exponent = 1;
        }

      private:
    };

    /* A default characteristic
     */
    template<typename T, typename E = void> 
    class CharacteristicT : public Characteristic<T> {
      public:
        CharacteristicT()
        : Characteristic<T>() {
        }

        CharacteristicT& operator=(const T& val) {
            Characteristic<T>::operator=(val);
            return *this;
        }
        virtual CharacteristicValue getCharacteristicValue() = 0; // defined only in specializations.
        virtual void setCharacteristicValue(const CharacteristicValue& value) = 0; // defined only in specializations.
    };

//    // A characteristic that implements ISerializable
//    template<typename T >
//    class CharacteristicT<T, typename std::enable_if<std::is_base_of<ISerializable, T>::value >::type> : public Characteristic<T> {
//       public:
//        CharacteristicT& operator=(const T& val) {
//            Characteristic<T>::operator=(val);
//            return *this;
//        }
//
//        virtual CharacteristicValue getCharacteristicValue() {
//            CharacteristicValue value;
//            const T& t = this->getValue();
//            AllocatedBuffer b;
//            b.allocate(t.getSerializedLength());
//            t.serialize(b);
//            b.release();
//            return CharacteristicValue(b.length, b.data);
//        }
//        virtual void setCharacteristicValue(const CharacteristicValue& value) {
//            T t;
//            AllocatedBuffer b(value.length, (uint8_t*)value.data);
//            t.deserialize(b);
//            this->setValue(t);
//        }
//    };

    // A characteristic for built-in arithmetic types (int, float, etc)
    template<typename T > class CharacteristicT<T, typename std::enable_if<std::is_arithmetic<T>::value >::type> : public Characteristic<T> {
    public:
        CharacteristicT& operator=(const T& val) {
            Characteristic<T>::operator=(val);
            return *this;
        }

        virtual CharacteristicValue getCharacteristicValue() {
            // TODO: endianness.
            return CharacteristicValue(sizeof(T), (uint8_t*)&this->getValue());
        }
        virtual void setCharacteristicValue(const CharacteristicValue& value) {
            // TODO: endianness.
            this->setValue(*(T*)(value.data));
        }


    };

    // A characteristic for strings
    template<> class CharacteristicT<std::string> : public Characteristic<std::string> {
      public:
        CharacteristicT& operator=(const std::string& val) {
            Characteristic<std::string>::operator=(val);
            return *this;
        }

        virtual uint16_t getValueMaxLength() {
            return 25;
        }

        virtual CharacteristicValue getCharacteristicValue() {
            return CharacteristicValue(getValue().length(), (uint8_t*)getValue().c_str());
        }
        virtual void setCharacteristicValue(const CharacteristicValue& value) {
            setValue(std::string((char*)value.data, value.length));
        }
    };

    // A characteristic for CharacteristicValue
    template<> class CharacteristicT<CharacteristicValue> : public Characteristic<CharacteristicValue> {
    public:
        CharacteristicT& operator=(const CharacteristicValue& val) {
            Characteristic<CharacteristicValue>::operator=(val);
            return *this;
        }

        virtual uint16_t getValueMaxLength() {
            return 25; // TODO
        }

        virtual CharacteristicValue getCharacteristicValue() {
            return getValue();
        }
        virtual void setCharacteristicValue(const CharacteristicValue& value) {
            setValue(value);
        }
    };

    /* Service as defined in the GATT Specification.
     */
    class Service {
        friend class BLEStack;

      public:
        static const char* defaultServiceName; // "Generic Service"

        typedef tuple<CharacteristicBase*> Characteristics_t;

      protected:

        BLEStack*          _stack;
        UUID               _uuid;
        std::string        _name;
        bool               _primary;
        uint16_t           _service_handle; // provided by stack.
        bool               _started;

      public:
        Service()
            :
              _name(""),
              _primary(true),
              _service_handle(BLE_CONN_HANDLE_INVALID),
              _started(false)
        {


        }
        virtual ~Service() {
            // we don't currently delete our characteristics as we don't really support dynamic service destruction.
            // if we wanted to allow services to be removed at runtime, we would need to, amongst many other things,
            // keep track of whether we allocated the characteristic or whether it was passed into us.
        }

        Service& setName(const std::string& name) {
            _name = name;
            return *this;
        }

        Service& setUUID(const UUID& uuid) {
            if (_started) BLE_THROW("Already started.");

            _uuid = uuid;
            return *this;
        }

        Service& setIsPrimary(bool isPrimary) {
            if (_started) BLE_THROW("Already started.");

            _primary = isPrimary;
            return *this;
        }

        BLEStack* getStack() {
            return _stack;
        }

        UUID& getUUID() {
            return _uuid;
        }

        uint16_t getHandle() {
            return _service_handle;
        }

        /** Registers a new Characteristic with the given uid, name, and intialValue. Pass the type of the
        * characteristic in angle brackets before the parentheses for the method invocation:
        *
        * <pre>
        *      service.createCharacteristic<std::string>().setName("Owner Name").setDefaultValue("Bob Roberts");
        *      service.createCharacteristic<int>().setName("Owner Age").setDefaultValue(39);
        *      service.createCharacteristic<float>().setName("Yaw").setDefaultValue(0.0);
        * */
        template <typename T> Characteristic<T>& createCharacteristic() {
            Characteristic<T>* base = new CharacteristicT<T>();
            addCharacteristic(base);
            return *base;
        }
        
	template <typename T> Characteristic<T>* createCharacteristicRef() {
            Characteristic<T>* base = new CharacteristicT<T>();
            addCharacteristic(base);
            return base;
        }

        // internal:

        virtual void start(BLEStack* stack);
        virtual void stop() {}

        virtual Service& addCharacteristic(CharacteristicBase* characteristic) = 0;

        virtual Characteristics_t& getCharacteristics() = 0;

        virtual void on_ble_event(ble_evt_t * p_ble_evt);

        virtual void on_connect(uint16_t conn_handle, ble_gap_evt_connected_t& gap_evt);  // FIXME NRFAPI  friend??

        virtual void on_disconnect(uint16_t conn_handle, ble_gap_evt_disconnected_t& gap_evt);  // FIXME NRFAPI

        virtual void on_write(ble_gatts_evt_write_t& write_evt);  // FIXME NRFAPI

        virtual void onTxComplete(ble_common_evt_t * p_ble_evt);
    };

    class GenericService;

    typedef void (GenericService::*addCharacteristicFunc)();

    struct CharacteristicStatus {
	    std::string name;
	    uint8_t UUID;
	    bool enabled;
	    BLEpp::addCharacteristicFunc func;
    };
    typedef struct CharacteristicStatus CharacteristicStatusT;

    class GenericService : public Service {

      protected:

        static const uint8_t MAX_CHARACTERISTICS = 6;

        // by keeping the container in this subclass, we can in the future define services that more than MAX_CHARACTERISTICS characteristics.

        fixed_tuple<CharacteristicBase*, MAX_CHARACTERISTICS> _characteristics;

    	// Enabled characteristics (to be set in constructor)
    	std::vector<CharacteristicStatusT> characStatus;

      public:

        GenericService() : Service(), characStatus(0) {
            setName(std::string("GenericService"));
	}

        virtual Characteristics_t & getCharacteristics() {
            return _characteristics;
        }
        virtual GenericService& addCharacteristic(CharacteristicBase* characteristic) {
            if (_characteristics.size() == MAX_CHARACTERISTICS) {
                BLE_THROW("Too many characteristics.");
            }
            _characteristics.push_back(characteristic);

            return *this;
        }

        void addSpecificCharacteristics();

//	virtual void on_ble_event(ble_evt_t * p_ble_evt);
    };

    /* Battery service
     *
     * Defines a single characteristic to read a battery level. This is a predefined UUID, stored at 
     * <BLE_UUID_BATTERY_LEVEL_CHAR>. The name is "battery", and the default value is 100.
     */
    class BatteryService : public GenericService {

      public:
	// Define func_t as a templated function with an unsigned byte
        typedef function<uint8_t()> func_t;

      protected:
	// A single characteristic with an unsigned 8-bit value
	CharacteristicT<uint8_t> *_characteristic;
	// A function for callback, not in use
        func_t _func;
      public:
	// Constructor sets name, allocate characteristic, sets UUID, and sets default value.
        BatteryService(): GenericService() {
            setUUID(UUID(BLE_UUID_BATTERY_SERVICE));
            setName(std::string("BatteryService"));
	
            _characteristic = new CharacteristicT<uint8_t>();

            (*_characteristic).setUUID(UUID(BLE_UUID_BATTERY_LEVEL_CHAR))
                           .setName(std::string("battery"))
                           .setDefaultValue(100);
	    addCharacteristic(_characteristic);
        }

	/* Set the battery level
	 * @batteryLevel level of the battery in percentage
	 *
	 * Indicates the level of the battery in a percentage to the user. This is of no use for a device attached to
	 * the main line. This function writes to the characteristic to show it to the user.
	 */
        void setBatteryLevel(uint8_t batteryLevel){
            (*_characteristic) = batteryLevel;
        }

	/* Set a callback function for a battery level change
	 * @func callback function
	 *
	 * Not in use
	 */
        void setBatteryLevelHandler(func_t func) {
            _func = func;
        }
    };

    /* BLEStack defines a chip-agnostic Bluetooth Low-Energy stack
     *
     * Currently, this class does not leverage much of the general Bluetooth Low-Energy functionality into
     * chip-agnostic code. However, this might be recommendable in the future.
     */
    class BLEStack {
        public:
        virtual bool connected() = 0;
        virtual uint16_t getConnectionHandle() = 0;
    };

    /* nRF51822 specific implementation of the BLEStack
     *
     * The Nrf51822BluetoothStack class is a direct descendent from BLEStack. It is implemented as a singleton, such
     * that it can only be allocated once and it can be reached from everywhere in the code, especially in interrupt
     * handlers. However, please, if an object depends on it, try to make this dependency explicit, and use this
     * stack object as an argument w.r.t. this object. This makes dependencies traceble for the user.
     */
    class Nrf51822BluetoothStack : public BLEStack {
        friend void SWI2_IRQHandler();   // ble stack events.
        friend void ::SWI1_IRQHandler(); // radio notification

    private:
	// The Nrf51822BluetoothStack is defined as a singleton
	Nrf51822BluetoothStack(); 
	Nrf51822BluetoothStack(Nrf51822BluetoothStack const&); 
	void operator=(Nrf51822BluetoothStack const &); 
	virtual ~Nrf51822BluetoothStack();
    public:
	static Nrf51822BluetoothStack& getInstance() {
		static Nrf51822BluetoothStack instance;
		return instance;
	}
        typedef function<void(uint16_t conn_handle)>   callback_connected_t;
        typedef function<void(uint16_t conn_handle)>   callback_disconnected_t;
        typedef function<void(bool radio_active)>   callback_radio_t;

        static const uint8_t MAX_SERVICE_COUNT = 5;

        //static const uint16_t                  defaultAppearance = BLE_APPEARANCE_UNKNOWN;
	static const uint16_t                  defaultAppearance = BLE_APPEARANCE_GENERIC_KEYRING;
        //static const nrf_clock_lfclksrc_t      defaultClockSource = NRF_CLOCK_LFCLKSRC_XTAL_20_PPM;
        static const nrf_clock_lfclksrc_t      defaultClockSource = NRF_CLOCK_LFCLKSRC_SYNTH_250_PPM;
        static const uint8_t                   defaultMtu = BLE_L2CAP_MTU_DEF;
        static const uint16_t                  defaultMinConnectionInterval_1_25_ms = 400;
        static const uint16_t                  defaultMaxConnectionInterval_1_25_ms = 800;
        static const uint16_t                  defaultSlaveLatencyCount = 0;
        static const uint16_t                  defaultConnectionSupervisionTimeout_10_ms = 400;
        static const uint16_t                  defaultAdvertisingInterval_0_625_ms = 40;
        static const uint16_t                  defaultAdvertisingTimeout_seconds = 180;
        static const int8_t                    defaultTxPowerLevel = -8;

    protected:
        std::string                                      _device_name; // 4
        uint16_t                                    _appearance;
        fixed_tuple<Service*, MAX_SERVICE_COUNT>    _services;  // 32

        nrf_clock_lfclksrc_t                        _clock_source; //4
        uint8_t                                     _mtu_size;
        int8_t                                      _tx_power_level;
        uint8_t *                                   _evt_buffer;
        uint32_t                                    _evt_buffer_size;
        ble_gap_conn_sec_mode_t                     _sec_mode;  //1
        //ble_gap_sec_params_t                        _sec_params; //6
        //ble_gap_adv_params_t                        _adv_params; // 20
        uint16_t                                    _interval;
        uint16_t                                    _timeout;
        ble_gap_conn_params_t                       _gap_conn_params; // 8

        bool                                        _inited;
        bool                                        _started;
        bool                                        _advertising;
        bool                                        _scanning;

        uint16_t                                    _conn_handle;

        callback_connected_t                        _callback_connected;  // 16
        callback_disconnected_t                     _callback_disconnected;  // 16
        callback_radio_t                            _callback_radio;  // 16
        volatile uint8_t                            _radio_notify; // 0 = no notification (radio off), 1 = notify radio on, 2 = no notification (radio on), 3 = notify radio off.
    public:
        Nrf51822BluetoothStack& init();

        Nrf51822BluetoothStack& start();

        Nrf51822BluetoothStack& shutdown();

        Nrf51822BluetoothStack& setAppearance(uint16_t appearance) {
            if (_inited) BLE_THROW("Already initialized.");
            _appearance = appearance;
            return *this;
        }

        Nrf51822BluetoothStack& setDeviceName(const std::string& deviceName) {
            if (_inited) BLE_THROW("Already initialized.");
            _device_name = deviceName;
            return *this;
        }

        Nrf51822BluetoothStack& setClockSource(nrf_clock_lfclksrc_t clockSource) {
            if (_inited) BLE_THROW("Already initialized.");
            _clock_source = clockSource;
            return *this;
        }

        Nrf51822BluetoothStack& setAdvertisingInterval(uint16_t advertisingInterval){
            // TODO: stop advertising?
            _interval = advertisingInterval;
            return *this;
        }

        Nrf51822BluetoothStack& setAdvertisingTimeoutSeconds(uint16_t advertisingTimeoutSeconds) {
            // TODO: stop advertising?
            _timeout = advertisingTimeoutSeconds;
            return *this;
        }

        Nrf51822BluetoothStack& updateDeviceName(const std::string& deviceName);
	std::string & getDeviceName() { return _device_name; }

        /** Set radio transmit power in dBm (accepted values are -40, -20, -16, -12, -8, -4, 0, and 4 dBm). */
        Nrf51822BluetoothStack& setTxPowerLevel(int8_t powerLevel);

        int8_t getTxPowerLevel() {
            return _tx_power_level;
        }

        /** Set the minimum connection interval in units of 1.25 ms. */
        Nrf51822BluetoothStack& setMinConnectionInterval(uint16_t connectionInterval_1_25_ms) {
            if (_gap_conn_params.min_conn_interval != connectionInterval_1_25_ms) {
                _gap_conn_params.min_conn_interval = connectionInterval_1_25_ms;
                if (_inited) setConnParams();
            }
            return *this;
        }

        /** Set the maximum connection interval in units of 1.25 ms. */
        Nrf51822BluetoothStack& setMaxConnectionInterval(uint16_t connectionInterval_1_25_ms) {
            if (_gap_conn_params.max_conn_interval != connectionInterval_1_25_ms) {
                _gap_conn_params.max_conn_interval = connectionInterval_1_25_ms;
                if (_inited) setConnParams();
            }
            return *this;
        }

        /** Set the slave latency count. */
        Nrf51822BluetoothStack& setSlaveLatency(uint16_t slaveLatency) {
            if ( _gap_conn_params.slave_latency != slaveLatency ) {
                _gap_conn_params.slave_latency = slaveLatency;
                if (_inited) setConnParams();
            }
            return *this;
        }

        /** Set the connection supervision timeout in units of 10 ms. */
        Nrf51822BluetoothStack& setConnectionSupervisionTimeout(uint16_t conSupTimeout_10_ms) {
            if (_gap_conn_params.conn_sup_timeout != conSupTimeout_10_ms) {
                _gap_conn_params.conn_sup_timeout = conSupTimeout_10_ms;
                if (_inited) setConnParams();
            }
            return *this;
        }

        Nrf51822BluetoothStack& onConnect(const callback_connected_t& callback) {
            //if (callback && _callback_connected) BLE_THROW("Connected callback already registered.");

            _callback_connected = callback;
            if (_inited) setConnParams();
            return *this;
        }
        Nrf51822BluetoothStack& onDisconnect(const callback_disconnected_t& callback) {
            //if (callback && _callback_disconnected) BLE_THROW("Disconnected callback already registered.");

            _callback_disconnected = callback;

            return *this;
        }

        Service& createService() {
            GenericService* svc = new GenericService();
            addService(svc);
            return *svc;
        }

        BatteryService& createBatteryService() {
            BatteryService* svc = new BatteryService();
            addService(svc);
            return *svc;
        }

        Service& getService(std::string name);
        Nrf51822BluetoothStack& addService(Service* svc);

        /* Start advertising as an iBeacon
         *
         * @beacon the object defining the parameters for the
         *   advertisement package. See <IBeacon> for an explanation
         *   of the parameters and values
         *
         * Initiates the advertisement package and fills the manufacturing
         * data array with the values of the <IBeacon> object, then starts
         * advertising as an iBeacon.
         *
         * **Note**: An iBeacon requires that the company identifier is
         *   set to the Apple Company ID, otherwise it's not an iBeacon.
         */
        Nrf51822BluetoothStack& startIBeacon(IBeacon beacon);

		/* Start sending advertisement packets.
 	 	 * This can not be called while scanning, start scanning while advertising is possible though.
 	 	 */
        Nrf51822BluetoothStack& startAdvertising();

        Nrf51822BluetoothStack& stopAdvertising();

        bool isAdvertising();
#if(SOFTDEVICE_SERIES != 110)
        Nrf51822BluetoothStack& startScanning();

        Nrf51822BluetoothStack& stopScanning();

        bool isScanning();
#else
        Nrf51822BluetoothStack& startScanning() {};

        Nrf51822BluetoothStack& stopScanning() {};

        bool isScanning() { return false; }
#endif
        Nrf51822BluetoothStack& onRadioNotificationInterrupt(uint32_t distanceUs, callback_radio_t callback);

        virtual bool connected() {
            return _conn_handle != BLE_CONN_HANDLE_INVALID;
        }
        virtual uint16_t getConnectionHandle() {  // TODO are multiple connections supported?
            return _conn_handle;
        }
        /** Call this as often as possible.  Callbacks on services and characteristics will be invoked from here. */
        void tick();

    protected:
        void setTxPowerLevel();
        void setConnParams();

        void on_ble_evt(ble_evt_t * p_ble_evt);
        void on_connected(ble_evt_t * p_ble_evt);
        void on_disconnected(ble_evt_t * p_ble_evt);
        void on_advertisement(ble_evt_t * p_ble_evt);
        void onTxComplete(ble_evt_t * p_ble_evt);

    };


} // end of namespace
