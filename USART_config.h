/*
 * USART_config.h
 *
 * Created: 4/17/2022 11:11:02 AM
 *  Author: Brandon
 */ 


#ifndef USART_CONFIG_H_
#define USART_CONFIG_H_

//set baud rate
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 *(float)BAUD_RATE)) + 0.5)
void USART0_init(void);
int USART0_printChar(char character, FILE *stream);

#endif /* USART_CONFIG_H_ */

/*define the USART0_printchar function early 
so it can be used in the USART_stream declaration*/
int USART0_printChar(char character, FILE *stream);

//setup a stream to print to USART, This will be used in place of stdout
FILE USART_stream = FDEV_SETUP_STREAM(USART0_printChar, NULL, _FDEV_SETUP_WRITE);

//variables for USART0 Reading
char command[50];
uint8_t cmd_index = 0;
char c;

//***************************************************************************
//
// Function Name        : "USART0_init"
// Date                 : 11/12/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; USART0 general output
// Author               : Brandon Guzy
// DESCRIPTION
// This sets up the USART on pins A0 and A1 so they are ready to transmit
// and receive data
// also replaces stdout with the USART stream so printf can be used to output 
// to USART
//
// Warnings             : none
// Restrictions         : requires that USART0_BAUD_RATE function be defined
//						  requires that USART_stream be defined
// Algorithms           : none
// References           : none
//
// Revision History     : v1.0: Initial version
//						  v1.1: Updated to enable receive complete interrupt
//								and enable receive functionality
//
//**************************************************************************
void USART0_init(void)
{
	PORTA.DIR &= ~PIN1_bm;
	PORTA.DIR |= PIN0_bm;
	
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(115200);
	USART0.CTRLA |= USART_RXCIE_bm;
	USART0.CTRLB |= USART_TXEN_bm;
	USART0.CTRLB |= USART_RXEN_bm;	//enable receive
	
	stdout = &USART_stream;
}

//***************************************************************************
//
// Function Name        : "USART0_RXC_vect Interrupt"
// Date                 : 12/5/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; USART0 input
// Author               : Brandon Guzy
// DESCRIPTION
// This runs every time data is received over USART. For this project, it is
// character data. The character data is stored in a temporary string.
// When the newline character is received, the string is presumed to be
// complete, and the execute_USART_command function is run
//
// Warnings             : none
// Restrictions         : Requires global variables char[] command,  
//						  int cmd_index, char c to be defined
//							
// Algorithms           : none
// References           : execute_USART_command()
//
// Revision History     : Initial version
//
//**************************************************************************
ISR(USART0_RXC_vect){
	while (!(USART0.STATUS & USART_RXCIF_bm))//wait for data to be available
	{
		;
	}
	c = USART0.RXDATAL;
	if(c != '\n' && c != '\r')
	{
		command[cmd_index++] = c;
		if(cmd_index > 50)
		{
			cmd_index = 0;
		}
	}
	if(c == '\n')
	{
		command[cmd_index] = '\0';
		cmd_index = 0;
		execute_USART_command(command);
	}
}



//***************************************************************************
//
// Function Name        : "USART0_printChar"
// Date                 : 11/12/21
// Version              : 1.0
// Target MCU           : AVR128DB48
// Target Hardware      ; USART general output
// Author               : Brandon Guzy
// DESCRIPTION
// This prints out a character from a file stream to USART0
// This will be used in conjunction with other code to print from stdout 
// to USART
//
// Warnings             : none
// Restrictions         : none
// Algorithms           : none
// References           : none
//
// Revision History     : Initial version
//
//**************************************************************************
int USART0_printChar(char character, FILE *stream)
{
	while (!(USART0.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART0.TXDATAL = character;
	return 0;
}