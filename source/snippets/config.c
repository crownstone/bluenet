/**
 * Small example file that shows how we obtain configuration values as macros.
 *
 * We have two configuration values:
 *   NAME_DEFAULT
 *   NAME_ADDRESS
 *
 * They can be defined as usual with:
 *
 *   gcc config.c -o config -DNAME_DEFAULT=$USER" -DADDRESS_DEFAULT="abbacafebabe"
 *
 * They are optional:
 *
 *   gcc config.c -o config
 *
 * Run the program as (it does not accept any options):
 *
 *   ./config
 *
 * There are a few things you will notice. First, the defaults are defined as macros as well. It's all already
 * defined at precompile time. The get function here uses a void pointer and assumes it is allocated beforehand.
 * If this is not done, it can easily lead to a segfault.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define STRINGIFY(s) STRINGIFY_HELPER(s)
#define STRINGIFY_HELPER(s) #s

enum { NAME, ADDRESS } TYPES;

#ifdef NAME_DEFAULT
#define CONFIG_NAME_DEFAULT STRINGIFY(NAME_DEFAULT)
#else
#define CONFIG_NAME_DEFAULT "Donnie"
#endif

#ifdef ADDRESS_DEFAULT
#define CONFIG_ADDRESS_DEFAULT STRINGIFY(ADDRESS_DEFAULT)
#else
#define CONFIG_ADDRESS_DEFAULT "232028ADFA12"
#endif

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void get(uint8_t type, void* data, size_t max_size) {
	switch (type) {
		case NAME: memcpy(data, CONFIG_NAME_DEFAULT, MIN(max_size, sizeof(CONFIG_NAME_DEFAULT))); break;
		case ADDRESS: memcpy(data, CONFIG_ADDRESS_DEFAULT, MIN(max_size, sizeof(CONFIG_ADDRESS_DEFAULT))); break;
	}
}

int main() {
	// assume we have allocated before an array that is large enough
	size_t max_size = 5;
	uint8_t name[max_size];
	get(NAME, name, max_size);

	char* config_name = name;
	printf("Hello %s!\n", config_name);

	max_size = 12;
	uint8_t address[max_size];
	get(ADDRESS, address, max_size);

	printf("Your BLE address is: ");
	printf("%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
		   address[0],
		   address[1],
		   address[2],
		   address[3],
		   address[4],
		   address[5],
		   address[6],
		   address[7],
		   address[8],
		   address[9],
		   address[10],
		   address[11]);

	printf("\n");
}
