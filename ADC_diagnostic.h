/*
 * ADC_diagnostic.h
 *
 * Created: 5/3/2022 3:08:54 PM
 *  Author: livan
 */ 


#ifndef ADC_DIAGNOSTIC_H_
#define ADC_DIAGNOSTIC_H_

double AIN1, AIN2, AIN3;

double ADCpinSel_and_output (uint8_t pinNum);
void ADC0_init(void);
void runDiagnostics(void);
double solarConversion(void);
void POST(void);

#endif /* ADC_DIAGNOSTIC_H_ */

double ADCpinSel_and_output (uint8_t pinNum){
	double adcVal;
	ADC0.MUXPOS = pinNum;
	ADC0.COMMAND = 0x01; // start the conversion
	while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
	adcVal = ADC0.RES/1600.0; // read the value into adcVal
	ADC0.INTFLAGS = ADC_RESRDY_bm; // clear the flag
	return adcVal;
}


void ADC0_init(void){
	VREF.ADC0REF = VREF_REFSEL_2V048_gc;
	ADC0.CTRLC = ADC_PRESC_DIV128_gc;
	ADC0.CTRLA |= ADC_ENABLE_bm;
	
	/* Disable interrupt and digital input buffer on PD3 */
	PORTD.PIN3CTRL &= ~PORT_ISC_gm;
	PORTD.PIN3CTRL |= PORT_ISC_INPUT_DISABLE_gc;
	/* Disable interrupt and digital input buffer on PD4 */
	PORTD.PIN4CTRL &= ~PORT_ISC_gm;
	PORTD.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
	/* Disable interrupt and digital input buffer on PD5 */
	PORTD.PIN5CTRL &= ~PORT_ISC_gm;
	PORTD.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
	/* Disable interrupt and digital input buffer on PD6 */
	PORTD.PIN5CTRL &= ~PORT_ISC_gm;
	PORTD.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
}

void runDiagnostics(void) {
	PORTA.OUT |= 0b00001100; //enable fill and clean SSR
	PORTD.OUT |= 0b10000010; //enable BB2 SSR and ENAC-
	AIN1 = ADCpinSel_and_output(0x04);
	AIN2 = ADCpinSel_and_output(0x05);
	AIN3 = ADCpinSel_and_output(0x06);
}

double solarConversion(void) {
	return ADCpinSel_and_output(0x03);
}

void POST(void) { //run power-on self-test
	runDiagnostics();
	snprintf(dsp_buff1, 21, "Power-On Self Tests ");
	snprintf(dsp_buff2, 21, "FILL-%s  WIFI-GOOD", ((AIN1 > 1) ? "GOOD" : "FAIL"));
	snprintf(dsp_buff3, 21, " CLN-%s  SOLR-%s", ((AIN3 > 1) ? "GOOD" : "FAIL"), 
												((solarConversion() > 1) ? "GOOD" : "FAIL"));
	snprintf(dsp_buff4, 21, " BB2-%s           ",((AIN2 > 1) ? "GOOD" : "FAIL"));
	update_lcd_dog();
	_delay_ms(10000);
}