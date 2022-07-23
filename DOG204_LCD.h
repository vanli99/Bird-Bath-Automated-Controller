/*
 * DOG204_LCD.h
 *
 * Created: 10/7/2021 1:45:35 PM
 *  Author: Brandon
 */ 

#ifndef DOG204_LCD_H_
#define DOG204_LCD_H_

//DOG LCD buffer
char dsp_buff1[21];
char dsp_buff2[21];
char dsp_buff3[21];
char dsp_buff4[21];

void lcd_spi_transmit_CMD (unsigned char cmd);
void lcd_spi_transmit_DATA (unsigned char cmd);
void init_spi_lcd (void);
void init_lcd_dog (void);
void delay_40mS(void);
void delay_30uS(void);
void update_lcd_dog(void);
void SPI0_wait();



#endif /* DOG204_LCD_H_ */

//***************************************************************************
//
// Function Name : "SPI_wait"
// Date : 9/14/21
// Version : 1.0
// Target MCU : AVR128DB48
// Target Hardware ;
// Author : Brandon Guzy
// DESCRIPTION
// This function waits until the interrupt flags for SPI are clear
//before proceeding
//
// Warnings : none
// Restrictions : none
// Algorithms : none
// References : none
//
// Revision History : Initial version
//
//**************************************************************************
void SPI0_wait(){
	//wait until there is no data waiting to be written or read
	while((SPI0.INTFLAGS & 0b11000000) != 0x80 ){ 
		asm volatile("nop");
	}
}

void delay_40mS(void){
	_delay_ms(40);
}

void delay_30uS(void){
	_delay_us(50);
}

//***************************************************************************
//
// Function Name        : "lcd_spi_transmit_DATA"
// Date                 : 9/14/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; ST7036 + LCD
// Author               : Brandon Guzy
// DESCRIPTION
// This function transmits data to the LCD
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
void lcd_spi_transmit_DATA (unsigned char cmd) {
	SPI0.DATA = 0x5F;	//Send 5 synchronization bits, RS = 1, R/W = 0
	SPI0_wait();
	SPI0.DATA = cmd & 0x0F;	//transmit lower data bits
	SPI0_wait();
	SPI0.DATA = ((cmd>>4) & 0x0F);	//send higher data bits
	SPI0_wait();
}

//***************************************************************************
//
// Function Name        : "lcd_spi_transmit_CMD"
// Date                 : 9/14/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; ST7036 + LCD
// Author               : Brandon Guzy
// DESCRIPTION
// This function transmits a command to the LCD, RS =0
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
void lcd_spi_transmit_CMD (unsigned char cmd) {
	SPI0.DATA = 0x1F;	//Send 5 synchronisation bits, RS = 0, R/W = 0
	SPI0_wait();
	SPI0.DATA = cmd & 0x0F;	//transmit lower data bits
	SPI0_wait();
	SPI0.DATA = ((cmd>>4) & 0x0F);	//send higher data bits
	SPI0_wait();
}

//***************************************************************************
//
// Function Name        : "init_spi_lcd"
// Date                 : 9/14/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; ST7036 + LCD
// Author               : Brandon Guzy
// DESCRIPTION
// This program initializes all ports and peripherals
// necessary to communicate with the LCD.
// Does not initialize any values on the LCD, only the AVR128
// Uses the standard SPI pins (PA7,6,4) for SPI communication
// PA7 = Slave Select, PA4 = MOSI, PA6 = SCK
// and PC0 for command select, RS = 1 for data, RS = 0 for command
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
void init_spi_lcd(){
	PORTA.DIR |= 0b11010000;	//enable output on necessary port A pins
	SPI0.CTRLA = 0b01100001;	//enable SPI, set in master mode, LSB First
	SPI0.CTRLB = 0b00000000;	//disable buffer, slave select enabled, normal mode
	PORTC.DIR = 0x01;			//enable output on port C0
	PORTA.OUT &= 0b01111111;	//clear Pin A7 to reset.
	delay_30uS();
	PORTA.OUT |= 0x80;			//set high to finish reset
}

//***************************************************************************
//
// Function Name        : "init_lcd_dog"
// Date                 : 9/14/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; ST7036 + LCD
// Author               : Brandon Guzy
// DESCRIPTION
// This program initializes the DOG LCD so it is ready to be sent lines.
// This program sends several commands using the spi_transmit_CMD function
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
void init_lcd_dog (void) {
	init_spi_lcd();		//Initialize mcu for LCD SPI
	
	//start_dly_40ms:
	delay_40mS();    //startup delay.


	//func_set1:
	lcd_spi_transmit_CMD(0x3A);   //8-Bit data length extension Bit RE=1; REV=0


	//func_set2:
	lcd_spi_transmit_CMD(0x09);	//4 line display

	

	//bias_set:
	lcd_spi_transmit_CMD(0x06);	//Bottom view
	//delay_30uS();	//delay for command to be processed


	//power_ctrl:
	lcd_spi_transmit_CMD(0x1E);	//Bias setting BS1=1
	//~ 0x55 for 3.3V (delicate adjustment).


	//follower_ctrl:
	lcd_spi_transmit_CMD(0x39);	//8-Bit data length extension Bit RE=0; IS=1
	
	//contrast_set:
	lcd_spi_transmit_CMD(0x1B);	//BS0=1 -> Bias=1/6


	//display_on:
	lcd_spi_transmit_CMD(0x6E);	//Devider on and set value


	//clr_display:
	lcd_spi_transmit_CMD(0x57);	//Booster on and set contrast (BB1=C5, DB0=C4)


	//entry_mode:
	lcd_spi_transmit_CMD(0x72);	//Set contrast (DB3-DB0=C3-C0)
	
	//entry_mode:
	lcd_spi_transmit_CMD(0x38);	//8-Bit data length extension Bit RE=0; IS=0
	
	//display_on
	lcd_spi_transmit_CMD(0x0C);	//Display on, Cursor off, Blink off
	
	lcd_spi_transmit_CMD(0x01);		//clear display
}

//***************************************************************************
//
// Function Name        : "update_lcd_dog"
// Date                 : 9/14/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; ST7036 + LCD
// Author               : Brandon Guzy
// DESCRIPTION
// This program updates the LCD with the characters from the 3 line buffers
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
void update_lcd_dog(void) {

	// send line 1 to the LCD module.
	lcd_spi_transmit_CMD(0x80);	//init DDRAM addr-ctr
	delay_30uS();
	for (int i = 0; i < 20; i++) {
		if(dsp_buff1[i] == '\0')
			break;
		lcd_spi_transmit_DATA(dsp_buff1[i]);
	}
	
	// send line 2 to the LCD module.
	lcd_spi_transmit_CMD(0xA0);	//init DDRAM addr-ctr
	delay_30uS();
	for (int i = 0; i < 20; i++) {
		if(dsp_buff2[i] == '\0')
			break;
		lcd_spi_transmit_DATA(dsp_buff2[i]);
	}
	
	// send line 3 to the LCD module.
	lcd_spi_transmit_CMD(0xC0);	//init DDRAM addr-ctr
	delay_30uS();
	for (int i = 0; i < 20; i++) {
		if(dsp_buff3[i] == '\0')
			break;
		lcd_spi_transmit_DATA(dsp_buff3[i]);
	}
	lcd_spi_transmit_CMD(0xE0);	//init DDRAM addr-ctr
	for (int i = 0; i < 20; i++) {
		if(dsp_buff4[i] == '\0')
			break;
		lcd_spi_transmit_DATA(dsp_buff4[i]);
	}
}

void clear_display(void){
	lcd_spi_transmit_CMD(0x01);		//clear display
}