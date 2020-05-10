#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/sleep.h>


#include "main.h"
#include "rc5.h"
#include "finlux.h"


// pulse parameters in usec
#define NEC_BITS          32
#define NEC_HDR_MARK    9000/8
#define NEC_HDR_SPACE   4500/8
#define NEC_BIT_MARK     560/8
#define NEC_ONE_SPACE   1690/8
#define NEC_ZERO_SPACE   560/8
#define NEC_RPT_SPACE   2250/8

#define TIMER_PWM_PIN        PB0  /* ATtiny13 */
#ifdef F_CPU
#	define SYSCLOCK  F_CPU     // main Arduino clock
#else
#	define SYSCLOCK  1200000  // default attiny clock
#endif
#define TIMER_RESET
#define TIMER_ENABLE_PWM     (TCCR0A |= _BV(COM0A0))          //on attiny moved from OC0B(PB1) to OC0A(PB0) as INT0 conflict
#define TIMER_DISABLE_PWM    (TCCR0A &= ~(_BV(COM0A0)))
#define TIMER_ENABLE_INTR    (TIMSK |= _BV(OCIE0A))
#define TIMER_DISABLE_INTR   (TIMSK &= ~(_BV(OCIE0A)))
#define TIMER_INTR_NAME      TIMER0_COMPA_vect

#define TIMER_CONFIG_KHZ(val) ({ \
  const uint8_t pwmval = 8; \
  TCCR0A = _BV(WGM00); \
  TCCR0B = _BV(WGM02) | _BV(CS00); \
  OCR0A = pwmval; \
  OCR0B = 3; \
})



void delay_us(int d)
{
    int i;
    for (i = 0; i < d; i++) {
        _delay_us(1);
    }
}
void  mark (unsigned int time)
{
	TIMER_ENABLE_PWM; 
	if (time > 0) delay_us(time);
}

void  space (unsigned int time)
{
	TIMER_DISABLE_PWM; 
	if (time > 0) delay_us(time);
}

static void  enableIROut (int khz)
{
	GIMSK &= ~_BV(INT0); 
	DDRB |= _BV(TIMER_PWM_PIN);
	PORTB &= ~(1 << TIMER_PWM_PIN); 
	TIMER_CONFIG_KHZ(khz);

}


void  sendNEC (unsigned long data,  int nbits)
{
	enableIROut(38);
	mark(NEC_HDR_MARK);
	space(NEC_HDR_SPACE);
	unsigned long  mask; 
	for (mask = 1UL << (nbits - 1);  mask;  mask >>= 1) {
		if (data & mask) {
			mark(NEC_BIT_MARK);
			space(NEC_ONE_SPACE);
		} else {
			mark(NEC_BIT_MARK);
			space(NEC_ZERO_SPACE);
		}
	}
	mark(NEC_BIT_MARK);
	space(0);  
}

static void tv(unsigned int data){
  unsigned int MSB = 0x20DF;
  unsigned long Result;
  Result = (unsigned long)MSB << 16 | data;
  sendNEC(Result,32);
}
static void sat(unsigned int data){
  unsigned int MSB = 0xE17A;
  unsigned long Result;
  Result = (unsigned long)MSB << 16 | data; 
  sendNEC(Result,32);  
}


ISR(PCINT0_vect)
{
}
static void sleep(){
  ADCSRA &= ~(1 << ADEN);
  ACSR |= (1 << ACD);      //Analog comparator off
  PCMSK |= (1<<PCINT1);   // pin change mask: listen to portb, pin PB1
  GIMSK |= (1<<PCIE); // enable PCINT interrupt
  PORTB = 0b000000; // all LOW
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();
  GIMSK &= ~(1<<PCIE); // disable PCINT interrupt

}

int main()
{
	DDRB  = 0b000001; // all but PB0 INPUT, want to use PB0 ...
    RC5_Init();
    sei();
   
    for(;;)
    {
		uint16_t command;
        if(RC5_NewCommandReceived(&command))
        {

            RC5_Reset();            
			uint8_t rc5_byte;
			uint16_t nec_byte;
			char i;
			for(i=0;i<24;i++)  //our table has 24 entries and 0..4  are satellite		
			
				{
					rc5_byte = pgm_read_byte(&(rc5_code_data[i]));
					if(rc5_byte==RC5_GetCommandAddressBits(command)){
						nec_byte = pgm_read_word(&(nec_code_data[i]));
						if(i<5)
							{sat(nec_byte);}
						else
							{tv(nec_byte);}
						//_delay_ms(200);
						command=0;						
						}
											
				}
		sleep(); 
		RC5_Init();
        }
    }
    
    return 0;
}
