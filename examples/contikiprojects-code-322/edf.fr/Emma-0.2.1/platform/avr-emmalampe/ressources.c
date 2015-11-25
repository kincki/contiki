/**
 *  Energy Monitoring & Management Agent for IPV6 RestFull HTTP 
 *  Copyright (C) 2010  DUHART Clément <duhart.clement@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * All rights reserved.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Emma-Rest6.h"

RESSOURCE_LOAD(White1);
RESSOURCE_LOAD(White2);
RESSOURCE_LOAD(White3);
RESSOURCE_LOAD(White4);

RESSOURCE_LOAD(Red);
RESSOURCE_LOAD(Green);
RESSOURCE_LOAD(Blue);

 void init_pwm(){

	// ***************** RGB LED **********************
	DDRD |= _BV(PIND4) | _BV(PIND5) |  _BV(PIND0);

	// ***************** PWM LED WHITE ****************
  // B5 B6 PIN 4 5 LED WITHE 2 3 => OC1A OC1B
  TCCR1A = _BV(WGM10) | _BV(WGM11) | _BV(COM1A1) | _BV(COM1A0)| _BV(COM1B1) | _BV(COM1B0);
  TCCR1B = _BV(CS10);
  DDRB |= _BV(PINB4) | _BV(PINB5);
  OCR1A = 1000;			/* set PWM value to 0 */
  OCR1B = 1000;
  
  // E4 PIN37 41 LED WITHE 1 0=> OC3B OC3A
  TCCR3A = _BV(WGM10) | _BV(WGM11) | _BV(COM3B1) | _BV(COM3B0) | _BV(COM3A1) | _BV(COM3A0);
  TCCR3B = _BV(CS10);
  DDRE |= _BV(PINE4) | _BV(PINE3);
  OCR3A = 1000;
  OCR3B = 1000;

	// ***************** INTERRUPTION TRACKBALL********
/*DDRD &= ~ ((1<<PIND3) | (1<<PIND2) | (1<<PIND1) | (1<<PINE6) | (1<<PINE7)); // PD2 en entrée 
EIMSK |= (1<<INT7) | (1<<INT6) | (1<<INT3) | (1<<INT2) | (1<<INT1); // Autorise l'interruption extérieure INT0 
EICRA |= (1<<ISC31) | (1<<ISC21) | (1<<ISC11); // INT0 déclanché sur front descendant 
EICRB |= (1<<ISC71) | (1<<ISC61);
sei();*/
 }
/*
// TRACK BUTTON
 ISR (INT3_vect){ 
// Routine appelée à chaque interruption 
OCR1A = 1000;
OCR1B = 1000;
OCR3A = 1000;
OCR3B = 1000;

} 
*/
void setPWM1 (int val){
OCR1A = val > 1024 ? 1023 : (val < 0 ? 0 : val);
}
void setPWM2 (int val){
OCR1B = val > 1024 ? 1023 : (val < 0 ? 0 : val);
}
void setPWM3 (int val){
OCR3A = val > 1024 ? 1023 : (val < 0 ? 0 : val);
}
void setPWM4 (int val){
OCR3B = val > 1024 ? 1023 : (val < 0 ? 0 : val);
}

int getPWM1(){
return OCR1A;
}

int getPWM2(){
return OCR1B;
}

int getPWM3(){
return OCR3A;
}

int getPWM4(){
return OCR3A;
}

int getRed(){
return (PORTD & (1<<PIND0)) >> PIND0;;
}

int getGreen(){
return (PORTD & (1<<PIND4)) >> PIND4;
}

int getBlue(){
return (PORTD & (1<<PIND5)) >> PIND5;
}

void setRed(int val){
if(val > 0)
	PORTD |=  (1<<PIND0);

else	PORTD &= ~(1<<PIND0);
}

void setGreen(int val){
if(val > 0)
	PORTD |=  (1<<PIND4);
else	PORTD &= ~(1<<PIND4);
}

void setBlue(int val){
if(val > 0)
	PORTD |=  (1<<PIND5);
else	PORTD &= ~(1<<PIND5);
}

// TRACK LEFT
/*
* Affiche les 8 combinaisons de couleurs sur la led par increment de un
* Décallage des bits
*
 ISR (INT6_vect){
	// 0011 0001
	char a =  (PORTD & (1<<PIND0));
	a |=  (PORTD & (1<<PIND4)) >> 3;
	a |=  (PORTD & (1<<PIND5)) >> 3;

	// 0000 0111
	a++;
	a %= 8;

	PORTD &= ~((1<<PIND4) | (1<<PIND5) | (1<<PIND0));
	// 0000 0111
	PORTD |= (a & 1);
	PORTD |= (a & 2) << 3;
	PORTD |= (a & 4) << 3;
	
	Red.value = (PORTD & (1<<PIND0)) >> PIND0;
	Green.value = (PORTD & (1<<PIND4)) >> PIND4;
	Blue.value = (PORTD & (1<<PIND5)) >> PIND5;
} 

// TRACK RIGHT
 ISR (INT7_vect){ 
	// 0011 0001
	char a =  (PORTD & (1<<PIND0));
	a |=  (PORTD & (1<<PIND4)) >> 3;
	a |=  (PORTD & (1<<PIND5)) >> 3;

	// 0000 0111
	a--;
	a %= 8;

	PORTD &= ~((1<<PIND4) | (1<<PIND5) | (1<<PIND0));
	// 0000 0111
	PORTD |= (a & 1);
	PORTD |= (a & 2) << 3;
	PORTD |= (a & 4) << 3;

	Red.value = (PORTD & (1<<PIND0)) >> PIND0;
	Green.value = (PORTD & (1<<PIND4)) >> PIND4;
	Blue.value = (PORTD & (1<<PIND5)) >> PIND5;
} 

// TRACK DOWN
 ISR (INT2_vect){ 
	OCR1A = (OCR1A+20) <= 1024 ? OCR1A+20 : 1024;
	OCR1B = (OCR1B+20) <= 1024 ? OCR1B+20 : 1024;
	OCR3A = (OCR3A+20) <= 1024 ? OCR3A+20 : 1024;
	OCR3B = (OCR3B+20) <= 1024 ? OCR3B+20 : 1024;

	
	Red.value = (PORTD & (1<<PIND0)) >> PIND0;
	Green.value = (PORTD & (1<<PIND4)) >> PIND4;
	Blue.value = (PORTD & (1<<PIND5)) >> PIND5;

	White1.value = OCR1A;
	White2.value = OCR1B;
	White3.value = OCR3A;
	White4.value = OCR3B;
} 

// TRACK UP
 ISR (INT1_vect){ 
	OCR1A = (OCR1A-20) >= 0 ? OCR1A-20 : 0;
	OCR1B = (OCR1B-20) >= 0 ? OCR1B-20 : 0;
	OCR3A = (OCR3A-20) >= 0 ? OCR3A-20 : 0;
	OCR3B = (OCR3B-20) >= 0 ? OCR3B-20 : 0;

	White1.value = OCR1A;
	White2.value = OCR1B;
	White3.value = OCR3A;
	White4.value = OCR3B;
}*/
