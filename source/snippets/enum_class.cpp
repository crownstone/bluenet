/**
 * Small example file that shows how to use enums with non-consecutive series of values.
 * Each value we like to enrich as well.
 *
 * There are two main options (let's not go the route of pre-compiler macros):
 *
 * 1. enums
 * 2. enum classes
 *
 * Pros and cons. The enums are integers. This means that when data is received over the air or when
 * it has to be written to disk, there is no need to cast back and forth to the right enum value. This in
 * contrast with the enum classes. However, the latter have many benefits:
 *
 * a. It is relative easy to do the casting using static_cast. No big deal. It needs to be done only when
 *    reading/writing to disk or the network. In all other places it will be nicely strongly typed.
 * b. The constexpr functions that can be used to assign properties to each enum value can have really
 *    sweet switch statements. This also means that they are properly defined at compile time for
 *    integer values outside of the enum class.
 * c. The std::array setup uses more memory. It has define every consecutive value for that array.
 * d. It is possible to define for example a StateIterator or a ConfigIterator that each only iterates
 *    over a subset of the enums.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <array>
#include <iostream>
#include <type_traits>

enum Value : uint8_t { DEFAULT_PROPERTY0, FLASH, RAM, FIRMWARE_DEFAULT, UNKNOWN };

template <typename C, C beginVal, C endVal>
class Iterator {
	typedef typename std::underlying_type<C>::type val_t;
	int val;

public:
	Iterator(const C& f) : val(static_cast<val_t>(f)) {}
	Iterator() : val(static_cast<val_t>(beginVal)) {}
	Iterator operator++() {
		++val;
		return *this;
	}
	C operator*() { return static_cast<C>(val); }
	Iterator begin() { return *this; }  // default ctor is good
	Iterator end() {
		static const Iterator endIter = ++Iterator(endVal);  // cache it
		return endIter;
	}
	bool operator!=(const Iterator& i) { return val != i.val; }
};

/*
 * Define an operator +, which allows to cast to underlying uint8_t, CS_TYPE, or other type. This is a static
 * cast. Do not use it if not necessary.
 */
template <typename T>
constexpr auto operator+(T e) noexcept -> std::enable_if_t<std::is_enum<T>::value, std::underlying_type_t<T>> {
	return static_cast<std::underlying_type_t<T>>(e);
}

#define State_Base 0x80

enum Type : uint8_t {
	STATE_RESET_COUNTER_UNIQUE = State_Base,  // 0x80 or 128
	STATE_SWITCH_STATE_UNIQUE,
	STATE_ACCUMULATED_ENERGY,
	STATE_POWER_USAGE,
	STATE_TRACKED_DEVICES,
	STATE_SCHEDULE,

	STATE_TYPES
};

enum class CS_TYPE : uint8_t {
	STATE_RESET_COUNTER = State_Base,  // 0x80 or 128
	STATE_SWITCH_STATE,
	STATE_ACCUMULATED_ENERGY,
	STATE_POWER_USAGE,
	STATE_TRACKED_DEVICES,
	STATE_SCHEDULE,

	STATE_TYPES
};

//! We can define an iterator over a subset of the enum values... It assumes consecutive values though.
typedef Iterator<CS_TYPE, CS_TYPE::STATE_RESET_COUNTER, CS_TYPE::STATE_SCHEDULE> stateIterator;

/*
 * This is an array. This wastes a lot of zeros for enums we have not defined.
 */
constexpr ::std::array<Value, STATE_TYPES> Property0() {
	using result_t  = ::std::array<Value, STATE_TYPES>;
	result_t result = {};
	for (uint8_t i = 0; i < STATE_TYPES; ++i) {
		result[i] = UNKNOWN;
	}
	for (uint8_t i = State_Base; i < STATE_TYPES; ++i) {
		result[i] = RAM;
	}
	return result;
}

/*
 * We can now have a nice switch statement that is entirely compile time, really pretty.
 */
constexpr Value Property1(CS_TYPE const& type) {
	switch (type) {
		case CS_TYPE::STATE_RESET_COUNTER:
		case CS_TYPE::STATE_SWITCH_STATE:
		case CS_TYPE::STATE_ACCUMULATED_ENERGY:
		case CS_TYPE::STATE_POWER_USAGE:
		case CS_TYPE::STATE_TRACKED_DEVICES:
		case CS_TYPE::STATE_SCHEDULE: return RAM;
		default: return UNKNOWN;
	}
};

#define STRINGIFY(s) STRINGIFY_HELPER(s)
#define STRINGIFY_HELPER(s) #s

#ifdef NAME_DEFAULT
#define CONFIG_NAME_DEFAULT STRINGIFY(NAME_DEFAULT)
#else
#define CONFIG_NAME_DEFAULT "Donnie"
#endif

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/**
 * I don't know if constexpr makes sense here.
 */
constexpr void Property2(CS_TYPE const& type, void* data, const size_t max_size) {
	switch (type) {
		case CS_TYPE::STATE_RESET_COUNTER: memcpy(data, "reset", MIN(max_size, sizeof("reset"))); break;
		case CS_TYPE::STATE_SWITCH_STATE:
		case CS_TYPE::STATE_ACCUMULATED_ENERGY:
		case CS_TYPE::STATE_POWER_USAGE:
		case CS_TYPE::STATE_TRACKED_DEVICES:
		case CS_TYPE::STATE_SCHEDULE:
			memcpy(data, CONFIG_NAME_DEFAULT, MIN(max_size, sizeof(CONFIG_NAME_DEFAULT)));
			break;
		default: memcpy(data, "default", MIN(max_size, sizeof("default")));
	}
}

constexpr const CS_TYPE TO_CS_TYPE(uint8_t const& type) {
	// can we make this safer?
	return static_cast<CS_TYPE>(type);
};

constexpr const ::std::array<Value, STATE_TYPES> property0 = Property0();

int main() {
	// introduce overflow, will only be an issue for the first setup
	uint8_t OVERFLOW = 10;

	static_assert(STATE_RESET_COUNTER_UNIQUE == 0x80);
	static_assert(property0[STATE_RESET_COUNTER_UNIQUE] == RAM);
	static_assert(property0[0] == UNKNOWN);
	// following statement gives a nice error, this is nice, however, see below where propert0[0x90] will
	// give undefined values at runtime.
	// static_assert (property0[0x90] == UNKNOWN);

	std::cout << "Use as array, but unsafe" << std::endl;
	std::cout << "Table[0]:" << &property0[0] << std::endl;
	std::cout << "Table[1]:" << &property0[1] << std::endl;
	std::cout << std::endl;

	for (uint8_t i = 0; i < STATE_TYPES + OVERFLOW; i++) {
		// not safe
		std::cout << property0[i] << ' ';
		if (!((i % 20) - 20 + 1)) std::cout << std::endl;
	}
	std::cout << std::endl;

	std::cout << "Type: " << typeid(STATE_SWITCH_STATE_UNIQUE).name() << std::endl;
	std::cout << "Type: " << typeid(property0[0]).name() << std::endl;
	std::cout << "Type: " << typeid(&property0[0]).name() << std::endl;
	std::cout << std::endl;

	// safe, for particular values we can use it safe...
	std::cout << "Now in a safe manner for a single value" << std::endl;
	std::cout << std::get<STATE_SWITCH_STATE_UNIQUE>(property0) << std::endl;
	std::cout << std::endl;

	static_assert(+CS_TYPE::STATE_RESET_COUNTER == 0x80);
	static_assert(Property1(CS_TYPE::STATE_RESET_COUNTER) == RAM);
	// the following statement actually works fine, it is indeed defined as UNKNOWN at compile time!
	static_assert(Property1(static_cast<CS_TYPE>(0x90)) == UNKNOWN);
	static_assert(TO_CS_TYPE(0x80) == CS_TYPE::STATE_RESET_COUNTER);

	// Note the cast is only because an uint8_t is shown as a char
	std::cout << "Use instead an enum class and a templated custom iterator" << std::endl;
	for (CS_TYPE i : stateIterator()) {
		std::cout << Property1(i) << ' ';
	}
	std::cout << std::endl << std::endl;
	std::cout << "Now in a safe manner for a single value" << std::endl;
	std::cout << Property1(CS_TYPE::STATE_SWITCH_STATE) << std::endl;
	std::cout << std::endl;

	for (uint8_t i = 0; i < STATE_TYPES + OVERFLOW; i++) {
		//      auto ltype = static_cast<CS_TYPE>(i);
		auto ltype = TO_CS_TYPE(i);
		std::cout << Property1(ltype) << ' ';
		if (!((i % 20) - 20 + 1)) std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << std::endl;

	std::cout << "Type: " << typeid(CS_TYPE::STATE_SWITCH_STATE).name() << std::endl;
	std::cout << "Type: " << typeid(+CS_TYPE::STATE_SWITCH_STATE).name() << std::endl;
	std::cout << "Type: " << typeid(Property1(CS_TYPE::STATE_SWITCH_STATE)).name() << std::endl;

	std::cout << std::endl;
	std::cout << "For reference sake" << std::endl;
	uint8_t t;
	std::cout << "Type uint8_t: " << typeid(t).name() << std::endl;
	std::cout << "Type CS_TYPE: " << typeid(CS_TYPE).name() << std::endl;

	std::cout << std::endl;

	size_t max_size = 10;
	uint8_t name[max_size];
	Property2(CS_TYPE::STATE_SWITCH_STATE, name, max_size);
	std::cout << "Name: " << name << std::endl;
}
