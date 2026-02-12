//
// Ugly software uart on PB4
//


#include <avr/io.h>
#include <util/delay_basic.h>
#include <avr/interrupt.h>
#include <stdint.h>

// #include "uart.h"

/*
 * Bit timing:
 * _delay_loop_2(n) takes 4*n cycles
 * 6667 cycles / 4 ≈ 1667
 */
#define UART_BIT_DELAY() _delay_loop_2(1667)

/* PB2 = physical pin 7 on ATtiny85 (SCK) */
#define UART_TX_DDR   DDRB
#define UART_TX_PORT  PORTB
#define UART_TX_PIN   PB2

#define to_hex(c) (((c)<10)?((c)+'0'):((c)-10+'A'))

void uart_tx_init(void)
{
    UART_TX_DDR  |= (1 << UART_TX_PIN);   // PB4 output
    UART_TX_PORT |= (1 << UART_TX_PIN);   // idle high
}

static inline void uart_tx_bit(uint8_t bit)
{
    if (bit)
        UART_TX_PORT |=  (1 << UART_TX_PIN);
    else
        UART_TX_PORT &= ~(1 << UART_TX_PIN);

    UART_BIT_DELAY();
}

void uart_tx_byte(uint8_t data)
{
    uint8_t i;

    cli();  // improve timing consistency

    /* Start bit (LOW) */
    uart_tx_bit(0);

    /* Data bits (LSB first) */
    for (i = 0; i < 8; i++) {
        uart_tx_bit(data & 0x01);
        data >>= 1;
    }

    /* Stop bit (HIGH) */
    uart_tx_bit(1);

    sei();
}

void uart_tx_string(const char *s)
{
    while (*s) {
        uart_tx_byte((uint8_t)*s++);
    }
}

void uart_tx_hex2( uint8_t c) {
  uart_tx_byte( to_hex((c>>4)&0xf));
  uart_tx_byte( to_hex(c&0xf));
}

void uart_tx_hex4( uint16_t i) {
  uart_tx_hex2( i>>8);
  uart_tx_hex2( i & 0xff);
}

void uart_tx_crlf(void) {
  uart_tx_byte(10);
  uart_tx_byte(13);
}
