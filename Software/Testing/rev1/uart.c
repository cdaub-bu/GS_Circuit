//
// Ugly software uart on PB4
//


#include <avr/io.h>
#include <util/delay_basic.h>
#include <avr/interrupt.h>

// #include "uart.h"

/*
 * Bit timing:
 * _delay_loop_2(n) takes 4*n cycles
 * 6667 cycles / 4 ≈ 1667
 */
#define UART_BIT_DELAY() _delay_loop_2(1667)

/* PB4 = physical pin 3 on ATtiny85 */
#define UART_TX_DDR   DDRB
#define UART_TX_PORT  PORTB
#define UART_TX_PIN   PB4

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
