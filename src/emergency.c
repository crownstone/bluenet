
#include <stdint.h>

/** When your program includes throwing an exception, the standard library
* allocates as static data (BSS) an "emergency buffer" that's used to store
* the exception in the case that there is no dynamic memory available.
* This buffer is by default quite large, 2k.
*
* This is an attempt to reduce the size of the emergency buffer.
* This does not appear to work however for reasons that escape me.
*/

/*
# define EMERGENCY_OBJ_SIZE	128
# define EMERGENCY_OBJ_COUNT	2
typedef char one_buffer[EMERGENCY_OBJ_SIZE] __attribute__((aligned));
static one_buffer emergency_buffer[EMERGENCY_OBJ_COUNT];
*/
