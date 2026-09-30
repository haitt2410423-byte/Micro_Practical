#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "uart.h"

#define ANGLE_MIN 0
#define ANGLE_MAX 180
#define LINE_SIZE 16

// ---------- Servo: Timer1, OC1A = PB1, chu kỳ 20 ms ----------
static void servo_init() {
    DDRB  |= (1 << PB1);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1   = 39999;
    OCR1A  = 3000;                                   // vị trí giữa
}
static void servo_angle(uint8_t deg) {
    OCR1A = 2000 + (uint32_t)deg * 2000UL / 180UL;   // 1 ms .. 2 ms
}

// Đọc một dòng (kết thúc bằng \r hoặc \n), bỏ qua dòng trống
static void read_line(char *buf, uint8_t size, bool &overflow) {
    uint8_t n = 0;
    overflow = false;
    while (true) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') {
            if (n == 0 && !overflow) continue;
            buf[n] = '\0';
            return;
        }
        if (n < size - 1) buf[n++] = c;
        else overflow = true;
    }
}

int main() {
    uart_init();
    servo_init();
    uart_puts("Servo ready. Send angle 0-180 + Enter\r\n");

    char line[LINE_SIZE], msg[56];
    while (true) {
        bool overflow;
        read_line(line, sizeof line, overflow);

        char *end;
        long v = strtol(line, &end, 10);

        if (overflow || end == line || *end != '\0') {
            uart_puts("ERR: invalid command (send an integer, e.g. 90)\r\n");
        } else if (v < ANGLE_MIN || v > ANGLE_MAX) {
            snprintf(msg, sizeof msg, "ERR: angle %ld out of range (%d-%d)\r\n",
                     v, ANGLE_MIN, ANGLE_MAX);
            uart_puts(msg);
        } else {
            servo_angle((uint8_t)v);
            snprintf(msg, sizeof msg, "OK: servo moved to %ld deg\r\n", v);
            uart_puts(msg);
        }
    }
}
