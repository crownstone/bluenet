# Etiquette

- use typify
- no implicit cast operators
- give names to functions
- don't init structs like: { 9, true, 3 }

# Code

## Whitespace

Indent with tabs, align with spaces.

Always put spaces after a comma, and before a `{`.
Although you could argue that `if`, `switch`, etc are also functions, we put a space between those and the `(`.
Math operators (`+`, `-`, `*`, etc) should also be surrounded by spaces.

Header example:
```
class MyClass: public Foo {
public:
	int barMethod(uint8_t value, bool triple);
}
```

Source example:
```
int MyClass::barMethod(uint8_t value, bool triple) {
	if (triple && value > 0) {
		value = value * 3;
	}
	else if (triple) {
		value = (value + 3) / 3;
	}
	else {
		value = value * 1;
	}

	switch (value) {
		case 0: {
			return 0;
		}
		case 1: {
			value++;
			break;
		}
		case 2: {
			int temp = 4;
			value += temp;
			break;
		}
		default: {
			return 0;
		}
	}

	switch (value) {
		case 0:    return 100    + value;
		case 10:   return 1000   + value;
		case 100:  return 10000  + value;
		case 1000: return 100000 + value;
	}

	return value;
}
```

The end bracket, `}`, is always on a line of its own. This makes it easier to parse in your head when the body of the `if` statement becomes larger.
The `case` clauses are indented w.r.t. the `switch` statement. This makes it easy to see where the switch stops.
The brackets are optional.

Nerd tip: create a `source/.vimrc` file with as option `set cinoptions=l1`.

In a function with pointers or address references, the operator is attached to the type (`void*` you can call a "void pointer").

```
void someEventHandler(pod_t const* event, void* context, uint32_t& address) {
```

## Naming

Identifiers follow the following convention, falling back to a previous rule if no specific case is declared for that particular scope.

- Namespace scope (possibly global):
	- classes (non-PODs): `UpperCamel`
	- structs (PODs): `small_caps_t`
	- enums: both members and type names currently varying between `UpperCamel` and `ALL_CAPS`
	- macros: `ALL_CAPS`
	- functions: `lowerCamel`
	- constants: `ALL_CAPS`
	- templates: same as underlying type
	- typedefs/aliases: same as underlying type
- Class/struct scope:
	- variables: `_underscoredLowerCamel`
	- methods: `lowerCamel`
	- constants: `ALL_CAPS`
- Method/function scope, including their parameters:
	- variables: `lowerCamel`

In a short overview:

```
#define CS_TYPE_CAST(EVT_NAME, PTR) reinterpret_cast<TYPIFY(EVT_NAME)*>(PTR)

static constexpr auto BLUETOOTH_NAME = "CRWN";

class ClassName {
private:
	typedef uint8_t index_t;

	class Settings {
		bool isActive;
	};

public:
	void functionName();
};

using MyVec = std::vector<uint8_t>;

struct __attribute__((__packed__)) a_packed_packet_t {
	uint8_t shortWord;
	uint32_t longWord;
};
```

Notes:
- Abbreviations in identifyer are considered to be a whole word in `lowerCamel` or `UpperCamel`. Only the first letter of an abbreviation will be upper case. For example: `rssiMacAddress`.
- The convention for variable/member names is chosen to make it easier to recognize their scope and saves you from coming up with alternative names in contexts such as:
	```
	void setRssi(int8_t rssi) { _rssi = rssi; }
	```
- Avoid use of single letters for identifiers (with the exception of a variable for loop iterations) as it impairs search/replace tools and readibility.
- Avoid use of names longer than about 35 characters.

## Comments

### Function comments
```
/**
 * Short description.
 *
 * Optional longer explanation.
 *
 * @param[in]     aParameter            Explanation.
 * @param[out]    anotherParameter      Explanation.
 * @param[in,out] exoticInOutParameter  Explanation.
 * @return        Explanation.
 */
```

## Various

### POD types

All data types that are used for communication over hardware protocols will be **Plain Old Data** types (PODs). All PODs will be declared as structs with the modifier `__attribute__((__packed__))`. Adding methods to datatypes that cross hardware boundaries is possible as long as these definitions do not interfere with the POD property of said type.

```
struct __attribute__((__packed__)) a_packed_packet_t {
	uint8_t shortWord;
	uint32_t longWord;
};
```

### Constants
There is a strong preference to use typed `constexpr` values over macros. The use of `auto` is permitted if the codebase would emit warnings when replacing a macro with a constexprs of particular type.

```
#define BLUETOOTH_NAME "CRWN"
static constexpr auto BLUETOOTH_NAME = "CRWN"; // <-- strongly preferred
```

### Declare variables on their own line.

This usually makes it easier to read.

```
int a;
int b;
```

### If statement body on its own line, and always in brackets.

Putting the body on a separate line makes it easier to read, while leaving out brackets easily introduces bugs.

```
if (on) {
	turnOn = true;
}
```




