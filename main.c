#include <stdint.h>

#define ADC_BASE 0xFF204000
#define LEDs_BASE 0xFF200000
#define GPIO_BASE 0xFF200070
#define BUFFER_SIZE 1024

volatile unsigned int adc_buffer[BUFFER_SIZE];
volatile int write_index = 0;
volatile int read_index = 0;

unsigned int read_adc(void);
void buffer_add(unsigned int sample);
unsigned int buffer_get(void);

int main(void) {
  while (1) {
    unsigned int adc_sample = read_adc();
    buffer_add(adc_sample);
  }

  return 0;
}

unsigned int read_adc(void) {
  volatile unsigned int* ADC_control = (unsigned int*)ADC_BASE;
  volatile unsigned int* LEDs_control = (unsigned int*)LEDs_BASE;
  volatile unsigned int* GPIO_control = (unsigned int*)GPIO_BASE;

  *ADC_control = 1;
  unsigned int result = *ADC_control & 0xFFF;

  *LEDs_control = result;

  for (volatile int delay = 0; delay < 1000; delay++);
  return result;
}

void buffer_add(unsigned int sample) {  // WRITE NEXT SAMPLE
  adc_buffer[write_index] = sample;

  write_index++;
  if (write_index >= BUFFER_SIZE) write_index = 0;

  // IF FULL OVERWRITE OLDEST SAMPLE
  if (write_index == read_index) {
    read_index++;
    if (read_index >= BUFFER_SIZE) read_index = 0;
  }
}

unsigned int buffer_get(void) {  // READ SAMPLE
  unsigned int sample = adc_buffer[read_index];

  read_index++;
  if (read_index >= BUFFER_SIZE) read_index = 0;

  return sample;
}
