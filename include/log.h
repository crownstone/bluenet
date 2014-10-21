/*
 * log.h
 *
 *  Created on: Oct 21, 2014
 *      Author: dominik
 */

#ifndef LOG_H_
#define LOG_H_

#include "uart.h"

/*****************************************************************************
* Logging and printing to UART
*****************************************************************************/

/**@brief Disable logging to UART by commenting out this line.*/
#define USE_UART_LOG_INFO   /* Enable to print standard output to UART. */
#define USE_UART_LOG_DEBUG  /* Enable to print standard output to UART. */

/**@brief Macro defined to output log data on the UART or not as user information __PRINT or debug __LOG,
                    based on the USE_UART_LOGGING and USE_UART_PRINTING flag.
                    If logging/printing is disabled, it will just yield a NOP instruction.
*/
#ifdef USE_UART_LOG_DEBUG
    #define LOG_DEBUG(F, ...) (uart_logf("DEBUG: %s: %d: " F "\r\n", __FILE__, __LINE__, ##__VA_ARGS__))
#else
    #define LOG_DEBUG(F, ...) (void)__NOP()
#endif
#ifdef USE_UART_LOG_INFO
    #define _LOG_INFO(F, ...) (uart_logf(F, ##__VA_ARGS__))
    #define LOG_INFO(F, ...) (uart_logf(F "\r\n", ##__VA_ARGS__))
#else
    #define LOG_INFO(F, ...) (void)__NOP()
#endif

#define NRF51_CRASH(x) __asm("BKPT");

#endif /* LOG_H_ */
