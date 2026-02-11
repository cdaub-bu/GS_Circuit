/*
 * Rev 2 GSA operating software
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
 * SLEEP_MODE   - sleep mode, wake only on power cycle
 *
 * WAKE_ILLUM   - night lights on until time-out
 *                on time-out goto SLEEP
 *                goto WAKE_DIM on button press
 *
 * store dimmer level [0...NDIM-1] in EEPROM address 0
 * default to dimmer level 1 if EEPROM is invalid
 */

#define USE_EEPROM

// rate of long_tick/sec
#define TICKS_SEC 100

// goto sleep and turn off after this many ticks of 100Hz
// now a 32-bit value, so 1 hr is e.g. (60L*60*TICKS_SEC)
// #define SLEEP_TIME 10*TICKS_SEC   // 10s for testing
// #define SLEEP_TIME 10L*60*TICKS_SEC   // 10m for testing

#define SLEEP_TIME 45L*60*TICKS_SEC   // 45m for production

// time lights stay on when button pressed
#define DIM_TIME 2*TICKS_SEC

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#ifdef USE_EEPROM
#include <avr/eeprom.h>
#endif

#include "timer_10kHz.h"

#include "sleep.h"

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

uint8_t dim[] = { 0, 1, 2, 5, 10, 20, 50, 99 };
#define NDIM (sizeof(dim)/sizeof(dim[0]))
static int dim_level;

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

  timer0_init_10kHz();

  long_tick = 0;
  button_tick = 0;

  bounce = DEBOUNCE;
#ifdef USE_EEPROM
  dim_level = eeprom_read_byte( 0);
  if( dim_level >= NDIM)
    dim_level = 1;
#else  
  dim_level = 1;
#endif

  pct = 0;

  flash_led(3);

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
	++dim_level;
	if( dim_level >= NDIM)
	  dim_level = 1;		/* don't allow 0 percent */
	pct = dim[dim_level];
#ifdef USE_EEPROM
	eeprom_write_byte( 0, dim_level);
#endif	
	button_tick = 0;
      }
    }
    
    switch( state) {

    case SLEEP_MODE:
      // lights off, timed-out.  Wake up on button press

      long_tick = 0;
      pct = 0;

      _delay_ms(10);

      // go to sleep
      sleep_init();
      sleep_cpu();

      // ---- SLEEP --- 
      // (don't ever get here...)

      // wake up
      sleep_disable();
      power_all_enable();
      power_adc_disable();

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

      pct = dim[dim_level];
      break;

    case WAKE_ILLUM:		/* dark out, enable nightlight */
      pct = dim[dim_level];

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


