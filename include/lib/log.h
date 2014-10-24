/* Copyright (c) 2014, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   * Neither the name of Nordic Semiconductor ASA nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef LOG_HPP_
#define LOG_HPP_

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

#endif /* LOG_HPP_ */
