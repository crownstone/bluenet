
extern "C" {

// Called by BluetoothLE.h classes when exceptions are disabled.
void ble_error_handler (string msg, uint32_t line_num, const char * p_file_name) {
    string message __attribute__((unused)) = msg;
    uint16_t line __attribute__((unused)) = line_num;
    const char* file __attribute__((unused)) = p_file_name;
    __asm("BKPT");
    while(1) {}
}

// called by soft device when you pass bad parameters, etc.
void app_error_handler (uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
    uint32_t error = error_code;
    uint16_t line = line_num;
    const uint8_t* file = p_file_name;
    __asm("BKPT");
    while(1) {}
}

//called by NRF SDK when it has an internal error.
void assert_nrf_callback (uint16_t line_num, const uint8_t *file_name) {
    uint16_t line = line_num;
    const uint8_t* file = file_name;
    __asm("BKPT");
    while(1) {}
}

//called by soft device when it has an internal error.
void softdevice_assertion_handler(uint32_t pc, uint16_t line_num, const uint8_t * file_name){
    uint16_t line = line_num;
    const uint8_t* file = file_name;
    __asm("BKPT");
    while(1) {}
}
}

