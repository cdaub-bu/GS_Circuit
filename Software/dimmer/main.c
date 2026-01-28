/*
 * simple dimmer example
 * use timer 0 interrupt at 10kHz to generate 0-99% at 100Hz PWM rate
 */

// #define USE_UART

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "timer_10kHz.h"
#include "adc.h"

#ifdef USE_UART
#include "uart.h"
#endif

// Arduino LED is on PB5

#define LED_DDR DDRB
#define LED_BIT 3
#define LED_PORT PORTB

#define SW_DDR DDRB
#define SW_BIT 0
#define SW_PORT PORTB
#define SW_PIN PINB

volatile uint16_t tick;
volatile int8_t pct = 1;

#define PERIOD 100

// Timer0 Compare Match A ISR (10 kHz)
// dim the 
ISR(TIMER0_COMPA_vect)
{
  ++tick;
  if( tick == PERIOD)
    tick = 0;
  if( tick < pct)
    LED_PORT |= (1 << LED_BIT);
  else
    LED_PORT &= ~(1 << LED_BIT);
}

uint8_t dim[] = { 0, 1, 2, 5, 10, 20, 50, 99 };
#define NDIM (sizeof(dim)/sizeof(dim[0]))
static int level;

static int bounce;
#define DEBOUNCE 50

static uint16_t adcv;

int main (void)
{
  timer0_init_10kHz();
  adc_init();
#ifdef USE_UART
  uart_tx_init();
#endif

  SW_PORT |= _BV( SW_BIT);	/* enable pull-up */

  LED_DDR |= (1 << LED_BIT);
  bounce = DEBOUNCE;
  level = 0;
  pct = 0;

  while( 1) {
    _delay_ms(1000);
    if( bounce)
      --bounce;
    else {
      if( (SW_PIN & _BV(SW_BIT)) == 0) { /* switch pressed */
	bounce = DEBOUNCE;
	++level;
	if( level >= NDIM)
	  level = 0;
      }
    }
    adcv = adc_read(1);

#ifdef USE_UART
    uart_tx_byte( adcv);
    uart_tx_byte( adcv >> 8);
#endif
    
    if( adcv >= 0x300)
      pct = 1;
    else
      pct = dim[level];
  }
}


