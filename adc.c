/* THIS DOESNT WORK */

#include <stdio.h>

#define ADC_BASE 0xFF204000
#define LED_BASE 0xFF200000

volatile int *ADC_ptr = (int *)ADC_BASE;
volatile int *LEDs = (volatile int *)LED_BASE;

void adcRead();

int main(void) {
	*(ADC_ptr + 1) = 0xFFFFFFFF;  // sets the ADC up to automatically perform conversion
	printf("Starting...\n");
	while (1) {
		adcRead();}
	return 0;
}

void adcRead() {
	int rawADC = *(ADC_ptr) & 0xFFF;
	float voltageFloat = rawADC * (5.0 / 4095.0);
	*LEDs = (int)voltageFloat;  // temporary debug
}