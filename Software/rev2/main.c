/*
 * Initial version of GSA operating software
 * No light sensor, just a time-out
 *
 * use timer 0 interrupt at 10kHz to generate 0-99% at 100Hz PWM rate
 *
 * Implemented as an FSM with the following states:
 *
 * WAKE_DIM     - dimmer button pressed.
 *                lights on for DIM_TIME seconds
 *                goto SLEEP_MODE or WAKE_ILLUM depending on timer
 *
 * SLEEP_MODE   - sleep mode, wake only on button press or reset
 *                goto WAKE_DIM on button press
 *
 * WAKE_ILLUM   - ambient lights off, night lights on until time-out
 *                on time-out goto SLEEP_DARK
 *                goto WAKE_DIM on button press
 */

// use watchdog
#define USE_WDT

// rate of long_tick/sec
#define TICKS_SEC 100

// goto sleep and turn off after this many ticks of 100Hz
// now a 32-bit value, so 1 hr is e.g. (60*60*TICKS_SEC)
#define SLEEP_TIME 10*TICKS_SEC

// time lights stay on when button pressed
#define DIM_TIME 2*TICKS_SEC

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include "timer_10kHz.h"

#include "sleep.h"
#ifdef USE_WDT
#include "wdt.h" 
#endif

// LEDs on PB3 and PB4
#define LED_DDR DDRB
#define LED1_BIT 3
#define LED2_BIT 4
//#define LED_MASK (_BV(LED_BIT)|_BV(LED2_BIT))
#define LED1_MASK (_BV(LED1_BIT))
#define LED2_MASK (_BV(LED2_BIT))
#define LED_MASK (LED1_MASK|LED2_MASK)
#define LED_PORT PORTB

#define SW_DDR DDRB
#define SW_BIT 0
#define SW_PORT PORTB
#define SW_PIN PINB
#define SW_MASK _BV(SW_BIT)

volatile uint8_t tick = 0;
volatile uint32_t long_tick = 0;
volatile uint16_t button_tick = 0;
volatile int8_t pct = 1;
volatile int8_t wdtick = 0;
uint8_t button_press;

#define PERIOD 100

// Timer0 Compare Match A ISR (10 kHz)
// dim the 
ISR(TIMER0_COMPA_vect)
{
  ++tick;
  if( tick == PERIOD) {
    tick = 0;
    ++long_tick;
    ++button_tick;
  }
  if( tick < pct)
    LED_PORT |= LED_MASK;
  else
    LED_PORT &= ~LED_MASK;
}

// pin change interrupt (not used except for wake-up)
ISR(PCINT0_vect)
{
  ;
}

#ifdef USE_WDT
ISR(WDT_vect) {
  ++wdtick;
}
#endif


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

typedef enum {
  WAKE_DIM,
  SLEEP_MODE,
  WAKE_ILLUM
} a_state;

int main (void)
{
  static a_state state;
  static uint32_t last_long;
  static uint8_t button;
  static uint8_t last_button;

  static uint16_t light_secs;

  SW_PORT |= SW_MASK;	/* enable pull-up */
  LED_DDR |= LED_MASK;

  // turn off BOD in sleep to reduce power
  MCUCR |= _BV(BODS) | _BV(BODSE);
  MCUCR |= _BV(BODS);
  power_adc_disable();  	/* not using the ADC */

  /* Enable pin-change interrupt on PB0 */
  PCMSK |= (1 << PCINT0);   // Unmask PCINT0
  GIMSK |= (1 << PCIE);     // Enable pin-change interrupts
  GIFR |= (1 << PCIF);	    // clear pending interrupts

  timer0_init_10kHz();
#ifdef USE_WDT
  setup_wdt();			// also enable interrupts
#endif

  long_tick = 0;
  button_tick = 0;

  bounce = DEBOUNCE;
  level = 1;
  pct = 0;

  // start in WAKE_ILLUM
  state = WAKE_ILLUM;

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
      button = ((SW_PIN & SW_MASK) != 0);
      if( (button && !last_button) || button_press) {
	button_press = 0;
	state = WAKE_DIM;
	++level;
	if( level >= NDIM)
	  level = 1;		/* don't allow 0 percent */
	pct = dim[level];
	button_tick = 0;
      }
    }
    
    switch( state) {

    case SLEEP_MODE:
      // lights off, timed-out.  Wake up on button press

      long_tick = 0;

      // go to sleep
      sleep_init();
      sleep_cpu();

      // ---- SLEEP ---

      // wake up
      sleep_disable();
      power_all_enable();
      power_adc_disable();

      // figure out how we woke up and take action
#ifdef USE_WDT
      if( wdtick) {		/* WDT tick wakeup */

	wdtick = 0;
	//	flash_led(1);
	// check for switch pulled high
	if (SW_PIN & SW_MASK) {
	  flash_led(2);
	  state = WAKE_DIM;	/* go to "dimmer control" state */
	  button_tick = 0;
	  button_press = 1;
	}
	

      } else {			/* pin change wakeup */
#endif

	//	flash_led(2);
	state = WAKE_DIM;	/* go to "dimmer control" state */
	button_tick = 0;
	button_press = 1;

#ifdef USE_WDT	
      }
#endif

      break;


    // adjusting the light level
    case WAKE_DIM:

      if( button_tick > DIM_TIME) {     /* time out on dimmer on-time?  */
	if( long_tick > SLEEP_TIME) {	/* still lit up? */
	  state = SLEEP_MODE;		/* go to sleep */
	} else {
	  state = WAKE_ILLUM;
	}
	break;
      }

      pct = dim[level];
      break;

    case WAKE_ILLUM:		/* dark out, enable nightlight */
      pct = dim[level];

      /* time out and go to sleep */
      if( long_tick > SLEEP_TIME) {
	pct = 0;
	state = SLEEP_MODE;
      }
      break;

    default:
      ;
    }
    
  }
}


