#define F_CPU 4000000UL

#include <avr/io.h>
#include <string.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "DOG204_LCD.h"
#include "USART_config.h"
#include "ADC_diagnostic.h"

void start_fill(void);
void start_clean(void);
void cancel_fill(void);
void cancel_clean(void);

char IPAdd[21];
uint16_t second_counter = 0;	//keeps track of seconds
uint16_t clean_time = 10800;	//time when a clean event occurs
uint16_t delay_end = 0;			//end of an event
const uint8_t fill_delay = 45;	//duration of fill event
uint8_t topOff_fill_delay = 20; //duration of top off fill 
const uint8_t clean_delay = 45;	//duration of clean event
int night_mode = 0;				//1 means that night mode is on and 0 mean that it is off
int clean = 0;					//1 means that a clean event was right before the fill event
int disabled = 0;				//1 means system was previously disabled before fill event
int resetFill = 0;				//1 means the fill cycle was an external fill and
								//second_counter should be reset to 0
char mode = 'h';				//stores 'state' of the system 

//***************************************************************************
//
// Function Name        : "PORTC ISR"
// Date                 : 9/20/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      : Push Button
// Author               : Brandon Guzy & Vanessa Li
// DESCRIPTION
// Interrupt service routine for port C, handles button presses in different
// modes. For each mode, buttons are assigned different functions like switching
// to another screen or triggering actions like a fill/clean
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : v1.1 Added additional mode options
//						  v1.2 Changed PORTB to PORTC
//**************************************************************************
ISR(PORTC_PORT_vect){
	_delay_ms(20); //delay to make sure the pushbutton was actually pressed
	switch(mode){
		case 'h'://home menu
		switch(PORTC.IN & 0x0F){
			case 0b00001110:	//mode menu
			mode = 'm';
			break;
			case 0b00001101:	//diagnostic menu
			mode = 'a';
			break;
			case 0b00001011:	//schedule clean cycles menu
			mode = 'l';
			break;
			case 0b00000111:	//schedule fill cycles menu
			mode = 'i';
			break;
		}
		break;
		case 'm': //mode menu
		switch(PORTC.IN & 0x0F){
			case 0b00001110:	//disable system menu
			mode = 'e';
			break;
			case 0b00001101:	//enable night-mode menu
			mode = 'n';
			break;
			case 0b00000111:	//home button
			mode = 'h';
			break;
		}
		break;
		case 'e'://disable system menu
		switch(PORTC.IN & 0x0F){
			case 0b00001110:	//yes to disable mode
			mode = 'd';
			break;
			case 0b00001101:	//no to disable mode
			mode = 'm';
			break;
		}
		break;
		case 'd'://menu that shows "SYSTEM DISABLED"
		switch(PORTC.IN & 0x0F){
			case 0b00000111:	//enable system
			mode = 'h';
			break;
		}
		break;
		case 'n'://enable night-mode menu
		if(night_mode == 1) {
			switch(PORTC.IN & 0x0F){
				case 0b00001110:	//yes to turn off night mode
				night_mode = 0;
				mode = 'h';
				break;
				case 0b00001101:	//no to turn off night mode
				mode = 'm';
				break;
			}	
		}else {
			switch(PORTC.IN & 0x0F){
				case 0b00001110:	//yes to turn on night mode
				night_mode = 1;
				mode = 'h';
				break;
				case 0b00001101:	//no to turn on night mode
				mode = 'm';
				break;
			}
		}
		break;
		case 'l'://schedule clean menu
		switch(PORTC.IN & 0x0F){
			case 0b00001110:	//increment fills per clean
			if(clean_time < 63000)	//cannot overflow, max 18 hour clean time
				clean_time += 3600; //
			break;
			case 0b00001101:	//decrement fills per clean
			if(clean_time > 10800)// cannot go under 2 fills per clean
				clean_time -= 3600;
			break;
			case 0b00001011:	//start clean cycle
			start_clean();
			break;
			case 0b00000111:	//home button
			if(second_counter > clean_time) {
				clean_time = clean_time + 3600;
			}
			mode = 'h';
			break;
		}
		break;
		case 'i'://schedule fill menu (duration of top-off fill)
		switch(PORTC.IN & 0x0F){
			case 0b00001110:	//increment by 1 second
				topOff_fill_delay += 1;
			break;
			case 0b00001101:	//decrement by 1 second
			if(topOff_fill_delay > 1) //cannot go below 1 second
				topOff_fill_delay -= 1;
			break;
			case 0b00001011:	//home button
				mode = 'h';
			break;
			case 0b00000111:	//start fill cycle
				resetFill = 1;
				start_fill();
			break;
		}
		break;
		case 'f'://fill menu
			cancel_fill();
		break;
		case 'c'://clean menu
			cancel_clean();
		break;
		case 'a': //diagnostics menu
		switch(PORTC.IN & 0x0F){
			case 0b00000111:	//home button
			mode = 'h';
			break;
		}
		break;
		case 'g': //night-mode menu
		switch(PORTC.IN & 0x0F){
			case 0b00000111:   //disable night mode
			night_mode = 0;
			mode = 'h';
			break;
		}
		break;	
	}
	PORTC_INTFLAGS = 0xFF;	//clear interrupt flags
}

//***************************************************************************
//
// Function Name        : "execute_USART_command"
// Date                 : 12/5/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; none
// Author               : Brandon Guzy
// DESCRIPTION
// This takes a string as an input and executes commands based on what
// the string is. Meant to be used in conjunction with the USART recieve
// interrupt
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
void execute_USART_command(char myCommand[]){
	if(!strcmp(myCommand, "fill")){
		resetFill = 1;
		start_fill();
	}else if(!strcmp(myCommand, "clean")){
		start_clean();
	}else if(!strcmp(myCommand, "disable")){
		mode = 'd';
	}else if(!strncmp(myCommand, "IP:", 3)){
		strcpy(IPAdd, myCommand);
	}else if(!strcmp(myCommand, "cancel")){
		if (mode == 'c') {
			cancel_clean();
		}else if(mode == 'd'){
			mode = 'h';		//turn off disabled mode
		}else if (mode == 'f') {
			cancel_fill();	
		}
	}
}

//***************************************************************************
//
// Function Name        : "PORTF ISR"
// Date                 : 12/07/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      : Push Button
// Author               : Vanessa Li
// DESCRIPTION
// Interrupt service routine for port F, handles button presses for the 
// external pushbuttons. PIN1 performs a clean and PIN0 performs a fill.
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     :
//
//**************************************************************************
ISR(PORTF_PORT_vect){
	_delay_ms(20);
	switch(PORTF.IN & 0b00000011){
			case 0b00000001:	//external clean
				if(mode == 'c') {
					cancel_clean();		//if currently cleaning, cancel if button is pressed
				}else if(mode == 'f') { //if currently filling, cancel fill if button is pressed
					cancel_fill();		
				}else {
					start_clean();		
				}
			break;
			case 0b00000010:	//external fill
				if(mode == 'f') {
					cancel_fill();
				}else {
					resetFill = 1;
					start_fill();
				}
			break;
		}

	PORTF_INTFLAGS = 0xFF;	//clear interrupt flags
}

//***************************************************************************
//
// Function Name        : "TCA0_init"
// Date                 : 10/16/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; none
// Author               : Vanessa Li
// DESCRIPTION
// This program initializes the TCA0 
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
void TCA0_init(void)
{
 /* enable overflow interrupt */
 TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
 /* set Normal mode */
 TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
 /* disable event counting */
 TCA0.SINGLE.EVCTRL &= ~(TCA_SINGLE_CNTAEI_bm); 
 /* set the period */
 TCA0.SINGLE.PER = 15624;
 /* set clock source (sys_clk/256) */
 TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV256_gc 
                    | TCA_SINGLE_ENABLE_bm; /* start timer */
}
//***************************************************************************
//
// Function Name        : "TCA0 ISR"
// Date                 : 10/16/21
// Version              : LED
// Target MCU           : AVR128DB48
// Target Hardware      ; none
// Author               : Vanessa Li
// DESCRIPTION
// Increments seconds counter and sends strings to USART0 using printf.
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
ISR(TCA0_OVF_vect) {
	second_counter += 0x01; // increment seconds counter 
	
	if(TCA0_SINGLE_PER > 10000){
	 printf("second_counter=%d\n", second_counter);
	 printf("clean_time=%d\n", clean_time);
	 printf("delay_end=%d\n", delay_end);
	 printf("mode=%c\n", mode);
	}else if(second_counter%30 == 0){
	printf("second_counter=%d\n", second_counter);
	 printf("clean_time=%d\n", clean_time);
	 printf("delay_end=%d\n", delay_end);
	 printf("mode=%c\n", mode);
	}else if(mode == 'd'){
	 printf("mode=%c\n", mode);
	}
	 
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; //clear interrupt flags
}

//***************************************************************************
//
// Function Name        : "port_init"
// Date                 : 9/30/21
// Version              : 1.2
// Target MCU           : AVR128DB48
// Target Hardware      ; Push Button
// Author               : Brandon Guzy
// DESCRIPTION
// This program sets up the ports of the AVR128DB28 as inputs, enables 
// pull ups, and interrupts. At the end of the subroutine, interrupts are
// enabled
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : v1.1 Updated code to clear accidental interrupts
//							after initialization
//						  v1.2 Updated to interrupt on falling edge to prevent
//								many interrupts when holding button
//
//**************************************************************************
void port_init(void){
	PORTC.DIR &= 0xF0; //C0-3, inputs for LCD pushbuttons
	PORTA.DIR |= 0b00001100; //A2-3, outputs for SSR enables(fill,clean)
	PORTD.DIR |= 0b10000010; //D7, output for SSR enable(BB2) and D1, output for ENAC-
	PORTF.DIR &= 0b11111100; //F0-1, inputs for external pushbuttons

	PORTF.PIN0CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTF.PIN1CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTC.PIN0CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTC.PIN1CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTC.PIN2CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTC.PIN3CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	_delay_ms(50);			//wait for pull ups and interrupts to fully enable
	
	PORTC_INTFLAGS = 0xFF;	//clear accidental interrupt flags
}

//during a fill, fill valve and BB2 valve are open 
void start_fill(void) {
	mode = 'f';
	PORTA.OUT |= 0b00000100; //open fill valve
	PORTD.OUT |= 0b10000010; //open BB2 valve and enable ENAC-
	if(clean == 1) { //fill duration is 45 sec if after clean event
		delay_end = second_counter + fill_delay;
	}else {	//fill duration defaults to 20 secs if top off fill
		delay_end = second_counter + topOff_fill_delay;
	}
}

//during clean, clean valve and BB2 valve are open, then BB2 closes after 15 secs
void start_clean(void) {
	clean = 1; //set clean to 1 so that the system knows that a clean event occurred
	mode = 'c'; 
	PORTA.OUT |= 0b00001000; //open clean valve
	PORTD.OUT |= 0b10000010; //open BB2 valve and enable ENAC-
	delay_end = second_counter + clean_delay;
}

//cancel_fill and cancel_clean are used to cancel clean/fill during the event by pressing any of the pushbuttons for the LCD
void cancel_fill(void) {
	PORTA.OUT &= 0b11111011;
	PORTD.OUT &= 0b01111101;
	mode = ((disabled == 1) ? 'd' : 'h'); //go back to disabled menu if previously disabled
	if(resetFill == 1) { //if external fill, reset second_counter to 0
		second_counter = 0;
		resetFill = 0;
	}
	if(clean == 1) { //if clean event was before filling, reset second_counter to 0
		second_counter = 0;
		clean = 0;
	}
}

void cancel_clean(void) {
	PORTA.OUT &= 0b11110111;
	PORTD.OUT &= 0b01111101;
	second_counter = 3600;	//skip to fill event
}

int main(void) {
	init_lcd_dog();
	port_init();
	TCA0_init();
	USART0_init();
	ADC0_init();
	sei();
	while(1) {
		//check if time for a fill or clean cycle
		if(second_counter%3600 == 0 && second_counter != clean_time && second_counter >= 3600){
			start_fill();
		}else if(second_counter == clean_time){
			start_clean();
		}
		
		//check if time to enter night mode
 		if(solarConversion() < 1.6 && night_mode == 1) {
 			mode = 'g';
 		}
		
		switch(mode){
			case 'd': //disabled menu
				snprintf(dsp_buff1, 21, "                     ");
				snprintf(dsp_buff2, 21, "   SYSTEM DISABLED   ");
				snprintf(dsp_buff3, 21, "                     ");
				snprintf(dsp_buff4, 21, "                ENBL ");
				second_counter = 0;
				disabled = 1;
			break;
			case 'h'://home menu
				PORTA.OUT &= 0b11111011;
				PORTD.OUT &= 0b01111111;
				PORTA.OUT &= 0b11110111;
				disabled = 0;
				snprintf(dsp_buff1, 21, "LAST NEXT NEXT    NM");
				if(second_counter < 3600){
					snprintf(dsp_buff2, 21, "CLN  FILL FILL   %s ", (night_mode == 1) ? " ON" : "OFF");
					snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ", 
					(second_counter)/3600, (second_counter%3600)/60,
					(3600 - (second_counter%3600))/60,
					(3600 - (second_counter%3600))/60);
				}else if(abs(clean_time - second_counter) < 3600) {
					snprintf(dsp_buff2, 21, "FILL CLN  FILL   %s ", (night_mode == 1) ? " ON" : "OFF");
					snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ",
					(second_counter%3601)/3600, (second_counter%3600)/60,
					(3600 - (second_counter%3600))/60,
					(3600 - (second_counter%3600))/60);
				}else if(clean_time - second_counter < (3600*2) && clean_time - second_counter > 3600) {
					snprintf(dsp_buff2, 21, "FILL FILL CLN    %s ", (night_mode == 1) ? " ON" : "OFF");
					snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ",
					(second_counter%3601)/3600, (second_counter%3600)/60,
					(3600 - (second_counter%3600))/60,
					(3600 - (second_counter%3600))/60);
				}else if(clean_time - second_counter > (3600*2)) {
					snprintf(dsp_buff2, 21, "FILL FILL FILL   %s ", (night_mode == 1) ? " ON" : "OFF");
					snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ",
					(second_counter%3601)/3600, (second_counter%3600)/60,
					(3600 - (second_counter%3600))/60,
					(3600 - (second_counter%3600))/60);
				}
				snprintf(dsp_buff4, 21, "MODE DIAG CLEAN FILL");
				break;
			case 'l'://schedule clean menu
				snprintf(dsp_buff1, 21, "Fill Every Hour     ");
				snprintf(dsp_buff2, 21, "How many Fills?      ");
				snprintf(dsp_buff3, 21, "%d Fills per 1 Clean ", clean_time/3600 - 1);
				snprintf(dsp_buff4, 21, "INC  DEC  CLEAN HOME ");
				break;
			case 'i'://schedule fill menu 
				snprintf(dsp_buff1, 21, "Duration of fill?    ");
				snprintf(dsp_buff2, 21, "%dm %ds              ", topOff_fill_delay/60, topOff_fill_delay%60);
				snprintf(dsp_buff3, 21, "                     ");
				snprintf(dsp_buff4, 21, "INC  DEC  HOME  FILL ");
				break;
			case 'e'://disable system menu
				snprintf(dsp_buff1, 21, "DISABLE SYSTEM?     ");
				snprintf(dsp_buff2, 21, "                    ");
				snprintf(dsp_buff3, 21, "                    ");
				snprintf(dsp_buff4, 21, "YES    NO           ");
				break;
			case 'n'://enable night mode menu
				if(night_mode == 1) {
					snprintf(dsp_buff1, 21, "Turn off night mode? ");
				}else if(night_mode == 0){
					snprintf(dsp_buff1, 21, "Turn on night mode?  ");
				}
				snprintf(dsp_buff2, 21, "                    ");
				snprintf(dsp_buff3, 21, "                    ");
				snprintf(dsp_buff4, 21, "YES    NO           ");
				break;
			case 'f'://fill menu
				snprintf(dsp_buff1, 21, "Currently Filling    ");
				snprintf(dsp_buff2, 21, "Time Remaining: %d:%02d ", 
				(delay_end - second_counter)/ 60, (delay_end - second_counter) %60);
				snprintf(dsp_buff3, 21, "                     ");
				snprintf(dsp_buff4, 21, "Any Button to Cancel ");
				if(second_counter >= delay_end) { //end fill
					PORTA.OUT &= 0b11111011; //close fill valve
					PORTD.OUT &= 0b01111101; //close BB2 valve and disable ENAC-
					mode = ((disabled == 1) ? 'd' : 'h'); //go back to disabled menu if previously disabled
					if(resetFill == 1) { //if external fill, reset second_counter to 0
						second_counter = 0;
						resetFill = 0;
					}
					if(clean == 1) { //if clean event was before filling, reset second_counter to 0
						second_counter = 0;
						clean = 0;
					}
				}
				break;
			case 'c'://clean menu 
				snprintf(dsp_buff1, 21, "Currently Cleaning   ");
				snprintf(dsp_buff2, 21, "Time Remaining: %d:%02d ", (delay_end - second_counter)/ 60, (delay_end - second_counter) %60);
				snprintf(dsp_buff3, 21, "                      ");
				snprintf(dsp_buff4, 21, "Any Button to Cancel  ");
				if (delay_end - second_counter <= 30){ 
					PORTD.OUT &= 0b01111111; //close BB2 after 15 seconds
				}
				if(second_counter >= delay_end) { 
					PORTA.OUT &= 0b11110111; //turn off clean valve
					PORTD.OUT &= 0b11111101; //disable ENAC-
					second_counter = 3600;	//skip to fill event
					start_fill();
				}
				break;
			case 'm'://mode menu
				snprintf(dsp_buff1, 21, "Select an option:   ");
				snprintf(dsp_buff2, 21, "                    ");
				snprintf(dsp_buff3, 21, "%s                    ", IPAdd);  //Print IP address
				snprintf(dsp_buff4, 21, "DSBL  NM        HOME");
				break;
			case 'a'://diagnostics menu
				{
					runDiagnostics();
					int tempInt1 = AIN1;
					double tempFrac1 = AIN1 - tempInt1;
					int fracInt1 = trunc(tempFrac1 * 100);
					int tempInt2 = AIN2;
					double tempFrac2 = AIN2 - tempInt2;
					int fracInt2 = trunc(tempFrac2 * 100);
					int tempInt3 = AIN3;
					double tempFrac3 = AIN3 - tempInt3;
					int fracInt3 = trunc(tempFrac3 * 100);
					snprintf(dsp_buff1, 21, "SSR1 SSR2  SSR3  SOL");
					snprintf(dsp_buff2, 21, "%d.%02d %d.%02d  %d.%02d      ",
					tempInt1, fracInt1, tempInt2, fracInt2, tempInt3, fracInt3);
					snprintf(dsp_buff3, 21, "                    ");
					snprintf(dsp_buff4, 21, "                HOME");
				}
				break;
			case 'g': //night mode menu
				snprintf(dsp_buff1, 21, "                     ");
				snprintf(dsp_buff2, 21, "     NIGHT MODE      ");
				snprintf(dsp_buff3, 21, "                     ");
				snprintf(dsp_buff4, 21, "                DSBL ");
				second_counter = 0;
				PORTF.PIN0CTRL = 0b00001100; //disable interrupt for external pushbuttons
				PORTF.PIN1CTRL = 0b00001100; 
	 			if(solarConversion() >= 1.6) { 
	 				mode = 'h';
	 			}
				break;
		}
		update_lcd_dog();
	}
}
