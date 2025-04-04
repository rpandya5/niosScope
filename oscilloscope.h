#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <stdbool.h>

// Hardware addresses
#define ADC_BASE 0xFF204000
#define LEDs_BASE 0xFF200000
#define GPIO_BASE 0xFF200070
#define BUTTON_BASE 0xFF200050
#define KEY_BASE 0xFF200050
#define TIMER_BASE 0xFF202000
#define SWITCH_BASE 0xFF200040
#define M_PI 3.14159

// Buffer and display settings
#define ADC_BUFFER_SIZE 80
#define REF_VOLTAGE 5.0f  // 5V reference
#define XMAX 320
#define YMAX 240

// Measurements
#define GOERTZEL_N 10
#define BIN_WIDTH (SAMPLE_RATE / GOERTZEL_N)

// COLOURS
#define BLUE 0x0100105
#define GRAY 0xA9A9A9
#define RED 0x808080
#define WHITE 0xFFFFFF
#define BLACK 0

// Global variables
extern volatile int write_index;
extern volatile int* LEDS;
extern volatile int* BUTTON;
extern volatile int* ADC_ptr;
extern volatile int* KEY_ptr;
extern volatile int* SWITCH_ptr;
extern volatile int* interval_timer_ptr;

extern int TRIGGER;
extern bool trigger_mode_active;
extern bool trigger_hold;

extern volatile int raw_adc_samples[ADC_BUFFER_SIZE];
extern float voltage_samples[ADC_BUFFER_SIZE];
extern volatile int SAMPLE_RATE;

extern volatile int pixel_buffer_start;
extern int final_wave[320];
extern int delete_wave[320];
extern int points[321][2];

extern float dc_offset;
extern float frequency;
extern float period;
extern float minimum_value;
extern float maximum_value;
extern float vpp;
extern float amplitude;
extern int samples_in_period;

extern static float windowed_samples[GOERTZEL_N];
extern static float coeff[GOERTZEL_N / 2 + 1];
extern static float window[GOERTZEL_N];

extern volatile bool sampling;

// VGA/Display functions
void plot_pixel(int x, int y, int line_color);
void plot_point(int x, int y, int line_color);
void draw_line(int x0, int y0, int x1, int y1, int line_color);
void background();
void clear_screen();
void update();
void display_freq();
void display_amplitude();
void display_sample_rate(float value);
void display_dc_offset();
void trigger_draw();
void draw_graph(volatile int samples[], int index, float amplitude, int freq);
void plot_character(int x, int y, char c);
void plot_shifted_sinc(int shift, float amplitude);

// Measurement functions
float cosine(float x);
void get_coeff();
void get_window();
void process_goertzel(int sample_rate, float* freq);
void calc_freq_period(float* frequency);
void windowing_for_measurement();
void calc_rise_fall_time(float* rise_time, float* fall_time);
void calc_amplitude();
void calc_vpp();
void find_min_max();
void calc_dc_offset();
float find_falling_crossing(float threshold);
float find_rising_crossing(float threshold);

// Logic/control functions
void trigger_function(int trigger_value);
void interrupt_handler();
void get_samples();
void pushbutton_ISR();
void update_timer(int SAMPLE_RATE);
void the_reset(void) __attribute__((section(".reset")));
void the_exception(void) __attribute__((section(".exceptions")));
void produce_points();

#endif /* OSCILLOSCOPE_H */
