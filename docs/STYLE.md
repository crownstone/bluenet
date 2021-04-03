# Formatter

Bluenet style is defined in source/.clang-format. Support for autoformatting in IDEs is widespread.
E.g.: 
- This [Eclipse plugin](https://marketplace.eclipse.org/content/cppstyle) enables formatting files on save or selected pieces of code using `ctrl+shift+f`.
- Commandline tools such as `python3 -m pip install clang-format` are also available.

# Etiquette

- use typify
- no implicit cast operators
- give names to functions
- don't init structs like: { 9, true, 3 }

# Code

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
- Abbreviations in identifier are considered to be a whole word in `lowerCamel` or `UpperCamel`. Only the first letter of an abbreviation will be upper case. For example: `rssiMacAddress`.
- The convention for variable/member names is chosen to make it easier to recognize their scope and saves you from coming up with alternative names in contexts such as:
	```
	void setRssi(int8_t rssi) {
		_rssi = rssi;
	}
	```
- Avoid use of single letters for identifiers (with the exception of a variable for loop iterations) as it impairs search/replace tools and readability.
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
#define BLUETOOTH_NAME "CRWN"                     // <-- obsolete
static constexpr auto BLUETOOTH_NAME   = "CRWN";  // <-- acceptable
static constexpr char[] BLUETOOTH_NAME = "CRWN";  // <-- preferred
```

### Declare variables on their own line

This usually makes it easier to read.

```
int a;
int b;
```

### Use nullptr instead of NULL

The preprocessor symbol `NULL` is usually defined as `(void*)0)`, but it is implementation defined. There are subtle differences between `NULL` and `nullptr` due to freedom of implementation. Hence `nullptr` is preferred or possibly an explicit alternative if `nullptr` doesn't lead to intended behaviour.

### If statement block must always be in brackets.

Leaving out brackets easily introduces bugs.

```
if (on) {
	turnOn = true;
}
```

### Include what you use

Each source file `filename.cpp` file includes its corresponding header `filename.h`. 
If a class, struct or function etc., say `class someclass{};` is defined in `otherfilename.h`:
- If `filename.h` uses `someclass`, it should _directly_ `#include <otherfilename.h>`
- Else, if `filename.cpp` uses `someclass`, it should _directly_ `#include <otherfilename.h>`

This paradigm ensures that changing a header does not break any upward dependencies increases awareness of dependencies and make them easier to analyse.    


