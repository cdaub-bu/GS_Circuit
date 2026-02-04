/*
 * test the light sensor
 * 
 */

// #define USE_UART
#define USE_ACOMP

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#ifdef USE_UART
#include "uart.h"
#endif

#ifdef USE_ACOMP
#include "acomp.h"
#endif

#define LED_DDR DDRB
#define LED_BIT 3
#define LED2_BIT 4
#define LED_MASK (_BV(LED_BIT)|_BV(LED2_BIT))
#define LED_PORT PORTB

#define SW_DDR DDRB
#define SW_BIT 0
#define SW_PORT PORTB
#define SW_PIN PINB

uint8_t dim[] = { 0, 1, 2, 5, 10, 20, 50, 99 };
#define NDIM (sizeof(dim)/sizeof(dim[0]))
static int level;

static int bounce;
#define DEBOUNCE 50

static uint16_t adcv;

int main (void)
{
  SW_PORT |= _BV( SW_BIT);	/* enable pull-up */
  LED_DDR |= LED_MASK;

#ifdef USE_UART
  uart_tx_init();
#endif
#ifdef USE_ACOMP
  acomp_init();
#endif  

  while(1) {
    if( acomp_read()) {
      LED_PORT |= _BV(LED_BIT);
      LED_PORT &= ~_BV(LED2_BIT);
    } else {
      LED_PORT &= ~_BV(LED_BIT);
      LED_PORT |= _BV(LED2_BIT);
    }      
  }
}


