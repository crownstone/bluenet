#include <protocol/cs_Defaults.h>

void getDefaults(uint8_t configurationType, void* default_type, size_t & default_size) {
    switch(configurationType) {
	case CONFIG_NAME: 
	    // by default create a very large array (of size as defined for NAME), this can be made smaller
	    default_size = sizeof(STRINGIFY(BLUETOOTH_NAME));
	    //default_size = ConfigurationTypeSizes[CONFIG_NAME];
	    char default_name[default_size];
	    snprintf(default_name, default_size, "%s", STRINGIFY(BLUETOOTH_NAME));
	    default_type = default_name;
	    // limit size to null-terminated default_name 
	    //if (strlen(default_name) < default_size) {
		//default_size = strlen(default_name);
	    //}
    }
}

