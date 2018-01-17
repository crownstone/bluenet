/**
 * Author: Crownstone Team
 * Copyright: Crownstone
 * Date: 4 Nov., 2014
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#define CROWNSTONE_COMPANY_ID                    0x038E

//! size of the buffer used for characteristics
//#define GENERAL_BUFFER_SIZE                      300

/** maximum length of strings used for characteristic values */
#define DEFAULT_CHAR_VALUE_STRING_LENGTH         25

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

/** Priorities of the different peripherals
 */
//#define SAADC_TIMER_CONFIG_IRQ_PRIORITY          2
//#define SAADC_CONFIG_IRQ_PRIORITY                2
//#define COMP_CONFIG_IRQ_PRIORITY                 3
//#define LPCOMP_CONFIG_IRQ_PRIORITY               3

/*
 */
#define APP_TIMER_PRESCALER                      0
#define APP_TIMER_MAX_TIMERS                     10
#define APP_TIMER_OP_QUEUE_SIZE                  10

/*
 */
/** Maximum size of scheduler events. */
/*
#define SCHED_MAX_EVENT_DATA_SIZE                ((CEIL_DIV(MAX(MAX(BLE_STACK_EVT_MSG_BUF_SIZE,    \
                                                                    ANT_STACK_EVT_STRUCT_SIZE),    \
                                                                SYS_EVT_MSG_BUF_SIZE),             \
                                                            sizeof(uint32_t))) * sizeof(uint32_t))
*/
#define SCHED_MAX_EVENT_DATA_SIZE                128

/** Maximum number of events in the scheduler queue. */
#define SCHED_QUEUE_SIZE                         30

//! See https://devzone.nordicsemi.com/question/84767/s132-scan-intervalwindow-adv-interval/
//! Old https://devzone.nordicsemi.com/question/21164/s130-unstable-advertising-reports-during-scan-updated/
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
/* moved to boards config in cs_Boards.c
#define LOW_TX_POWER                             -40
 */

//#define CLOCK_SOURCE                             NRF_CLOCK_LFCLKSRC_RC_250_PPM_TEMP_8000MS_CALIBRATION

//! duration (in ms) how long the relay pins should be set to high
#define RELAY_HIGH_DURATION                      15 //! ms
//! duration (in ms) how long to retry switching the relay if there was not enough power to switch.
#define RELAY_DELAY                              50 //! ms

//! Max number of schedule entries in the schedule service.
#define MAX_SCHEDULE_ENTRIES                     10

//! How many seconds of time jump is regarded a big time jump
#define SCHEDULE_BIG_TIME_JUMP                   (75*60)

//! How many channels the pwm can control. Limited by the amount of timer channels.
#define CS_PWM_MAX_CHANNELS                      2

//! ----- TIMERS -----
#define CS_PWM_TIMER                             NRF_TIMER4
#define CS_PWM_TIMER_IRQ                         TIMER4_IRQHandler
#define CS_PWM_IRQn                              TIMER4_IRQn
#define CS_PWM_INSTANCE_INDEX                    TIMER4_INSTANCE_INDEX
#define CS_PWM_TIMER_ID                          4
#define CS_PWM_TIMER_IRQ_PRIORITY                APP_IRQ_PRIORITY_HIGH
//#define CS_PWM_TIMER_FREQ                        NRF_TIMER_FREQ_500kHz
#define CS_PWM_TIMER_FREQ                        NRF_TIMER_FREQ_4MHz // Max frequency, higher can give issues.

#define CS_ADC_TIMER                             NRF_TIMER1
#define CS_ADC_TIMER_IRQ                         TIMER1_IRQHandler
#define CS_ADC_TIMER_IRQn                        TIMER1_IRQn
#define CS_ADC_INSTANCE_INDEX                    TIMER1_INSTANCE_INDEX
#define CS_ADC_TIMER_ID                          1
#define CS_ADC_TIMER_FREQ                        NRF_TIMER_FREQ_16MHz


//! ----- PPI -----
#define CS_ADC_PPI_CHANNEL_START                 0
#define CS_ADC_PPI_CHANNEL_COUNT                 1
#define CS_PWM_PPI_CHANNEL_START                 (CS_ADC_PPI_CHANNEL_START + CS_ADC_PPI_CHANNEL_COUNT)
#define CS_PWM_PPI_CHANNEL_COUNT                 (2 + 2 * CS_PWM_MAX_CHANNELS)

//! ----- PPI groups -----
#define CS_PWM_PPI_GROUP_START                   0
#define CS_PWM_PPI_GROUP_COUNT                   1

//! ----- GPIOTE -----
#define CS_PWM_GPIOTE_CHANNEL_START              0
#define CS_PWM_GPIOTE_CHANNEL_COUNT              (CS_PWM_MAX_CHANNELS)



#define POWER_SAMPLE_BURST_NUM_SAMPLES           75 // Number of voltage and current samples per burst

#define CS_ADC_SAMPLE_INTERVAL_US                200
//#define CS_ADC_SAMPLE_INTERVAL_US                400

#define CS_ADC_MAX_PINS                          2
#define CS_ADC_NUM_BUFFERS                       2 // ADC code is currently written for (max) 2
#define CS_ADC_BUF_SIZE                          (2*20000/CS_ADC_SAMPLE_INTERVAL_US)
//#define CS_ADC_BUF_SIZE                          (2*30000/CS_ADC_SAMPLE_INTERVAL_US)

#define STORAGE_REQUEST_BUFFER_SIZE              5

#define FACTORY_RESET_CODE                       0xdeadbeef
#define FACTORY_RESET_TIMEOUT                    60000 // Timeout before recovery becomes unavailable after reset (ms)
#define FACTORY_PROCESS_TIMEOUT                  200 // Timeout before recovery process step is executed (ms)

#define ENCYRPTION_KEY_LENGTH                    SOC_ECB_KEY_LENGTH //! 16 byte length

#define BROWNOUT_TRIGGER_THRESHOLD               NRF_POWER_THRESHOLD_V27

/* moved to boards config in cs_Boards.c
#define VOLTAGE_MULTIPLIER                       0.20f
//#define VOLTAGE_MULTIPLIER                       0.40f // For 1_4 gain
#define CURRENT_MULTIPLIER                       0.0045f
//#define CURRENT_MULTIPLIER                       0.0110f // For 1_4 gain
#define VOLTAGE_ZERO                             2003
#define CURRENT_ZERO                             1997
#define POWER_ZERO                               1500
*/
//#define POWER_ZERO_AVG_WINDOW                    100
// Octave: a=0.05; x=[0:1000]; y=(1-a).^x; y2=cumsum(y)*a; figure(1); plot(x,y); figure(2); plot(x,y2); find(y2 > 0.99)(1)
#define VOLTAGE_ZERO_EXP_AVG_DISCOUNT            20  // Is divided by 1000, so 20 is a discount of 0.02. // 99% of the average is influenced by the last 228 values
//#define VOLTAGE_ZERO_EXP_AVG_DISCOUNT            1000 // No averaging
#define CURRENT_ZERO_EXP_AVG_DISCOUNT            100 // Is divided by 1000, so 100 is a discount of 0.1. // 99% of the average is influenced by the last 44 values
//#define CURRENT_ZERO_EXP_AVG_DISCOUNT            1000 // No averaging
#define POWER_EXP_AVG_DISCOUNT                   200 // Is divided by 1000, so 200 is a discount of 0.2. // 99% of the average is influenced by the last 21 values
//#define POWER_EXP_AVG_DISCOUNT                   1000 // No averaging
#define POWER_SAMPLING_RMS_WINDOW_SIZE           9 // Windows size used for filtering the power and current rms. Currently can only be 7, 9, or 25!

#define POWER_SAMPLING_CURVE_HALF_WINDOW_SIZE    5 // Half window size used for filtering the current curve. Can't just be any value!
//#define POWER_SAMPLING_CURVE_HALF_WINDOW_SIZE    16 // Half window size used for filtering the current curve. Can't just be any value!

#define CURRENT_USAGE_THRESHOLD                  (16000) // Power usage threshold in mA at which the switch should be turned off.
#define CURRENT_USAGE_THRESHOLD_PWM              (1000)  // Power usage threshold in mA at which the PWM should be turned off.

#define PWM_PERIOD                               10000L // Interval in us: 1/10000e-6 = 100 Hz

#define KEEP_ALIVE_INTERVAL                      (2 * 60) // 2 minutes, in seconds

#define SWITCH_DELAYED_STORE_MS                  10000 // Timeout before storing the pwm switch value is stored.

#define PWM_BOOT_DELAY_MS                        60000 // Delay after boot until pwm can be used. Has to be smaller than overflow time of RTC.

// Stack config values
// See: https://devzone.nordicsemi.com/question/60/what-is-connection-parameters/

/*
 * Determines how often the Central will ask for data from the Peripheral. When the Peripheral requests an update, it 
 * supplies a maximum and a minimum wanted interval. The connection interval must be between 7.5 ms and 4 s. 
 */
#define MIN_CONNECTION_INTERVAL                  6   // In units of 1.25ms. Lowest possible, see https://devzone.nordicsemi.com/question/161154/minimum-connection-interval/
#define MAX_CONNECTION_INTERVAL                  16  // In units of 1.25ms.

/*
 * This timeout determines the timeout from the last data exchange till a link is considered lost. A Central will not 
 * start trying to reconnect before the timeout has passed, so if you have a device which goes in and out of range 
 * often, and you need to notice when that happens, it might make sense to have a short timeout.
 */
#define CONNECTION_SUPERVISION_TIMEOUT           400 // In units of 10ms.

/* 
 * By setting a non-zero slave latency, the Peripheral can choose to not answer when the Central asks for data up to 
 * the slave latency number of times. However, if the Peripheral has data to send, it can choose to send data at any 
 * time. This enables a peripheral to stay sleeping for a longer time, if it doesn't have data to send, but still send 
 * data fast if needed. The text book example of such device is for example keyboard and mice, which want to be 
 * sleeping for as long as possible when there is no data to send, but still have low latency (and for the mouse: 
 * low connection interval) when needed.
 */
#define SLAVE_LATENCY                            10  // See: https://devzone.nordicsemi.com/question/14029/slave-latency-for-s110s120-connection/

#define ADVERTISING_TIMEOUT                      0
#define ADVERTISING_REFRESH_PERIOD               1000 // Push the changes in the advertisement packet to the stack every x milliseconds
#define ADVERTISING_REFRESH_PERIOD_SETUP         100  // Push the changes in the advertisement packet to the stack every x milliseconds

#define MESH_STATE_REFRESH_PERIOD                50000 // 50 seconds + some random amount of seconds
#define MESH_STATE_TIMEOUT                       (3*MESH_STATE_REFRESH_PERIOD) // ms until state of a crownstone is considered to be timed out.
#define LAST_SEEN_COUNT_PER_STATE_CHAN           3 // Number of last seen timestamps to store per state channel.

#define SWITCH_ON_AT_SETUP_BOOT_DELAY            3600  // Seconds until the switch turns on when in setup mode (Crownstone built-in only)
