#pragma once
#include <avr/io.h>
#include <stdint.h>

// UART0 (PD0 = RX, PD1 = TX), 8N1
static inline void uart_init(uint32_t baud = 9600) {
    uint16_t ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = ubrr >> 8;
    UBRR0L = ubrr & 0xFF;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
static inline void uart_putc(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}
static inline void uart_puts(const char *s) { while (*s) uart_putc(*s++); }
static inline bool uart_available() { return UCSR0A & (1 << RXC0); }
static inline char uart_getc() {
    while (!uart_available());
    return UDR0;
}
