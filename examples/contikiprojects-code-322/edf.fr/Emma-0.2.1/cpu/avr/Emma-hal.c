#include <avr/io.h>
enum {
  PWM0,
  PWM1,
  PWM2,
  PWM3
};

void initPWM(void){

  // B5 B6 PIN 4 5 LED WITHE 2 3 => OC1A OC1B
/*  TCCR1A = _BV(WGM10) | _BV(WGM11) | _BV(COM1A1) | _BV(COM1A0)| _BV(COM1B1) | _BV(COM1B0);
  TCCR1B = _BV(CS10);

  DDRB |= _BV(PINB4) | _BV(PINB5);
  OCR1A = 1023;			/* set PWM value to 0 */
 // OCR1B = 1023;
  
  // E4 PIN37 41 LED WITHE 1 0=> OC3B OC3A
  TCCR3A = _BV(WGM10) | _BV(WGM11) | _BV(COM3B1) | _BV(COM3B0) | _BV(COM3A1) | _BV(COM3A0);
  TCCR3B = _BV(CS10);
  DDRE |= _BV(PINE4) | _BV(PINE3);
  OCR3A = 1023;
  OCR3B = 1023;
}

void setPWM(char pin, int val){
  switch(pin){
    case PWM0 : OCR1A = val; break;
    case PWM1 : OCR1B = val; break;
    case PWM2 : OCR3A = val; break;
    case PWM3 : OCR3B = val; break;
    default : return ;
  }
}

int getPWM(char pin){
  switch(pin){
    case PWM0 : return OCR1A; break;
    case PWM1 : return OCR1B; break;
    case PWM2 : return OCR3A; break;
    case PWM3 : return OCR3B; break;
    default : return -1;
  }
}

/*************************************
*
* ADC Converter
*
***************************************/
/*
volatile uint16_t ADCVal;
volatile uint16_t ADC_READY;

void initADC(void)
{
	ADMUX =  (1<<REFS0);	// Voltage reference
	ADMUX &= ~(1<<REFS1);
	ADMUX |= (0<<ADLAR)|	// ADC Left Adjust result
		 (0<<MUX3)| 	// ADC0 pin configuration
		 (0<<MUX2)| 	// ADC0 pin configuration
		 (0<<MUX1)| 	// ADC0 pin configuration
		 (0<<MUX0); 	// ADC0 pin configuration
				
	ADCSRA = (1<<ADEN)| 		// Enable ADC Converter
			(0<<ADSC)|	// Bit to start the ADC converter
			(0<<ADFR)|	// Free Running Select
			(1<<ADIF)|	// ADC Interrupt Flag
			(1<<ADIE)|	// ADC interrupt enable
			(1<<ADPS2)|	//
			(1<<ADPS1)| 	// ADC prescaler select bits Division factor : 128
			(1<<ADPS0); 	//

	// Init ADC value  buffer
	ADCVal = 0;

	// Init ADC Read Flag
	ADC_READY = 0;
}

#define HARDWARE_ADC_LOAD ISR(ADC_vect){ \
	ADCVal = ADCW; \
	ADCSRA &= ~_BV(ADIE); \
	ADC_READY = 1; \
	}
*/
