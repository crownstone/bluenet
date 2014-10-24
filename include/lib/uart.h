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

#ifndef __UART_HPP__
#define __UART_HPP__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "nrf51.h"
#include "nrf51_bitfields.h"

/** Some definitions needed for the ported console functions */
#define UART_MODE_GENERIC_POLLING 0

/* Newline character sequences */
#define UART_NEWLINE_CRLF          "\r\n"
#define UART_NEWLINE_CR            "\r"
#define UART_NEWLINE_LF            "\n"
#define UART_NEWLINE_CRLF_NUMERIC  "\x0D\x0A"  /* Hardcoded ASCII values for CRLF */
#define UART_NEWLINE_CR_NUMERIC    "\x0D"      /* It is possible, though unlikely, that */
#define UART_NEWLINE_LF_NUMERIC    "\x0A"      /* \r and \n do not equal these values. */
/* Default newline style */
#define UART_NEWLINE_DEFAULT       UART_NEWLINE_CRLF_NUMERIC

/* Newline style for input */
#ifndef UART_NEWLINE_INPUT
  /* UART_NEWLINE_INPUT was not defined in console_config.h */
  #define UART_NEWLINE_INPUT      UART_NEWLINE_DEFAULT
#endif

/* Newline style for output */
#ifndef UART_NEWLINE_OUTPUT
  /* UART_NEWLINE_OUTPUT was not defined in console_config.h */
  #define UART_NEWLINE_OUTPUT     UART_NEWLINE_DEFAULT
#endif


/** Available Baud rates for UART. */
typedef enum
{
    UART_BAUD_2K4 = 0,        ///< 2400 baud
    UART_BAUD_4K8,            ///< 4800 baud
    UART_BAUD_9K6,            ///< 9600 baud
    UART_BAUD_14K4,           ///< 14.4 kbaud
    UART_BAUD_19K2,           ///< 19.2 kbaud
    UART_BAUD_28K8,           ///< 28.8 kbaud
    UART_BAUD_38K4,           ///< 38.4 kbaud
    UART_BAUD_57K6,           ///< 57.6 kbaud
    UART_BAUD_76K8,           ///< 76.8 kbaud
    UART_BAUD_115K2,          ///< 115.2 kbaud
    UART_BAUD_230K4,          ///< 230.4 kbaud
    UART_BAUD_250K0,          ///< 250.0 kbaud
    UART_BAUD_500K0,          ///< 500.0 kbaud
    UART_BAUD_1M0,            ///< 1 mbaud
    UART_BAUD_TABLE_MAX_SIZE  ///< Used to specify the size of the baudrate table.
} uart_baudrate_t;

/** @brief The baudrate devisors array, calculated for standard baudrates.
    Number of elements defined by ::HAL_UART_BAUD_TABLE_MAX_SIZE*/
#define UART_BAUDRATE_DEVISORS_ARRAY    { \
    0x0009D000, 0x0013B000, 0x00275000, 0x003BA000, 0x004EA000, 0x0075F000, 0x009D4000, \
    0x00EBE000, 0x013A9000, 0x01D7D000, 0x03AFB000, 0x03FFF000, 0x075F6000, 0x10000000  }

static const uint32_t m_baudrates[UART_BAUD_TABLE_MAX_SIZE] = UART_BAUDRATE_DEVISORS_ARRAY;

/** @brief This function initializes the UART.
 *
 * @param baud_rate - the baud rate to be used, ::uart_baudrate_t.
 *
 */
inline void uart_init(uart_baudrate_t const baud_rate) {
	//  NRF_UART0->PSELRTS = BOARD_UART0_RTS;
	//  NRF_UART0->PSELTXD = BOARD_UART0_TX;
	//  NRF_UART0->PSELCTS = BOARD_UART0_CTS;
	//  NRF_UART0->PSELRXD = BOARD_UART0_RX;

	NRF_UART0->PSELRXD = 16;
	NRF_UART0->PSELTXD = 17;
	//  NRF_UART0->PSELRTS = 18;
	//  NRF_UART0->PSELCTS = 19;

	NRF_UART0->ENABLE           = 0x04;
	NRF_UART0->BAUDRATE         = m_baudrates[baud_rate];
	NRF_UART0->CONFIG           = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
	NRF_UART0->TASKS_STARTTX    = 1;
	NRF_UART0->TASKS_STARTRX    = 1;
	NRF_UART0->EVENTS_RXDRDY    = 0;
}

/** @brief Find number of characters in the UART receive buffer
 *
 * This function returns the number of characters available for reading
 * in the UART receive buffer.
 *
 * @return Number of characters available
 */
inline bool uart_chars_available(void) {
	return (NRF_UART0->EVENTS_RXDRDY == 1);
}

/** Function to write a character to the UART transmit buffer.
 * @param ch Character to write
 */
inline void uart_putchar(uint8_t ch) {
	NRF_UART0->EVENTS_TXDRDY = 0;
	NRF_UART0->TXD = ch;
	while(NRF_UART0->EVENTS_TXDRDY != 1){}  // Wait for TXD data to be sent
	NRF_UART0->EVENTS_TXDRDY = 0;
}

/** Function to read a character from the UART receive buffer.
 * @return Character read
 */
inline uint8_t uart_getchar(void) {
	while(NRF_UART0->EVENTS_RXDRDY != 1){}  // Wait for RXD data to be received
	NRF_UART0->EVENTS_RXDRDY = 0;
	return (uint8_t)NRF_UART0->RXD;
}


/** Function to write and array of byts to UART.
 *
 * @param buf - buffer to be printed on the UART.
 * @param len - Number of bytes in buffer.
 */
inline void uart_write_buf(uint8_t const *buf, uint32_t len) {
	while(len--)
		uart_putchar(*buf++);
}

/**@brief Logging function, used for formated output on the UART.
 */
inline void uart_logf(const char *fmt, ...) {
    uint16_t i = 0;
    static uint8_t logf_buf[150];
    va_list args;
    va_start(args, fmt);
    i = vsprintf((char*)logf_buf, fmt, args);
    logf_buf[i] = 0x00; /* Make sure its zero terminated */
    uart_write_buf((uint8_t*)logf_buf, i);
    va_end(args);
}

#endif // __UART_HPP__
