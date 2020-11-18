
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
	int bar(int value, bool triple);
}
```

Source example:
```
int MyClass::bar(int value, bool triple) {
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
		case 0:
			return 0;
		case 1:
			value +=1;
			break;
		case 2: {
			int temp = 4;
			value += temp;
			break;
		}
		default:
			return 0;
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

This way, the '}' is always a line on its own, making it easier to parse in your head, especially when the body of the if statements become larger.
By indenting the cases, it's easy to see where the switch stops.



## Naming

Try to use camelcase naming. Struct definitions are the exception.

- ClassName
- functionName()
- cs_some_struct_t

Sometimes you have a full uppercase abbreviation in your name, in that case just pretend it's a word. For example: rssiMacAddress.

### Variables

- localVar
- _memberVar

This makes it easier to recognize member variables, and saves you from coming up with alternative names when you have things like: `void setRssi(int8_t rssi) { _rssi = rssi; }`.

Try to actually name your variables, avoid single letters (with the exception of loop counters).


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
