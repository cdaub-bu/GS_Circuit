/*
 * Initial version of GSA operating software
 *
 * use timer 0 interrupt at 10kHz to generate 0-99% at 100Hz PWM rate
 *
 * Implemented as an FSM with the following states:
 *
 * WAKE_DIM     - dimmer button pressed.
 *                lights on for DIM_TIME seconds
 *                goto SLEEP_LIGHT or WAKE_ILLUM depending on ambient light
 *
 * SLEEP_LIGHT  - ambient lights on, sleep and wake every 1s to check
 *                goto WAKE_ILLUM if ambient lights off
 *                goto WAKE_DIM on button press
 *
 * SLEEP_DARK   - ambient lights off, timed-out, sleep until ambient lights on
 *                goto SLEEP_LIGHT if ambient lights on
 *                goto WAKE_DIM on button press
 *
 * WAKE_ILLUM   - ambient lights off, night lights on until time-out
 *                on time-out goto SLEEP_DARK
 *                goto WAKE_DIM on button press
 */

// NC dimmer button as on PCB
#define SW_NC

// rate of long_tick/sec
#define TICKS_SEC 100

// goto sleep and turn off after this many ticks of 100Hz
// now a 32-bit value, so 1 hr is e.g. (60*60*TICKS_SEC)
#define SLEEP_TIME 10*TICKS_SEC

// time lights stay on when button pressed
#define DIM_TIME 5*TICKS_SEC

// time delay before switching from LIGHT to DARK and vice-versa
#define LIGHT_DARK_TIME 2*TICKS_SEC

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
//#define LED_MASK (_BV(LED_BIT)|_BV(LED2_BIT))
#define LED_MASK (_BV(LED_BIT))
#define LED2_MASK (_BV(LED2_BIT))
#define LED_PORT PORTB

#define SW_DDR DDRB
#define SW_BIT 0
#define SW_PORT PORTB
#define SW_PIN PINB

volatile uint8_t tick = 0;
volatile uint32_t long_tick = 0;
volatile int8_t pct = 1;
volatile int8_t wdtick = 0;

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
  ++wdtick;
}


uint8_t dim[] = { 0, 1, 2, 5, 10, 20, 50, 99 };
#define NDIM (sizeof(dim)/sizeof(dim[0]))
static int level;

static int bounce;
#define DEBOUNCE 12

static uint16_t adcv;

void flash_led( int n) {
      // flash the LED
  for( int i=0; i<n; i++) {
    pct = dim[3];
    _delay_ms(50);
    pct = 0;
    _delay_ms(50);
  }
  _delay_ms(200);
}

int acomp_read(void) {
  uint8_t ac = (ACSR & _BV(ACO)) != 0;
  if( ac)
    LED_PORT |= LED2_MASK;
  else
    LED_PORT &= ~LED2_MASK;
  return ac;
}


typedef enum {
  WAKE_DIM,
  SLEEP_LIGHT,
  SLEEP_DARK,
  WAKE_ILLUM
} a_state;

int main (void)
{
  static a_state state;
  static uint32_t last_long;
  static uint8_t button;
  static uint8_t last_button;

  static uint16_t light_secs;

  SW_PORT |= _BV( SW_BIT);	/* enable pull-up */
  LED_DDR |= LED_MASK | LED2_MASK;

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

  state = WAKE_DIM;

  while( 1) {

    // wait for one tick to elapse
    while( long_tick == last_long)
      ;
    last_long = long_tick;

    // check for and process button presses
    if( bounce) {
      --bounce;
    } else {
      bounce = DEBOUNCE;
      last_button = button;
      button = ((SW_PIN & _BV(SW_BIT)) != 0);
      state = WAKE_DIM;
      if( button && !last_button) {
	++level;
	if( level >= NDIM)
	  level = 1;		/* don't allow 0 percent */
	pct = dim[level];
	long_tick = 0;
      }
    }
    
    switch( state) {

    // got here because lights were on
    // goto WAKE_ILLUM if the lights are now off
    case SLEEP_LIGHT:

      long_tick = 0;

      // go to sleep
      sleep_init();

      set_sleep_mode( SLEEP_MODE_PWR_DOWN);
      sleep_enable();
      sleep_cpu();

      // ---- SLEEP ---

      // wake up
      sleep_disable();
      power_all_enable();
      power_adc_disable();

      // turn the comparator back on
      ACSR &= ~_BV(ACD);
      acomp_init();
      _delay_ms(1);

      // figure out how we woke up and take action
      if( wdtick) {		/* WDT tick wakeup */

	wdtick = 0;
	flash_led(1);

	// check ambient light
	if( acomp_read()) {	/* it's light out */
	  pct = 0;		/* stay in SLEEP_LIGHT mode */
	} else {		/* it's dark out */
	    state = WAKE_ILLUM;
	    pct = dim[level];
	}
	
      } else {			/* pin change wakeup */

	flash_led(2);
	state = WAKE_DIM;	/* go to "dimmer control" state */
	long_tick = 0;
	
      }

      break;

    case SLEEP_DARK:
      // lights off, timed-out
      // goto SLEEP_LIGHT if ambient lights on
      // goto WAKE_DIM on button press
      
      long_tick = 0;

      // go to sleep
      sleep_init();

      set_sleep_mode( SLEEP_MODE_PWR_DOWN);
      sleep_enable();
      sleep_cpu();

      // ---- SLEEP ---

      // wake up
      sleep_disable();
      power_all_enable();
      power_adc_disable();

      // turn the comparator back on
      ACSR &= ~_BV(ACD);
      acomp_init();
      _delay_ms(1);

      // figure out how we woke up and take action
      if( wdtick) {		/* WDT tick wakeup */

	wdtick = 0;
	flash_led(3);

	// check ambient light
	if( acomp_read()) {	/* it's light out */
	  pct = 0;		/* go to SLEEP_LIGHT mode */
	  state = SLEEP_LIGHT;
	}
	// else, stay in SLEEP_DARK

      } else {			/* pin change wakeup */

	flash_led(4);
	state = WAKE_DIM;	/* go to "dimmer control" state */
	long_tick = 0;
	
      }

      break;


    // adjusting the light level
    case WAKE_DIM:

      if( long_tick > DIM_TIME) {
	pct = 0;
	state = SLEEP_LIGHT;		/* go back to sleep */
	break;
      }

      acomp_read();		/* update the sensor reading */

      pct = dim[level];
      break;

    case WAKE_ILLUM:		/* dark out, enable nightlight */
      pct = dim[level];

      if( acomp_read()) {	/* is it light now? */
	state = SLEEP_LIGHT;
	pct = 0;
	break;
      }

      /* time out and go to sleep */
      if( long_tick > SLEEP_TIME) {
	pct = 0;
	state = SLEEP_DARK;
      }
      break;

    default:
      ;
    }
    
  }
}


