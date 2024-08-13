 /******************************************************************************
 *
 * Module: ADC
 *
 * File Name: adc.c
 *
 * Description: Source file for the ATmega16 ADC driver
 *
 * Author: Mohamed Hisham
 *
 *******************************************************************************/

#include "avr/io.h" /* To use the ADC Registers */
#include "adc.h"
#include "common_macros.h" /* To use the macros like SET_BIT */

/*******************************************************************************
 *                      Functions Definitions                                  *
 *******************************************************************************/

void ADC_init(uint8 channel_num)
{
	/* ADMUX Register Bits Description:
	 * REFS1:0 = 00 to choose to connect external reference voltage by input this voltage through AREF pin
	 * ADLAR   = 0 right adjusted
	 * MUX4:0  = 00000 to choose channel 0 as initialization
	 */
	//V_REF = vcc
	//V_REF = vcc
		SET_BIT(ADMUX,6);
		CLEAR_BIT(ADMUX,7);
		//NO LEFT ADJ
		CLEAR_BIT(ADMUX,5);
		//CHANNEL 0 SINGLE
		CLEAR_BIT(ADMUX,4);
		CLEAR_BIT(ADMUX,3);
		CLEAR_BIT(ADMUX,2);
		CLEAR_BIT(ADMUX,1);
		CLEAR_BIT(ADMUX,0);
		ADMUX = ADMUX | channel_num;
		//PRESCALAR 128
		SET_BIT(ADCSRA,0);
		SET_BIT(ADCSRA,1);
		SET_BIT(ADCSRA,2);
		//DISABLE INTERUPT
		CLEAR_BIT(ADCSRA,3);
		//ENABLE ADC
		SET_BIT(ADCSRA,7);
		SET_BIT(ADCSRA,6);
}

	/* ADCSRA Register Bits Description:
	 * ADEN    = 1 Enable ADC
	 * ADIE    = 0 Disable ADC Interrupt
	 * ADATE   = 0 Disable Auto Trigger
	 * ADPS2:0 = 011 to choose ADC_Clock = F_CPU/8 = 1Mhz/8 = 125Khz --> ADC must operate in range 50-200Khz
	 */

uint8 ADC_readChannel(uint16* result)
{
	if (GET_BIT(ADCSRA,4)) {
		ADCW&=(0b0000001111111111);
		*result = ADCW;
		SET_BIT(ADCSRA,4);
		SET_BIT(ADCSRA,6);
		return 1;
	}
	return 0;
}


//for ac ameter
void ADC_initacameter() {
    // Set reference to internal 2.56V and right adjust the result
    ADMUX = (1<<REFS1) | (1<<REFS0);
    // Enable ADC, set prescaler to 128 for stability
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

uint16_t ADC_readChannelacameter(uint8_t channel) {
    // Select ADC channel with safety mask
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);
    // Start single conversion
    ADCSRA |= (1<<ADSC);
    // Wait for conversion to complete
    while (ADCSRA & (1<<ADSC));
    // Return ADC value
    return ADC;
}
