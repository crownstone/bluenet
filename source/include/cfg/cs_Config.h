/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 4 Nov., 2014
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#define CROWNSTONE_COMPANY_ID                    0x038E

#define CS_CONNECTION_PROTOCOL_VERSION           5

// size of the buffer used for characteristics
//#define GENERAL_BUFFER_SIZE                      300

/** maximum length of strings used for characteristic values */
#define DEFAULT_CHAR_VALUE_STRING_LENGTH         25

/** define the maximum size for strings to be stored
 */
#define MAX_STRING_STORAGE_SIZE                  31


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
#if (NORDIC_SDK_VERSION == 16)
// TODO: only this big because something fills up the scheduler during connection.
#define SCHED_QUEUE_SIZE                         512
#else
#define SCHED_QUEUE_SIZE                         32
#endif


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

// Max number of schedule entries in the schedule service.
#define MAX_SCHEDULE_ENTRIES                     10

// How many seconds of time jump is regarded a big time jump
#define SCHEDULE_BIG_TIME_JUMP                   (75*60)

// How many channels the pwm can control. Limited by the amount of timer channels.
#define CS_PWM_MAX_CHANNELS                      2

// ----- TIMERS -----
// Soft device uses timer 0
// Mesh uses timer 3 (BEARER_ACTION_TIMER_INDEX)
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



// ----- PPI -----
// Soft device uses 17-31
// Mesh SDK uses 8-11
#define CS_PWM_PPI_CHANNEL_START                 0
#define CS_PWM_PPI_CHANNEL_COUNT                 (1 + 2 * CS_PWM_MAX_CHANNELS)
//#define CS_ADC_PPI_CHANNEL_START                 (CS_PWM_PPI_CHANNEL_START + CS_PWM_PPI_CHANNEL_COUNT)
#define CS_ADC_PPI_CHANNEL_START                 12
#define CS_ADC_PPI_CHANNEL_COUNT                 2

// ----- PPI groups -----
// Soft device uses 4-5
#define CS_PWM_PPI_GROUP_START                   0
#define CS_PWM_PPI_GROUP_COUNT                   0
#define CS_ADC_PPI_GROUP_START                   (CS_PWM_PPI_GROUP_START + CS_PWM_PPI_GROUP_COUNT)
#define CS_ADC_PPI_GROUP_COUNT                   0

// ----- GPIOTE -----
#define CS_ADC_GPIOTE_CHANNEL_START              0
#define CS_ADC_GPIOTE_CHANNEL_COUNT              1 // Actually only used for debug
#define CS_PWM_GPIOTE_CHANNEL_START              (CS_ADC_GPIOTE_CHANNEL_START + CS_ADC_GPIOTE_CHANNEL_COUNT)
#define CS_PWM_GPIOTE_CHANNEL_COUNT              (CS_PWM_MAX_CHANNELS)

// ----- SAADC -----
#define CS_ADC_RESOLUTION                        NRF_SAADC_RESOLUTION_12BIT // Don't change without changing code.
#define CS_ADC_IRQ_PRIORITY                      APP_IRQ_PRIORITY_HIGH
#define CS_ADC_IRQ                               SAADC_IRQHandler

// ----- WDT -----
#define CS_WATCHDOG_PRIORITY                     APP_IRQ_PRIORITY_HIGH
#define CS_WATCHDOG_TIMEOUT_MS                   60000


#define CS_ADC_SAMPLE_INTERVAL_US                200 // 100 samples per period of 50Hz wave
#define CS_ADC_NUM_CHANNELS                      2 // Number of channels (pins) to sample.
#define CS_ADC_NUM_SAMPLES_PER_CHANNEL           (20000 / CS_ADC_SAMPLE_INTERVAL_US) // Make size so it fills up 20ms of data.
//#define CS_ADC_BUF_SIZE                          (CS_ADC_NUM_CHANNELS * CS_ADC_NUM_SAMPLES_PER_CHANNEL)

#define CS_ADC_NUM_BUFFERS                       9 // 5 buffers are held by processing, 2 queued in SAADC, 1 moves between them, 1 extra for CPU usage peaks.
#define CS_ADC_TIMEOUT_SAMPLES                   2 // Timeout when no buffer has been set at N samples before end of interval.


// Buffer size for storage requests. Storage requests get buffered when the device is scanning or meshing.
#define STORAGE_REQUEST_BUFFER_SIZE              5 // Should be at least 3, because setup pushes 3 storage requests (configs + operation mode + switch state).

#define FACTORY_RESET_CODE                       0xdeadbeef
#define FACTORY_RESET_TIMEOUT                    60000 // Timeout before recovery becomes unavailable after reset (ms)
#define FACTORY_PROCESS_TIMEOUT                  200 // Timeout before recovery process step is executed (ms)

#define ENCRYPTION_KEY_LENGTH                    16 // 16 byte length, just like SOC_ECB_KEY_LENGTH

#define BROWNOUT_TRIGGER_THRESHOLD               NRF_POWER_THRESHOLD_V27

// Octave: a=0.05; x=[0:1000]; y=(1-a).^x; y2=cumsum(y)*a; figure(1); plot(x,y); figure(2); plot(x,y2); find(y2 > 0.99)(1)
// Python: d=0.1; x=range(0,1000); y=np.power(1.0 - d, x); y2=np.cumsum(y) * d; indices=np.where(y2 > 0.99); print(indices[0][0])
#define VOLTAGE_ZERO_EXP_AVG_DISCOUNT            20  // Is divided by 1000, so 20 is a discount of 0.02. // 99% of the average is influenced by the last 228 values
//#define VOLTAGE_ZERO_EXP_AVG_DISCOUNT            1000 // No averaging
#define CURRENT_ZERO_EXP_AVG_DISCOUNT            100 // Is divided by 1000, so 100 is a discount of 0.1. // 99% of the average is influenced by the last 44 values
//#define CURRENT_ZERO_EXP_AVG_DISCOUNT            1000 // No averaging
#define POWER_EXP_AVG_DISCOUNT                   200 // Is divided by 1000, so 200 is a discount of 0.2. // 99% of the average is influenced by the last 21 values
//#define POWER_EXP_AVG_DISCOUNT                   1000 // No averaging
#define POWER_SAMPLING_RMS_WINDOW_SIZE           9 // Windows size used for filtering the power and current rms. Currently can only be 7, 9, or 25!

#define POWER_SAMPLING_CURVE_HALF_WINDOW_SIZE    5 // Half window size used for filtering the current curve. Can't just be any value!
//#define POWER_SAMPLING_CURVE_HALF_WINDOW_SIZE    16 // Half window size used for filtering the current curve. Can't just be any value!


#define POWER_DIFF_THRESHOLD_PART                0.10f  // When difference is 10% larger or smaller, consider it a significant change.
#define POWER_DIFF_THRESHOLD_MIN_WATT            10.0f  // But the difference must also be at least so many Watts.
#define NEGATIVE_POWER_THRESHOLD_WATT            -10.0f // Only if power is below threshold, it may be negative.
#define POWER_DIFF_THRESHOLD_PART_CS_ZERO        0.10f  // Same, but for plug zero or builtin zero (less accurate power measurements).
#define POWER_DIFF_THRESHOLD_MIN_WATT_CS_ZERO    15.0f  // Same, but for plug zero or builtin zero (less accurate power measurements).
#define NEGATIVE_POWER_THRESHOLD_WATT_CS_ZERO    -20.0f // Same, but for plug zero or builtin zero (less accurate power measurements).

#define CURRENT_USAGE_THRESHOLD                  (16000) // Power usage threshold in mA at which the switch should be turned off.
#define CURRENT_USAGE_THRESHOLD_DIMMER           (1000)  // Power usage threshold in mA at which the PWM should be turned off.
#define CURRENT_THRESHOLD_CONSECUTIVE            100 // Number of consecutive times the current has to be above the threshold before triggering the softfuse.
#define CURRENT_THRESHOLD_DIMMER_CONSECUTIVE     20  // Number of consecutive times the current has to be above the threshold before triggering the softfuse.


#define SWITCHCRAFT_THRESHOLD                    (500000) // Threshold for switch recognition (float).

#define PWM_PERIOD                               10000L // Interval in us: 1/10000e-6 = 100 Hz

#define SWITCH_DELAYED_STORE_MS                  (10 * 1000) // Timeout before storing the pwm switch value is stored.
#define STATE_RETRY_STORE_DELAY_MS               200 // Time before retrying to store a varable to flash.
#define MESH_SEND_TIME_INTERVAL_MS               (50 * 1000) // Interval at which the time is sent via the mesh.
#define MESH_SEND_TIME_INTERVAL_MS_VARIATION     (20 * 1000) // Max amount that gets added to interval.
#define MESH_SEND_STATE_INTERVAL_MS              (50 * 1000) // Interval at which the stone state is sent via the mesh.
#define MESH_SEND_STATE_INTERVAL_MS_VARIATION    (20 * 1000) // Max amount that gets added to interval.
#define MESH_SYNC_RETRY_INTERVAL_MS              (2500)
#define MESH_SYNC_GIVE_UP_MS                     (60 * 1000) // After some time, give up syncing.
#define CS_MESH_DEFAULT_TTL                      10

#define PWM_BOOT_DELAY_MS                        60000 // Delay after boot until pwm can be used. Has to be smaller than overflow time of RTC.
#define DIMMER_BOOT_CHECK_DELAY_MS               5000  // Delay after boot until power measurement is checked to see if dimmer works.
#define DIMMER_BOOT_CHECK_POWER_MW               3000  // Threshold in milliWatt above which the dimmer is considered to be working.
#define DIMMER_BOOT_CHECK_POWER_MW_UNCALIBRATED  10000 // Threshold in milliWatt above which the dimmer is considered to be working, in case power zero is not calibrated yet.
#define DIMMER_SOFT_ON_SPEED                     8     // Speed of the soft on feature.

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
 * - https://devzone.nordicsemi.com/f/nordic-q-a/10636/what-are-latency-and-supervision-timeout-limits
 */
#define CONNECTION_SUPERVISION_TIMEOUT           100 // In units of 10ms.

/*
 * By setting a non-zero slave latency, the Peripheral can choose to not answer when the Central asks for data up to
 * the slave latency number of times. However, if the Peripheral has data to send, it can choose to send data at any
 * time. This enables a peripheral to stay sleeping for a longer time, if it doesn't have data to send, but still send
 * data fast if needed. The text book example of such device is for example keyboard and mice, which want to be
 * sleeping for as long as possible when there is no data to send, but still have low latency (and for the mouse:
 * low connection interval) when needed.
 *
 * Slave latency increases delay of data from central to peripheral, but not from peripheral to central.
 * - https://devzone.nordicsemi.com/f/nordic-q-a/53230/what-is-the-impact-of-slave-latencyin-ble
 *
 * Timeout will be CONNECTION_SUPERVISION_TIMEOUT * SLAVE_LATENCY.
 * - https://devzone.nordicsemi.com/question/14029/slave-latency-for-s110s120-connection/
 */
#define SLAVE_LATENCY                            0

#define ADVERTISING_TIMEOUT                      0
#define ADVERTISING_REFRESH_PERIOD               500 // Push the changes in the advertisement packet to the stack every x milliseconds
#define ADVERTISING_REFRESH_PERIOD_SETUP         500 // Push the changes in the advertisement packet to the stack every x milliseconds

#define EXTERNAL_STATE_LIST_COUNT                10 // Number of stones to cache the state of, for advertising external state.
#define EXTERNAL_STATE_TIMEOUT_MS                60000 // Time after which a state of another stone is considered to be timed out.

#define SWITCH_ON_AT_SETUP_BOOT_DELAY            3600  // Seconds until the switch turns on when in setup mode (Crownstone built-in only)

#define SUN_TIME_THROTTLE_PERIOD_SECONDS         (60*60*24) // Seconds to throttle writing the sun time to flash.

#define CS_CLEAR_GPREGRET_COUNTER_TIMEOUT_S      60 // Seconds after boot to clear the GPREGRET reset counter.

/**
 * Interval in milliseconds at which tick events are dispatched.
 */
#define TICK_INTERVAL_MS 100

#define CONFIG_POWER_ZERO_INVALID 0x7FFFFFFF

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

#ifndef STATE_OPERATION_MODE_DEFAULT
#define STATE_OPERATION_MODE_DEFAULT 0
#endif

#ifndef STATE_TEMPERATURE_DEFAULT
#define STATE_TEMPERATURE_DEFAULT 0
#endif

#ifndef STATE_FACTORY_RESET_DEFAULT
#define STATE_FACTORY_RESET_DEFAULT 0
#endif

#ifndef STATE_ERRORS_DEFAULT
#define STATE_ERRORS_DEFAULT 0
#endif

#ifndef STATE_BEHAVIOUR_SETTINGS_DEFAULT
#define STATE_BEHAVIOUR_SETTINGS_DEFAULT 1 // Enabled (bit 0) is true by default.
#endif

#ifndef STATE_BEHAVIOUR_MASTER_HASH_DEFAULT
#define STATE_BEHAVIOUR_MASTER_HASH_DEFAULT 0
#endif

#ifndef STATE_MESH_IV_INDEX_DEFAULT
#define STATE_MESH_IV_INDEX_DEFAULT 0
#endif

#ifndef STATE_MESH_IV_STATUS_DEFAULT
#define STATE_MESH_IV_STATUS_DEFAULT 0 // NET_STATE_IV_UPDATE_NORMAL
#endif

#ifndef STATE_MESH_SEQ_NUMBER_DEFAULT
#define STATE_MESH_SEQ_NUMBER_DEFAULT 0
#endif

#ifndef STATE_HUB_MODE_DEFAULT
#define STATE_HUB_MODE_DEFAULT 0
#endif


