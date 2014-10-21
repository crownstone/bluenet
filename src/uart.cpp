#include "uart.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"

static const uint32_t m_baudrates[UART_BAUD_TABLE_MAX_SIZE] = UART_BAUDRATE_DEVISORS_ARRAY;

void uart_init(uart_baudrate_t const baud_rate)
{
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

bool hal_uart_chars_available(void)
{
	return (NRF_UART0->EVENTS_RXDRDY == 1);
}

void sendchar(uint8_t ch)
{
	uart_putchar(ch);
}

void uart_putchar(uint8_t ch)
{
	NRF_UART0->EVENTS_TXDRDY = 0;
	NRF_UART0->TXD = ch;
	while(NRF_UART0->EVENTS_TXDRDY != 1){}  // Wait for TXD data to be sent
	NRF_UART0->EVENTS_TXDRDY = 0;
}

uint8_t uart_getchar(void)
{
	while(NRF_UART0->EVENTS_RXDRDY != 1){}  // Wait for RXD data to be received
	NRF_UART0->EVENTS_RXDRDY = 0;
	return (uint8_t)NRF_UART0->RXD;
}

void uart_write_buf(uint8_t const *buf, uint32_t len)
{
	while(len--)
		uart_putchar(*buf++);
}

void uart_logf(const char *fmt, ...)
{
    uint16_t i = 0;
    static uint8_t logf_buf[150];
    va_list args;
    va_start(args, fmt);
    i = vsprintf((char*)logf_buf, fmt, args);
    logf_buf[i] = 0x00; /* Make sure its zero terminated */
    uart_write_buf((uint8_t*)logf_buf, i);
    va_end(args);
}

/*
#include "uart.h"
#include "nRF51822.h"
#include <stdarg.h>
#include <cstdio>

#define NRF51_UART_9600_BAUD  0x00275000UL
#define NRF51_UART_38400_BAUD 0x009D5000UL

UART::UART() {
	NRF51_GPIO_DIR_OUTPUT(17); // set pins to output
	NRF51_GPIO_PIN_CNF(16) = NRF51_GPIO_PIN_CNF_PULL_DISABLED;
	NRF51_GPIO_DIR_INPUT(16);
	//NRF51_GPIO_DIRSET = 3<<8;
	NRF51_UART_ENABLE = 0x04; // 0b00000100

	// Configure UART pins:    GPIO   UART
	NRF51_UART_PSELRXD = 16;  	// P0.11  RXD //16
	NRF51_UART_PSELTXD = 17;   	// P0.09  TXD //17
	//NRF51_UART_PSELRTS = 18;   	// P0.08  RTS //18
	//NRF51_UART_PSELCTS = 19;  	// P0.10  CTS //19

	//NRF51_UART_CONFIG = NRF51_UART_CONFIG_HWFC_ENABLED; // enable hardware flow control.
	NRF51_UART_BAUDRATE = NRF51_UART_38400_BAUD;
	NRF51_UART_STARTTX = 1;
	NRF51_UART_STARTRX = 1;
	NRF51_UART_RXDRDY = 0;
	NRF51_UART_TXDRDY = 0;
}

size_t UART::write(uint8_t b) {
	NRF51_UART_TXD = b;
	while(NRF51_UART_TXDRDY != 1);
	NRF51_UART_TXDRDY = 0;
	return size_t(1);
}

void UART::printf(const char* fmt, ...) {
	char buf[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr); // does not overrun sizeof(buf) including null terminator
	Print::print(buf);
	va_end(argptr);
}

void UART::printfln(const char* fmt, ...) {
	char buf[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr); // does not overrun sizeof(buf) including null terminator
	Print::print(buf);
	va_end(argptr);

	Print::println();
}
 */
