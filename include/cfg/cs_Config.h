/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#define CROWNSTONE_COMPANY_ID                    0x038E

//! size of the buffer used for characteristics
//#define GENERAL_BUFFER_SIZE                      300

/** maximum length of strings used for characteristic values */
#define MAX_STRING_LENGTH                        25

/** define the maximum size for strings to be stored
 */
#define MAX_STRING_STORAGE_SIZE                  31

/** Command to enter the bootloader and stay there.
 *
 * This should be the same value as defined in the bootloader.
 */
#define GPREGRET_DFU_RESET                       66
#define GPREGRET_BROWNOUT_RESET                  96
#define GPREGRET_SOFT_RESET                      0

/*
 */
#define APP_TIMER_PRESCALER                      0
#define APP_TIMER_MAX_TIMERS                     10
#define APP_TIMER_OP_QUEUE_SIZE                  10

/*
 */
/** Maximum size of scheduler events. */
#define SCHED_MAX_EVENT_DATA_SIZE                ((CEIL_DIV(MAX(MAX(BLE_STACK_EVT_MSG_BUF_SIZE,    \
                                                                    ANT_STACK_EVT_STRUCT_SIZE),    \
                                                                SYS_EVT_MSG_BUF_SIZE),             \
                                                            sizeof(uint32_t))) * sizeof(uint32_t))
/** Maximum number of events in the scheduler queue. */
#define SCHED_QUEUE_SIZE                         30

//! See https://devzone.nordicsemi.com/question/21164/s130-unstable-advertising-reports-during-scan-updated/
/** Determines scan interval in units of 0.625 millisecond. */
#define SCAN_INTERVAL                            0x00A0
/** Determines scan window in units of 0.625 millisecond. */
//#define SCAN_WINDOW                            0x0050
/** Determines scan window in units of 0.625 millisecond. */
#define SCAN_WINDOW                              0x009E

//! bonding / security
#define SEC_PARAM_TIMEOUT                        30                                          /** < Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                           1                                           /** < Perform bonding. */
#define SEC_PARAM_MITM                           1                                           /** < Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES                BLE_GAP_IO_CAPS_DISPLAY_ONLY				/** < No I/O capabilities. */
#define SEC_PARAM_OOB                            0                                           /** < Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE                   7                                           /** < Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE                   16                                          /** < Maximum encryption key size. */

#define SECURITY_REQUEST_DELAY                   1500                                        /**< Delay after connection until security request is sent, if necessary (ms). */

//! tx power used for low power mode during bonding
#define LOW_TX_POWER                             -40

#define CLOCK_SOURCE                             NRF_CLOCK_LFCLKSRC_RC_250_PPM_TEMP_8000MS_CALIBRATION

//! duration (in ms) how long the relay pins should be set to high
#define RELAY_HIGH_DURATION                      20 //! ms

//! Max number of schedule entries in the schedule service.
#define MAX_SCHEDULE_ENTRIES                     10

#define CURRENT_LIMIT							 0

#define CS_ADC_MAX_PINS                          2
#define CS_ADC_TIMER                             NRF_TIMER1
//#define CS_ADC_TIMER_IRQ                         TIMER1_IRQHandler
#define CS_ADC_TIMER_IRQn                        TIMER1_IRQn
#define CS_ADC_PPI_CHANNEL                       7

#if CONTINUOUS_POWER_SAMPLER == 1
#define CS_ADC_SAMPLE_RATE                       100
#else
#define CS_ADC_SAMPLE_RATE                       3000 //! Max 10000 / numpins (min about 500? to avoid too large difference in timestamps)
#endif

#define POWER_SAMPLE_BURST_INTERVAL              3000 //! Time to next burst sampling (ms)
#define POWER_SAMPLE_BURST_NUM_SAMPLES           80 //! Number of voltage and current samples per burst

#define POWER_SAMPLE_CONT_INTERVAL               50 //! Time to next buffer read and attempt to send msg (ms)
#define POWER_SAMPLE_CONT_NUM_SAMPLES            80 //! Number of voltage and current samples in the buffer, written by ADC, read by power service

#define STORAGE_REQUEST_BUFFER_SIZE              5

#define FACTORY_RESET_CODE                       0xdeadbeef

#define ENCYRPTION_KEY_LENGTH                    SOC_ECB_KEY_LENGTH //! 16 byte length

#define BROWNOUT_TRIGGER_THRESHOLD               NRF_POWER_THRESHOLD_V27

#define VOLTAGE_MULTIPLIER                       2.374f
#define CURRENT_MULTIPLIER                       0.044f
#define VOLTAGE_ZERO                             169.0f
#define CURRENT_ZERO                             168.5f
#define POWER_ZERO                               9.0f
#define POWER_ZERO_AVG_WINDOW                    100
