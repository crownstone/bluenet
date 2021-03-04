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

Upper CamelCase naming is used for class names, typedefs and aliases. Members and functions are lower camel cased except for inner classes and types. 

```
void someEventHandler(pod_t const* event, void* context, uint32_t& address) {
```

## Naming

Upper CamelCase naming is used for class names, typedefs and aliases. Members and functions are lower camel cased except for inner classes and types. 

```
class ClassName {
    private:
    typedef std::vector<uint8_t>::iterator IteratorType;
    
    class Settings {
        bool isActive;
    };
    
    public:
    void functionName();
};

using MyVec = std::vector<uint8_t>;

Struct definitions (or in general **plain old data**, POD) are the exception, end them with `_t`.

```
struct __attribute__((__packed__)) a_packed_packet_t {
    uint8_t shortWord;
    uint32_t longWord;
};
```
Macros are an exception as well (use them sparingly), use underscores.

- `BLUETOOTH_NAME`

Sometimes you have a full uppercase abbreviation in your name, in that case just pretend it's a word. For example: rssiMacAddress.

### Variables

- `localVar`
- `_memberVar`

This makes it easier to recognize member variables, and saves you from coming up with alternative names when you have things like: `void setRssi(int8_t rssi) { _rssi = rssi; }`.

Try to actually name your variables, avoid single letters (with the exception of a variable for loop iterations).

## Comments

### Function comments
```
/**
 * Short description.
 *
 * Optional longer explanation.
 *
 * @param[in]     val    Explanation.
 * @param[out]    val    Explanation.
 * @param[in,out] val    Explanation.
 * @return        Explanation.
 */
```

## Various

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




