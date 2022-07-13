# Formatter

You can use auto formatting by using `source/.clang-format`. Auto formatted code is not always the best though, so use it as a tool rather than the truth. Support for autoformatting in IDEs is widespread.
E.g.: 
- This [Eclipse plugin](https://marketplace.eclipse.org/content/cppstyle) enables formatting files on save or selected pieces of code using `ctrl+shift+f`.
- Commandline tools such as `python3 -m pip install clang-format` are also available.

Investigate the `source/.clang-format` (<https://github.com/crownstone/bluenet/blob/master/source/.clang-format>) file for more formatting details, a few:

- align subsequent assignments on the `=` token
- column limit at 120
- brace wrapping before else

# Etiquette

- use typify
- no implicit cast operators
- give names to functions
- don't init structs like: { 9, true, 3 }

# Role of formatter

The current state of the code does not adhere to the formatter everywhere. The code will be cleaned up over time.
The goal is to be able to run the formatter over each file without having it changing any whitespace.
That means it is possible to concentrate on functional changes.
Hence, always run the formatter before committing. Also, if the formatter makes something less readible, adjust the
code slightly. Some examples of making it easier for the formatter:

- Use curly brackets for each case in a switch block.
- Do not add `/******** *******/` or other kind of decorations to delineate code sections.
- Do not use trailing comments, they are a pain to align. Just explain code with comments before that code.
- Remove code that is not in use. There should be no commented code. Eventually place it within a preprocessor clause, but be careful not to introduce code rot in case those preprocessor defines are never compiled for. Refer eventually in a comment to an older commit if a reference to unused code will be important in the future.

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
	
	uint16_t* _data; 

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
- Use `uint16_t*` with no space in between (not `uint16_t *_data`). Do not declare multiple variables on one line (see below).
- Assume C++ / g++ compiler in the sense that no `typedef` is required for the `struct` while still being able to pass it around as `func(a_packed_packet_t p)`.

## Indents

Indentation defaults can be found in <https://github.com/crownstone/bluenet/blob/master/source/.clang-format> as well:

- Indent case statement within a switch block.

## Comments

### Function comments

In header files create a description for the methods or classes that looks like this (which is doxygen compatible). Do use good comment hygiene. Don't describe how a function works, but the conditions under which it is expected to be used or why it is written in a certain way. Use complete grammatical sentences starting with a capital and ending with a dot.

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

In source files additional information can be given about particular implementation details. This will not end up in doxygen (a single `/*` rather than `/**` code block starter is sufficient).

```
/*
 * Optional longer explanation.
 */
```

### Struct comments

Just add your comments to just before the struct definition. Do not sprinkle code with oneliner comments per field. 

### General

Within the function bodies themselves, abstain from comments unless absolutely necessary. If comments are required, write them on a line preceding the code. Do **not** write comments on the same line as the code.

```
// Always write comments on a separate line. 
bool result = false;
```

### Get rid of TODOs

Don't sprinkle the code with many TODOs, use them only when absolutely necessary.

### Get rid of commented code

If code is commented it should be deleted. If you want to preserve code for posterity find other means to do so.

## Various

### POD types

Datastructures that cross the boundaries of the application memory (ram) are tied to strict lay-out rules to ensure correct interoperability. In particular this applies to all data types that are used for communication over hardware protocols (bluetooth, flash, uart, etc.).

 All such structures must:

* be defined in `./source/include/protocol/`
* be **Plain Old Data** types (PODs).
* be packed `struct __attribute__((__packed__)) struct_name_t { /* ... */ };`
* as much as possible ensure that members are aligned on 4 bytes boundaries. E.g.:

```
struct __attribute__((__packed__)) bad_t { 
	uint16_t _memberX;  
	uint32_t _memberY; // Bad: member pointer straddles 4 byte
	uint16_t _memberZ; 
};
```

```
struct __attribute__((__packed__)) good_t { 
	uint16_t _memberX; 
	uint16_t _memberZ;  // Good: placing the two uint16_t's side by side correctly aligns _memberY
	uint32_t _memberY;
};
```

Adding methods to datatypes that cross hardware boundaries is possible as long as these definitions do not interfere with the POD property of said type.


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

The preprocessor symbol `NULL` is usually defined as `((void*)0)`, but it is implementation defined. There are subtle differences between `NULL` and `nullptr` due to freedom of implementation. Hence `nullptr` is preferred or possibly an explicit alternative if `nullptr` doesn't lead to intended behaviour.

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


## CMake

### Auto generated files
Always use the `@ONLY` option when using `configure_file(..)`. This increases grep-ability of the codebase and makes for clearer template files for bash scripts.

### Config values
Some config values are defined both in code and in config files. These are statically checked to be equal during compilation. For these variables, the config file variable name is leading and should be used where possible.

E.g. the memory locations of some firmware blobs:


The config defines the leading variable name.
```
./config/nrf52840/CMakeBuild.config

# This setting is defined in nrf_dfu_types.h. Here we predefine it so it is available to the wider code base.
# This is the mbr settings address for the nRF52840-QFAA.
CS_BOOTLOADER_SETTINGS_ADDRESS=0xFF000
```

In other files, this name is used.
```
./source/CMakeLists.txt

add_custom_target(read_bootloader_settings
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=${BOOTLOADER_SETTINGS_ADDRESS}" "-DCOUNT=128" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read bootloader settings from remote target board"
	)
```

The relevant source file statically asserts the equality.
```
./source/include/cfg/cs_MemoryLayout.h.in:

_Static_assert(
		BOOTLOADER_SETTINGS_ADDRESS == @CS_BOOTLOADER_SETTINGS_ADDRESS@, "Bootloader settings address mismatch");
```

