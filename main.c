/* THIS IS THE MAIN LOOP UPDATED AS OF MARCH 23, 10PM

this file deals with all sampling + buffering

read_adc - STILL DOESNT WORK. NEED TO TROUBLESHOOT THIS
buffer_add - adds new sample to the circular buffer + wraps around index if more
values than max buffer_get - gets next sample from buffer
process_available_samples - finds how many samples in buffer, converts to
voltage, and sends out chunks of 80. can be connected to other functions for
further processing, but for now connected to buffer_to_measurements() to display
measurements
*/

#include <stdint.h>

#define ADC_BASE 0xFF204000
#define LEDs_BASE 0xFF200000
#define GPIO_BASE 0xFF200070

#define ADC_BUFFER_SIZE 1024

#define REF_VOLTAGE 5.0f  // 5V reference

volatile unsigned int adc_buffer[ADC_BUFFER_SIZE];
volatile int write_index = 0;
volatile int read_index = 0;

extern float adc_samples[ADC_BUFFER_SIZE];
extern int adc_sample_count;
extern void buffer_to_measurements(void);

// ALL FUNCTION PROTOTYPES
unsigned int read_adc(void);
void buffer_add(unsigned int sample);
unsigned int buffer_get(void);
void process_available_samples(void);

int main(void) {
  while (1) {
    unsigned int adc_sample = read_adc();
    buffer_add(adc_sample);

    process_available_samples();  // chunk samples and process
  }

  return 0;
}

unsigned int read_adc(
    void) {  // READ FUNCTION - THIS STILL DOEANT WORK!!!!!!!!!
  volatile unsigned int* ADC_control = (unsigned int*)ADC_BASE;
  volatile unsigned int* LEDs_control = (unsigned int*)LEDs_BASE;
  volatile unsigned int* GPIO_control = (unsigned int*)GPIO_BASE;

  *ADC_control = 1;
  unsigned int result = *ADC_control & 0xFFF;
  *LEDs_control = result;  // display on leds

  for (volatile int delay = 0; delay < 1000; delay++);

  return result;
}

void buffer_add(unsigned int sample) {  // WRITE NEXT SAMPLE
  adc_buffer[write_index] = sample;
  write_index++;
  if (write_index >= ADC_BUFFER_SIZE) write_index = 0;

  // IF FULL OVERWRITE OLDEST SAMPLE
  if (write_index == read_index) {
    read_index++;
    if (read_index >= ADC_BUFFER_SIZE) read_index = 0;
  }
}

unsigned int buffer_get(void) {  // READ SAMPLE
  if (write_index == read_index) {
    return 0;  // empty buffer
  }

  unsigned int sample = adc_buffer[read_index];
  read_index++;
  if (read_index >= ADC_BUFFER_SIZE) read_index = 0;
  return sample;
}

// PROCESS SAMPLES IN CHUNKS (rather than processing individually)
#define CHUNK_SIZE 80

void process_available_samples(void) {
  int available_adc_samples;

  while (1) {
    // how many samples are there?
    if (write_index >= read_index) {
      available_adc_samples = write_index - read_index;
    } else {
      available_adc_samples = ADC_BUFFER_SIZE - read_index + write_index;
    }

    if (available_adc_samples <= 0)  // if none break
      break;

    // GET MAX SIZE TO READ
    int samples_to_read;
    if (available_adc_samples > CHUNK_SIZE) {
      samples_to_read = CHUNK_SIZE;
    } else {
      samples_to_read = available_adc_samples;
    }

    // CONVERT VALUES TO VOTLAGE INSTEAD OF RAW ADC VALUES
    for (int i = 0; i < samples_to_read; i++) {
      unsigned int raw_sample = buffer_get();

      float voltage = (raw_sample / 4095.0f) * REF_VOLTAGE;
      adc_samples[adc_sample_count++] = voltage;
    }

    // DIVIDE AND SEND IN CHUNKS
    while (adc_sample_count >= CHUNK_SIZE) {
      // first chunk
      buffer_to_measurements();

      // shift and send
      int remaining = adc_sample_count - CHUNK_SIZE;
      for (int j = 0; j < remaining; j++) {
        adc_samples[j] = adc_samples[j + CHUNK_SIZE];
      }
      adc_sample_count = remaining;
    }
  }

  // IF ANY MORE FULL CHUNKS SEND
  while (adc_sample_count >= CHUNK_SIZE) {
    buffer_to_measurements();

    int remaining = adc_sample_count - CHUNK_SIZE;
    for (int j = 0; j < remaining; j++) {
      adc_samples[j] = adc_samples[j + CHUNK_SIZE];
    }
    adc_sample_count = remaining;
  }
}
