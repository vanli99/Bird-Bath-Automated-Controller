/*
 * BBAC_Test.c
 *
 * Created: 9/13/2021 1:06:47 PM
 * Author : Brandon Guzy & Vanessa Li
 */ 

#define F_CPU 3333333UL
#include <avr/io.h>
#include <string.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "DOG204_LCD.h"

uint16_t second_counter = 180;
uint16_t double_press = 0;
uint16_t clean_time = 10800;
uint16_t delay_end = 0;
const uint8_t fill_delay = 120;
const uint8_t clean_delay = 180;
int night_mode = 1; // 1 means that night mode is on and 0 mean that it is off
int clean = 2; //1 means that a clean event was right before the fill event
char night[4]; //stores the status of night mode

//MODE List:
// h = home
// f = fill screen
// c = clean screen
// d = system disable screen 
// l = schedule clean screen
// i = schedule fill screen
// n = night mode 
// e = disable system screen
char mode = 'h';

//***************************************************************************
//
// Function Name        : "PORTB ISR"
// Date                 : 9/20/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      : Push Button
// Author               : Brandon Guzy & Vanessa Li
// DESCRIPTION
// Interrupt service routine for port B, handles button presses in different
// modes. For each mode, buttons are assigned different functions like switching
// to another screen or triggering actions like a fill/clean
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : v1.1 Added additional mode options
//
//**************************************************************************
ISR(PORTB_PORT_vect){
	
	switch(mode){
		case 'h'://home menu
		switch(PORTB.IN & 0x0F){
			case 0b00001110:	//disable system button
			mode = 'e';
			break;
			case 0b00001101:	//night mode button
			mode = 'n';
			break;
			case 0b00001011:	//schedule clean cycles
			mode = 'l';
			break;
			case 0b00000111:	//schedule fill cycles
			mode = 'i';
			break;
		}
		break;
		case 'e'://disable system menu
		switch(PORTB.IN & 0x0F){
			case 0b00001110:	//yes to disable mode
			mode = 'd';
			break;
			case 0b00001101:	//no to disable mode
			mode = 'h';
			break;
		}
		break;
		case 'd'://menu that shows "SYSTEM DISABLED"
		switch(PORTB.IN & 0x0F){
			case 0b00001110:	//turn off disabled mode
			TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV256_gc 
                    | TCA_SINGLE_ENABLE_bm; /* start timer */
			mode = 'h';
			break;
		}
		break;
		case 'n'://night mode menu
		if(night_mode == 1) {
			switch(PORTB.IN & 0x0F){
				case 0b00001110:	//yes to turn off night mode
				night_mode = 0;
				mode = 'h';
				break;
				case 0b00001101:	//no to turn off night mode
				mode = 'h';
				break;
			}	
		}else {
			switch(PORTB.IN & 0x0F){
				case 0b00001110:	//yes to turn on night mode
				night_mode = 1;
				mode = 'h';
				break;
				case 0b00001101:	//no to turn on night mode
				mode = 'h';
				break;
			}
		}
		break;
		case 'l'://clean menu
		switch(PORTB.IN & 0x0F){
			case 0b00001110:	//increment fills per clean
			if(clean_time < 63000)	//cannot overflow, max 18 hour clean time
				clean_time += 3600; //
			break;
			case 0b00001101:	//decrement fills per clean
			if(clean_time > 10800)// cannot go under 2 fills per clean
				clean_time -= 3600;
			break;
			case 0b00000111:	//home button
			if(second_counter > clean_time) {
				clean_time = clean_time + 3600;
			}
			mode = 'h';
			break;
		}
		break;
		case 'i'://fill menu (currently does nothing, will change next semester)
		switch(PORTB.IN & 0x0F){
			case 0b00000111:	//home button
			mode = 'h';
			break;
		}
		break;
		case 'f'://fill menu
		cancel_fill();
		break;
		case 'c'://clean menu
		cancel_clean();
		break;
		/*case 'd'://diagnostic menu
		mode = 'h';//go home on any button press
		//cancel_diagnostic function goes here
		break;*/
	}
	_delay_ms(250);
	PORTB_INTFLAGS = 0xFF;	//clear interrupt flags
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
 TCA0.SINGLE.EVCTRL &= ~(TCA_SINGLE_CNTEI_bm);
 /* set the period */
 TCA0.SINGLE.PER = 200;//12100;
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
// Increments seconds counter and checks if seconds counter is equal
// to the clean variable. If it is equal, the "clean" LED will light
// up, else the "fill" LED will light up. 
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
	 TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; //clear interrupt flags
}

//***************************************************************************
//
// Function Name        : "button_setup"
// Date                 : 9/30/21
// Version              : 1.2
// Target MCU           : AVR128DB48
// Target Hardware      ; Push Button
// Author               : Brandon Guzy
// DESCRIPTION
// This Program sets up pins B0-B3 as inputs, enables pull ups,
// and interrupts. At the end of the subroutine, interrupts are enabled
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
void button_setup(void){
	PORTB.DIR = 0xF0; //clear bits B0-3, set as input
	PORTB.PIN0CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTB.PIN1CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTB.PIN2CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	PORTB.PIN3CTRL = 0b00001011; //enable pull up and interrupt on falling edge
	_delay_ms(50);			//wait for pull ups and interrupts to fully enable
	PORTB_INTFLAGS = 0xFF;	//clear accidental interrupt flags
}

void start_fill(void) {
	mode = 'f';
	PORTB.OUT |= 0x10;
	delay_end = second_counter + fill_delay;
}

void start_clean(void) {
	clean = 1;
	mode = 'c';
	PORTB.OUT |= 0x20;
	delay_end = second_counter + clean_delay;
}

void cancel_fill(void) {
	PORTB.OUT &= 0b11101111;
	mode = 'h';
}

void cancel_clean(void) {
	PORTB.OUT &= 0b11011111;
	mode = 'h';
	second_counter = 180; //skip to start of clean/fill cycle
}

int main(void) {
	init_lcd_dog();
	snprintf(dsp_buff1, 21, "Startup Process");
	snprintf(dsp_buff2, 21, "-----------------");
	snprintf(dsp_buff3, 21, "------------------");
	snprintf(dsp_buff4, 21, "-------------------");
	update_lcd_dog();
	button_setup();
	TCA0_init();
	sei();
	while(1) {
		/*if(mode == 'f'  && second_counter > delay_end){
			cancel_fill();
		}else if(mode != 'c' && second_counter > clean_time){
			start_clean();
		}else if(mode != 'f' && second_counter%3600 < 30){	//there is a 30 second window to trigger fill, in case that something takes longer than 1s and we miss the window
			start_fill();
			//start fill goes here
			//in the cancel fill function, remember to go to AT LEAST second counter + 31 so as not to trigger the fill again after cancel
			//might need to discuss this later
		}*/
		
		if(second_counter%3600 == 0 && second_counter != clean_time && second_counter >= 3600){
			start_fill();
		}else if(second_counter == clean_time){
			start_clean();
		}/*else if(second_counter == delay_end && delay_end < clean_time){	
			cancel_fill();
		}else if(second_counter == delay_end && delay_end > clean_time){
			cancel_clean();
		}*/
		
		switch(mode){
			case 'd': //disabled menu
				snprintf(dsp_buff1, 21, "                     ");
				snprintf(dsp_buff2, 21, "   SYSTEM DISABLED   ");
				snprintf(dsp_buff3, 21, "                     ");
				snprintf(dsp_buff4, 21, "ENBL                   ");
				TCA0.SINGLE.CTRLA = 0; // stop timer if system is disabled
			break;
			case 'h'://home menu
			//if statement for checking what last operation was
			/*if(second_counter < 3600)
				snprintf(dsp_buff1, 21, "Last: Clean %d:%02d Ago  ", 0, second_counter); //(second_counter)/3600, (second_counter%3600)/60
			else
				snprintf(dsp_buff1, 21, "Last: Fill %d:%02d Ago  ", (second_counter%3601)/3600, (second_counter%3600)/60);
			// if statement for timing fill correctly, it skips when there is a clean	
			if(second_counter > clean_time - 3600)
				snprintf(dsp_buff3, 21, "Fill in 1:%02d          ", (3600 - (second_counter % 3600))/60 );
			else
				snprintf(dsp_buff3, 21, "Fill in 0:%02d          ", (3600 - (second_counter % 3600))/60 );
				
			snprintf(dsp_buff2, 21, "Clean in %d:%02d         ", (clean_time - second_counter)/3600, ((clean_time-second_counter)%3600)/60);
			snprintf(dsp_buff4, 21, "Fill Clean Schd Optn");*/
			if(night_mode == 1) {
				strcpy(night," ON");
			}else {
				strcpy(night,"OFF");
			}
			snprintf(dsp_buff1, 21, "LAST NEXT NEXT    NM");
			if(second_counter < 3600){
				snprintf(dsp_buff2, 21, "CLN  FILL FILL   %s ", night);
				snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ", 
				(second_counter)/3600, (second_counter%3600)/60,
				(3600 - (second_counter%3600))/60,
				(3600 - (second_counter%3600))/60);
			}else if(abs(clean_time - second_counter) < 3600) {
				snprintf(dsp_buff2, 21, "FILL CLN  FILL   %s ", night);
				snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ",
				(second_counter%3601)/3600, (second_counter%3600)/60,
				(3600 - (second_counter%3600))/60,
				(3600 - (second_counter%3600))/60);
			}else if(clean_time - second_counter < (3600*2) && clean_time - second_counter > 3600) {
				snprintf(dsp_buff2, 21, "FILL FILL CLN    %s ", night);
				snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ",
				(second_counter%3601)/3600, (second_counter%3600)/60,
				(3600 - (second_counter%3600))/60,
				(3600 - (second_counter%3600))/60);
			}else if(clean_time - second_counter > (3600*2)) {
				snprintf(dsp_buff2, 21, "FILL FILL FILL   %s ", night);
				snprintf(dsp_buff3, 21, "%d:%02d 0:%02d 1:%02d      ",
				(second_counter%3601)/3600, (second_counter%3600)/60,
				(3600 - (second_counter%3600))/60,
				(3600 - (second_counter%3600))/60);
			}
			snprintf(dsp_buff4, 21, "ENBL MODE CLEAN FILL");
			break;
			case 'l'://schedule clean menu
			snprintf(dsp_buff1, 21, "Fill Every Hour     ");
			snprintf(dsp_buff2, 21, "How many Fills?      ");
			snprintf(dsp_buff3, 21, "%d Fills per 1 Clean ", clean_time/3600 - 1);
			snprintf(dsp_buff4, 21, "Inc  Dec        Home ");
			break;
			case 'i'://schedule fill menu
			snprintf(dsp_buff1, 21, "Select when to fill: ");
			snprintf(dsp_buff2, 21, "Fill every hour      ");
			snprintf(dsp_buff3, 21, "                     ");
			snprintf(dsp_buff4, 21, "                Home ");
			break;
			case 'e'://enable menu
			snprintf(dsp_buff1, 21, "DISABLE SYSTEM?     ");
			snprintf(dsp_buff2, 21, "                    ");
			snprintf(dsp_buff3, 21, "                    ");
			snprintf(dsp_buff4, 21, "YES    NO           ");
			break;
			case 'n'://night mode menu
			if(night_mode == 1) {
				snprintf(dsp_buff1, 21, "Turn off night mode? ");
			}else if(night_mode == 0){
				snprintf(dsp_buff1, 21, "Turn on night mode?  ");
			}
			snprintf(dsp_buff2, 21, "                     ");
			snprintf(dsp_buff3, 21, "                     ");
			snprintf(dsp_buff4, 21, "YES    NO           ");
			break;
			case 'f'://fill menu
			snprintf(dsp_buff1, 21, "Currently Filling    ");
			snprintf(dsp_buff2, 21, "Time Remaining: %d:%02d ", (delay_end - second_counter)/ 60, (delay_end - second_counter) %60);
			snprintf(dsp_buff3, 21, "                     ");
			snprintf(dsp_buff4, 21, "Any Button to Cancel ");
			if(((delay_end - second_counter)/ 60 == 0) && ((delay_end - second_counter) %60) == 0) {
				PORTB.OUT &= 0b11101111;
				mode = 'h';
				if(clean == 1) { //if clean event was before filling, set second_counter to beginning of cycle
					second_counter = 180;
					clean = 0;
				}
			}
			break;
			case 'c'://clean menu
			snprintf(dsp_buff1, 21, "Currently Cleaning   ");
			snprintf(dsp_buff2, 21, "Time Remaining: %d:%02d ", (delay_end - second_counter)/ 60, (delay_end - second_counter) %60);
			snprintf(dsp_buff3, 21, "                      ");
			snprintf(dsp_buff4, 21, "Any Button to Cancel  ");
			if(((delay_end - second_counter)/ 60 == 0) && ((delay_end - second_counter) %60) == 0) {
				PORTB.OUT &= 0b11011111;
				second_counter = 3600;	//skip to fill event
			}
			break;
			/*case 'o'://option menu
			snprintf(dsp_buff1, 21, "Options Menu          ");
			snprintf(dsp_buff2, 21, "Run Diagnostic?       ");
			snprintf(dsp_buff3, 21, "                      ");
			snprintf(dsp_buff4, 21, "Diag            Home  ");
			break;
			case 'd'://diagnostic menu
			snprintf(dsp_buff1, 21, "Running Diagnostic     ");
			snprintf(dsp_buff2, 21, "Time Remaining: %d:%02d   ", (delay_end - second_counter)/ 60, (delay_end - second_counter) %60);
			snprintf(dsp_buff3, 21, "                      ");
			snprintf(dsp_buff4, 21, "Any Button to Cancel  ");
			break;*/
		}
		update_lcd_dog();
	}
}


