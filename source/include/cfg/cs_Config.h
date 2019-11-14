/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 4 Nov., 2014
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#define CROWNSTONE_COMPANY_ID                    0x038E

// size of the buffer used for characteristics
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
#define GPREGRET_DFU_RESET                       66 // 07-11-2019 TODO: why 66? It makes more sense to use 63 or 31.
#define GPREGRET_BROWNOUT_RESET                  96 // 07-11-2019 TODO: why 96? It makes more sense to use 64 or 32.
#define GPREGRET_SOFT_RESET                      1

/**
 * Values used to remember flags after a reboot.
 *
 * Make sure this doesn't interfere with the nrf bootloader values that are used. Like:
 * - BOOTLOADER_DFU_GPREGRET2_MASK
 * - BOOTLOADER_DFU_GPREGRET2
 * - BOOTLOADER_DFU_SKIP_CRC_BIT_MASK
 */
#define GPREGRET2_STORAGE_RECOVERED              4

/** Priorities of the different peripherals
 */
//#define SAADC_TIMER_CONFIG_IRQ_PRIORITY          2
//#define SAADC_CONFIG_IRQ_PRIORITY                2
//#define COMP_CONFIG_IRQ_PRIORITY                 3
//#define LPCOMP_CONFIG_IRQ_PRIORITY               3

/*
 */
#define APP_TIMER_PRESCALER                      0

/** Size of queues holding timer operations that are pending execution
 *  Meaning: amount of timers that can be started simultaneously.
 */
//#define APP_TIMER_OP_QUEUE_SIZE                  10
#define APP_TIMER_OP_QUEUE_SIZE                  16

/**
 * Maximum size of scheduler events.
 * TODO: NRF_SDH_BLE_EVT_BUF_SIZE is very large, examples don't use it. Maybe we can use a smaller size?
 */
#define SCHED_MAX_EVENT_DATA_SIZE               (MAX(20, MAX(APP_TIMER_SCHED_EVENT_DATA_SIZE, NRF_SDH_BLE_EVT_BUF_SIZE)))

/** Maximum number of events in the scheduler queue.
 *
 *  The scheduler will require a buffer of size:
 *  (SCHED_MAX_EVENT_DATA_SIZE + APP_SCHED_EVENT_HEADER_SIZE) * (SCHED_QUEUE_SIZE + 1)
 */
#define SCHED_QUEUE_SIZE                         32

/**
 * Buffer size that is used for characteristics that the user reads from.
 */
#define CS_CHAR_READ_BUF_SIZE                    MASTER_BUFFER_SIZE

/**
 * Buffer size that is used for characteristics that the user writes to.
 */
#define CS_CHAR_WRITE_BUF_SIZE                   MASTER_BUFFER_SIZE

/**
 * Determines scan interval in units of 0.625 millisecond.
 * Channel is changed every interval.
 */
#define SCAN_INTERVAL                            160 // 100 ms
/**
 * Determines scan window in units of 0.625 millisecond.
 * This is the amount of time in an interval that the scanner actually listens.
 * You should leave some radio time for other modules (like advertising).
 * See https://devzone.nordicsemi.com/question/84767/s132-scan-intervalwindow-adv-interval
 * See https://devzone.nordicsemi.com/f/nordic-q-a/14733/s132-scan-interval-window-adv-interval
 * Old https://devzone.nordicsemi.com/question/21164/s130-unstable-advertising-reports-during-scan-updated/
 */
#define SCAN_WINDOW                              158 // 98.75 ms

#define APP_BLE_CONN_CFG_TAG                     1 // Connection configuration identifier.

// bonding / security
#define SEC_PARAM_TIMEOUT                        30                                          /** < Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                           1                                           /** < Perform bonding. */
#define SEC_PARAM_MITM                           1                                           /** < Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES                BLE_GAP_IO_CAPS_DISPLAY_ONLY				/** < No I/O capabilities. */
#define SEC_PARAM_OOB                            0                                           /** < Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE                   7                                           /** < Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE                   16                                          /** < Maximum encryption key size. */

#define SECURITY_REQUEST_DELAY                   1500                                        /**< Delay after connection until security request is sent, if necessary (ms). */

#define SWITCH_CLAIM_TIME_MS                     2000                                        /**< Time that switch commands of other sources are ignored. */

// tx power used for low power mode during bonding
/* moved to boards config in cs_Boards.c
#define LOW_TX_POWER                             -40
 */

//#define CLOCK_SOURCE                             NRF_CLOCK_LFCLKSRC_RC_250_PPM_TEMP_8000MS_CALIBRATION

// duration (in ms) how long the relay pins should be set to high
#define RELAY_HIGH_DURATION                      15
// duration (in ms) how long to retry switching the relay if there was not enough power to switch.
#define RELAY_DELAY                              50

// Max number of schedule entries in the schedule service.
#define MAX_SCHEDULE_ENTRIES                     10

// How many seconds of time jump is regarded a big time jump
#define SCHEDULE_BIG_TIME_JUMP                   (75*60)

// How many channels the pwm can control. Limited by the amount of timer channels.
#define CS_PWM_MAX_CHANNELS                      2

// ----- TIMERS -----
// Soft device uses timer 0
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

#define CS_ADC_TIMEOUT_TIMER                     NRF_TIMER2
#define CS_ADC_TIMEOUT_TIMER_IRQ                 TIMER2_IRQHandler
#define CS_ADC_TIMEOUT_TIMER_IRQn                TIMER2_IRQn
#define CS_ADC_TIMEOUT_INSTANCE_INDEX            TIMER2_INSTANCE_INDEX
#define CS_ADC_TIMEOUT_TIMER_ID                  2
#define CS_ADC_TIMEOUT_TIMER_IRQ_PRIORITY        APP_IRQ_PRIORITY_MID // MUST be lower than CS_ADC_IRQ_PRIORITY


// ----- PPI -----
// Soft device uses 17-31
// Mesh SDK uses 8-11
#define CS_PWM_PPI_CHANNEL_START                 0
#define CS_PWM_PPI_CHANNEL_COUNT                 (2 + 2 * CS_PWM_MAX_CHANNELS)
//#define CS_ADC_PPI_CHANNEL_START                 (CS_PWM_PPI_CHANNEL_START + CS_PWM_PPI_CHANNEL_COUNT)
#define CS_ADC_PPI_CHANNEL_START                 12
#define CS_ADC_PPI_CHANNEL_COUNT                 4

// ----- PPI groups -----
// Soft device uses 4-5
#define CS_PWM_PPI_GROUP_START                   0
#define CS_PWM_PPI_GROUP_COUNT                   1

// ----- GPIOTE -----
#define CS_ADC_GPIOTE_CHANNEL_START              0
#define CS_ADC_GPIOTE_CHANNEL_COUNT              1 // Actually only used for debug
#define CS_PWM_GPIOTE_CHANNEL_START              (CS_ADC_GPIOTE_CHANNEL_START + CS_ADC_GPIOTE_CHANNEL_COUNT)
#define CS_PWM_GPIOTE_CHANNEL_COUNT              (CS_PWM_MAX_CHANNELS)

// ----- SAADC -----
#define CS_ADC_RESOLUTION                        NRF_SAADC_RESOLUTION_12BIT
#define CS_ADC_IRQ_PRIORITY                      APP_IRQ_PRIORITY_HIGH
#define CS_ADC_IRQ                               SAADC_IRQHandler


#define CS_ADC_SAMPLE_INTERVAL_US                200 // 100 samples per period of 50Hz wave
#define CS_ADC_MAX_PINS                          2
#define CS_ADC_NUM_BUFFERS                       4
#define CS_ADC_BUF_SIZE                          (CS_ADC_MAX_PINS * 20000 / CS_ADC_SAMPLE_INTERVAL_US) // Make size so it fills up 20ms of data.

#define POWER_SAMPLE_BURST_NUM_SAMPLES           (20000/CS_ADC_SAMPLE_INTERVAL_US) // Number of voltage and current samples per burst


// Buffer size for storage requests. Storage requests get buffered when the device is scanning or meshing.
#define STORAGE_REQUEST_BUFFER_SIZE              5 // Should be at least 3, because setup pushes 3 storage requests (configs + operation mode + switch state).

#define FACTORY_RESET_CODE                       0xdeadbeef
#define FACTORY_RESET_TIMEOUT                    60000 // Timeout before recovery becomes unavailable after reset (ms)
#define FACTORY_PROCESS_TIMEOUT                  200 // Timeout before recovery process step is executed (ms)

#define ENCRYPTION_KEY_LENGTH                    16 // 16 byte length, just like SOC_ECB_KEY_LENGTH

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

#define SWITCHCRAFT_THRESHOLD                    (500000) // Threshold for switch recognition (float).

#define PWM_PERIOD                               10000L // Interval in us: 1/10000e-6 = 100 Hz

#define KEEP_ALIVE_INTERVAL                      (2 * 60) // 2 minutes, in seconds

#define SWITCH_DELAYED_STORE_MS                  10000 // Timeout before storing the pwm switch value is stored.
#define STATE_RETRY_STORE_DELAY_MS               1000 // Time before retrying to store a varable to flash.
#define MESH_SEND_TIME_INTERVAL_MS               60000 // Interval at which the time is sent via the mesh.
#define MESH_SEND_TIME_INTERVAL_MS_VARIATION     10000 // Max amount that gets added to interval.
#define MESH_SEND_STATE_INTERVAL_MS              60000 // Interval at which the stone state is sent via the mesh.
#define MESH_SEND_STATE_INTERVAL_MS_VARIATION    10000 // Max amount that gets added to interval.

#define PWM_BOOT_DELAY_MS                        60000 // Delay after boot until pwm can be used. Has to be smaller than overflow time of RTC.
#define DIMMER_BOOT_CHECK_DELAY_MS               5000  // Delay after boot until power measurement is checked to see if dimmer works.
#define DIMMER_BOOT_CHECK_POWER_MW               3000  // Threshold in milliWatt above which the dimmer is considered to be working.
#define DIMMER_BOOT_CHECK_POWER_MW_UNCALIBRATED  10000 // Threshold in milliWatt above which the dimmer is considered to be working, in case power zero is not calibrated yet.

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

#define SWITCHCRAFT_DEBUG_BUFFERS                false // Set to true to store the last voltage samples that were recognized as switch (short power interrupt).

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
#define ADVERTISING_REFRESH_PERIOD_SETUP         500  // Push the changes in the advertisement packet to the stack every x milliseconds

#define EXTERNAL_STATE_LIST_COUNT                10 // Number of stones to cache the state of, for advertising external state.
#define EXTERNAL_STATE_TIMEOUT_MS                60000 // Time after which a state of another stone is considered to be timed out.

#define SWITCH_ON_AT_SETUP_BOOT_DELAY            3600  // Seconds until the switch turns on when in setup mode (Crownstone built-in only)

/**
 * Interval in milliseconds at which tick events are dispatched.
 */
#define TICK_INTERVAL_MS 100

/**
 * The configuration parameters here will have the following format:
 *   LABEL
 * and
 *   CONFIG_LABEL_DEFAULT
 * It would be cumbersome for the user to type CONFIG and DEFAULT all the time, so the incoming macro is assumed
 * to be just LABEL. Hence, you will have a macro that can be used within the firmware:
 *
 *   #ifdef LABEL
 *     #define CONFIG_LABEL_DEFAULT LABEL
 *   #else
 *     #define CONFIG_LABEL_DEFAULT ...
 *   #endif
 *
 * In this way we will be able to quickly rewrite the code if there is a conflict. For example, prepending each
 * config macro with a Crownstone specific prefix (parallel to the Nordic prefix, NRF_, we might go for CS_ as
 * Crownstone abbreviation). If we use a short abbreviation we can have exactly the same macro for the user.
 *
 * There are values that indicate if parts of the code base will be compiled such as:
 *
 *   CS_MESH_COMPILED
 *
 *   CS_MESH_ENABLED
 *   CS_ENCRYPTION_ENABLED
 *   CS_IBEACON_ENABLED
 *   CS_SCANNER_ENABLED
 *   CS_POWER_SAMPLER_ENABLED
 *   CS_RELAY_START_STATE
 *   CS_PWM_ENABLED
 *
 * For now, a lot of these variables has not yet been made conform to the above naming scheme.
 */


#if defined MESHING
#define CONFIG_MESH_ENABLED_DEFAULT MESHING
#else
#define CONFIG_MESH_ENABLED_DEFAULT 1
#endif

#if defined ENCRYPTION
#define CONFIG_ENCRYPTION_ENABLED_DEFAULT ENCRYPTION
#else
#define CONFIG_ENCRYPTION_ENABLED_DEFAULT 1
#endif

#if defined IBEACON
#define CONFIG_IBEACON_ENABLED_DEFAULT IBEACON
#else
#define CONFIG_IBEACON_ENABLED_DEFAULT 1
#endif

#if defined INTERVAL_SCANNER_ENABLED
#define CONFIG_SCANNER_ENABLED_DEFAULT INTERVAL_SCANNER_ENABLED
#else
#define CONFIG_SCANNER_ENABLED_DEFAULT 0
#endif

#if defined PWM
#define CONFIG_PWM_DEFAULT PWM
#else
#define CONFIG_PWM_DEFAULT 0
#endif

#if defined CONFIG_START_DIMMER_ON_ZERO_CROSSING_DEFAULT
#else
#define CONFIG_START_DIMMER_ON_ZERO_CROSSING_DEFAULT 1
#endif

#if defined CONFIG_TAP_TO_TOGGLE_ENABLED_DEFAULT
#else
#define CONFIG_TAP_TO_TOGGLE_ENABLED_DEFAULT 0
#endif

#if defined CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_DEFAULT
#else
#define CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_DEFAULT -35
#endif

#if defined SWITCH_LOCK
#define CONFIG_SWITCH_LOCK_DEFAULT SWITCH_LOCK
#else
#define CONFIG_SWITCH_LOCK_DEFAULT 0
#endif

#if defined SWITCHCRAFT
#define CONFIG_SWITCHCRAFT_DEFAULT SWITCHCRAFT
#else
#define CONFIG_SWITCHCRAFT_DEFAULT 0
#endif

#if defined SPHERE_ID
#define CONFIG_SPHERE_ID_DEFAULT SPHERE_ID
#else
#define CONFIG_SPHERE_ID_DEFAULT 0
#endif

#if defined CROWNSTONE_ID
#define CONFIG_CROWNSTONE_ID_DEFAULT CROWNSTONE_ID
#else
#define CONFIG_CROWNSTONE_ID_DEFAULT 0
#endif

#if defined BOOT_DELAY
#define CONFIG_BOOT_DELAY_DEFAULT BOOT_DELAY
#else
#define CONFIG_BOOT_DELAY_DEFAULT 0
#endif

#if defined CONFIG_LOW_TX_POWER_DEFAULT
#else
#define CONFIG_LOW_TX_POWER_DEFAULT 0
#endif

#if defined CONFIG_VOLTAGE_MULTIPLIER_DEFAULT
#else
#define CONFIG_VOLTAGE_MULTIPLIER_DEFAULT 1
#endif

#if defined CONFIG_CURRENT_MULTIPLIER_DEFAULT
#else
#define CONFIG_CURRENT_MULTIPLIER_DEFAULT 1
#endif

#if defined CONFIG_VOLTAGE_ZERO_DEFAULT
#else
#define CONFIG_VOLTAGE_ZERO_DEFAULT 0
#endif

#if defined CONFIG_CURRENT_ZERO_DEFAULT
#else
#define CONFIG_CURRENT_ZERO_DEFAULT 0
#endif

#if defined CONFIG_POWER_ZERO_DEFAULT
#else
#define CONFIG_POWER_ZERO_DEFAULT 0
#endif
#define CONFIG_POWER_ZERO_INVALID 0x7FFFFFFF

#if defined CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP_DEFAULT
#else
#define CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP_DEFAULT 0
#endif

#ifndef CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN_DEFAULT
#define CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN_DEFAULT 0
#endif

#ifndef STATE_SWITCH_STATE_DEFAULT
#define STATE_SWITCH_STATE_DEFAULT 0
#endif

#ifndef STATE_ACCUMULATED_ENERGY_DEFAULT
#define STATE_ACCUMULATED_ENERGY_DEFAULT 0
#endif

#ifndef STATE_POWER_USAGE_DEFAULT
#define STATE_POWER_USAGE_DEFAULT 0
#endif

#ifndef STATE_RESET_COUNTER_DEFAULT
#define STATE_RESET_COUNTER_DEFAULT 0
#endif

//#ifndef STATE_SCHEDULE_DEFAULT
//#define STATE_SCHEDULE_DEFAULT 0
//#endif

#ifndef STATE_OPERATION_MODE_DEFAULT
#define STATE_OPERATION_MODE_DEFAULT 0
#endif

#ifndef STATE_TEMPERATURE_DEFAULT
#define STATE_TEMPERATURE_DEFAULT 0
#endif

#ifndef STATE_TIME_DEFAULT
#define STATE_TIME_DEFAULT 0
#endif

#ifndef STATE_FACTORY_RESET_DEFAULT
#define STATE_FACTORY_RESET_DEFAULT 0
#endif

#ifndef STATE_ERRORS_DEFAULT
#define STATE_ERRORS_DEFAULT 0
#endif
