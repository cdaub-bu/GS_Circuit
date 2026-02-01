/*
 * Initial version of GSA operating software
 *
 * use timer 0 interrupt at 10kHz to generate 0-99% at 100Hz PWM rate
 */

// NC dimmer button as on PCB
#define SW_NC

// goto sleep and turn off after this many ticks of 100Hz
// now a 32-bit value, so 1 hr is e.g. (100*60*60)
#define SLEEP_TIME 6000

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include "timer_10kHz.h"

#include "acomp.h"
#include "sleep.h"
#include "wdt.h" 

// LEDs on PB3 and PB4
#define LED_DDR DDRB
#define LED_BIT 3
#define LED2_BIT 4
#define LED_MASK (_BV(LED_BIT)|_BV(LED2_BIT))
#define LED_PORT PORTB

#define SW_DDR DDRB
#define SW_BIT 0
#define SW_PORT PORTB
#define SW_PIN PINB

volatile uint8_t tick;
volatile uint32_t long_tick;
volatile int8_t pct = 1;

#define PERIOD 100

// Timer0 Compare Match A ISR (10 kHz)
// dim the 
ISR(TIMER0_COMPA_vect)
{
  ++tick;
  if( tick == PERIOD) {
    tick = 0;
    ++long_tick;
  }
  if( tick < pct)
    LED_PORT |= LED_MASK;
  else
    LED_PORT &= ~LED_MASK;
}

ISR(PCINT0_vect)
{
    // Executed after wake-up
    // Keep this ISR short
}

ISR(WDT_vect) {
  ;
}


uint8_t dim[] = { 0, 1, 2, 5, 10, 20, 50, 99 };
#define NDIM (sizeof(dim)/sizeof(dim[0]))
static int level;

static int bounce;
#define DEBOUNCE 50

static uint16_t adcv;

void flash_led( int n) {
      // flash the LED
  for( int i=0; i<n; i++) {
    pct = dim[3];
    _delay_ms(25);
    pct = 0;
    _delay_ms(25);
  }
}

int main (void)
{
  SW_PORT |= _BV( SW_BIT);	/* enable pull-up */
  LED_DDR |= LED_MASK;

  // turn off BOD in sleep to reduce power
  MCUCR |= _BV(BODS) | _BV(BODSE);
  MCUCR |= _BV(BODS);
  power_adc_disable();  

  timer0_init_10kHz();
  setup_wdt();

  acomp_init();
  long_tick = 0;

  bounce = DEBOUNCE;
  level = 1;
  pct = 0;

  while( 1) {
    _delay_ms(3);

    if( bounce)
      --bounce;
    else {
#ifdef SW_NC
      if( (SW_PIN & _BV(SW_BIT)) != 0) { /* switch pressed */
#else      
      if( (SW_PIN & _BV(SW_BIT)) == 0) { /* switch pressed */
#endif
	bounce = DEBOUNCE;
	++level;
	if( level >= NDIM)
	  level = 0;
      }
    }

    if( acomp_read())
      pct = 0;
    else
      pct = dim[level];

    if( long_tick > SLEEP_TIME) {

      flash_led(10);

      // go to sleep
      long_tick = 0;
      sleep_init();

      set_sleep_mode( SLEEP_MODE_PWR_DOWN);
      sleep_enable();
      sleep_cpu();

      // ---- SLEEP ----


      // come here on wake-up
      sleep_disable();
      power_all_enable();
      power_adc_disable();

      long_tick = 0;
      pct = dim[level];

      // turn the comparator back on
      ACSR &= ~_BV(ACD);
      acomp_init();

      flash_led(5);
    }
    
  }
}


