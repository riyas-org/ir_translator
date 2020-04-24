#include "rc5.h"

#include <avr/io.h>
#include <avr/interrupt.h>

/* The formula to calculate ticks is as follows 
 * TICKS = PULSE_LENGTH / (1 / (CPU_FREQ / TIMER_PRESCALER))
 * Where CPU_FREQ is given in MHz and PULSE_LENGTH in us.
 * LONG_MIN should usually be SHORT_MAX + 1 */
 
//#define SHORT_MIN 888   /* 444 microseconds */
//#define SHORT_MAX 2666  /* 1333 microseconds */
//#define LONG_MIN 2668   /* 1334 microseconds */
//#define LONG_MAX 4444   /* 2222 microseconds */
 
//for attiny13 at 1.2MHZ with prescaler 64
//#define SHORT_MIN 7   /* 444 microseconds */
//#define SHORT_MAX 20  /* 1333 microseconds */
//#define LONG_MIN 21   /* 1334 microseconds */
//#define LONG_MAX 35   /* 2222 microseconds */

//for attiny85 at 16.5MHZ with prescaler 1024
#define SHORT_MIN 7   /* 444 microseconds */
#define SHORT_MAX 20  /* 1333 microseconds */
#define LONG_MIN 21   /* 1334 microseconds */
#define LONG_MAX 35   /* 2222 microseconds */



#define    IR_IN_PIN              PB1  //INT0 pin on attiny13 but it is PB2 in attiny85


typedef enum {
    STATE_START1, 
    STATE_MID1, 
    STATE_MID0, 
    STATE_START0, 
    STATE_ERROR, 
    STATE_BEGIN, 
    STATE_END
} State;

const uint8_t trans[4] = {0x01, 0x91, 0x9b, 0xfb};
volatile uint16_t command;
uint8_t ccounter;
volatile uint8_t has_new;
State state = STATE_BEGIN;

void RC5_Init()
{
    /* Set INT0 to trigger on any edge */
    //EICRA |= _BV(ISC00);

	GIMSK |= _BV(INT0); // enable INT0 interrupt handler
    MCUCR &= ~_BV(ISC01); // trigger INTO interrupt on raising
    MCUCR |= _BV(ISC00); // and falling edge

	
    /* Set PD2 to input */
    //DDRD &= ~_BV(PD2);
	DDRB &= ~_BV(IR_IN_PIN); // set IR IN pin as INPUT
    
    /* Reset Timer1 Counter */
    //TCCR1A = 0;
    /* Enable Timer1 in normal mode with /8 clock prescaling */
    /* One tick is 500ns with 16MHz clock */
    //TCCR1B = _BV(CS11);
	
	TCCR0A =0;
	// set prescaler to 64
	TCCR0B |= _BV(CS00);
	TCCR0B |= _BV(CS01);
	
	// set prescaler to 1024 for attiny85
	//TCCR0B |= _BV(CS00);
	//TCCR0B |= _BV(CS02);
	
	
	/* One tick is 32 microseconds for 8MHZ clock 1/(8M/256) */
    
    RC5_Reset();
}


void RC5_Reset()
{
    has_new = 0;
    ccounter = 14;
    command = 0;
    state = STATE_BEGIN;
    
    /* Enable INT0 */
    //EIMSK |= _BV(INT0);
	//GIMSK |= _BV(INT0); 
    GIMSK |= _BV(INT0); // enable INT0 interrupt handler
    MCUCR &= ~_BV(ISC01); // trigger INTO interrupt on raising
    MCUCR |= _BV(ISC00); // and falling edge
}


uint8_t RC5_NewCommandReceived(uint16_t *new_command) 
{ 
    if(has_new) 
    { 
        *new_command = command; 
    } 
    
    return has_new; 
}

ISR(INT0_vect)
{
    uint8_t delay = TCNT0;

    /* TSOP2236 pulls the data line up, giving active low,
     * so the output is inverted. If data pin is high then the edge
     * was falling and vice versa.
     * 
     *  Event numbers:
     *  0 - short space
     *  2 - short pulse
     *  4 - long space
     *  6 - long pulse
     */
	//PORTB ^= _BV(PB0);

	 
    uint8_t event = (PINB & _BV(PINB1)) ? 2 : 0;
	
    
	
    if(delay > LONG_MIN && delay < LONG_MAX)
    {
        event += 4;
    }
    else if(delay < SHORT_MIN || delay > SHORT_MAX)
    {
        /* If delay wasn't long and isn't short then
         * it is erroneous so we need to reset but
         * we don't return from interrupt so we don't
         * loose the edge currently detected. */
        RC5_Reset();
    }

    if(state == STATE_BEGIN)
    {
        ccounter--;
        command |= 1 << ccounter;
        state = STATE_MID1;
        TCNT0 = 0;
        return;
    }
    
    State newstate = (trans[state] >> event) & 0x03;

    if(newstate == state || state > STATE_START0)
    {
        /* No state change or wrong state means
         * error so reset. */
        RC5_Reset();
        return;
    }
    
    state = newstate;
    
    /* Emit 0 - jest decrement bit position counter
     * cause data is already zeroed by default. */
    if(state == STATE_MID0)
    {
        ccounter--;
    }
    else if(state == STATE_MID1)
    {
        /* Emit 1 */
        ccounter--;
        command |= 1 << ccounter;
    }
    
    /* The only valid end states are MID0 and START1.
     * Mid0 is ok, but if we finish in MID1 we need to wait
     * for START1 so the last edge is consumed. */
    if(ccounter == 0 && (state == STATE_START1 || state == STATE_MID0))
    {
        state = STATE_END;
        has_new = 1;
        
        /* Disable INT0 */
		GIMSK &= ~_BV(INT0); 
    }
    
    TCNT0 = 0;
}
