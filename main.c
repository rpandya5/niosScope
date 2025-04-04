#include <stdbool.h>

#define ADC_BASE 0xFF204000
#define LEDs_BASE 0xFF200000
#define GPIO_BASE 0xFF200070
#define BUTTON_BASE 0xFF200050
#define KEY_BASE 0xFF200050
#define TIMER_BASE 0xFF202000
#define SWITCH_BASE 0xFF200040
#define M_PI 3.14159

#define ADC_BUFFER_SIZE 80
#define REF_VOLTAGE 5.0f  // 5V reference

volatile int write_index = 0;

volatile int* LEDS = (int*)LEDs_BASE;
volatile int* BUTTON = (int*)BUTTON_BASE;

volatile int* ADC_ptr = (int*)ADC_BASE;
volatile int* KEY_ptr = (int*)KEY_BASE;
volatile int* SWITCH_ptr = (int*)SWITCH_BASE;

volatile int* interval_timer_ptr =
    (int*)TIMER_BASE;  // interval timer base address

// MANUALLY SET SETTINGS
int TRIGGER = 3;  // IN VOLTAGE

// TRIGGER VARIABLES
bool trigger_mode_active = true;
bool trigger_hold = false;

volatile int raw_adc_samples[ADC_BUFFER_SIZE];
float voltage_samples[ADC_BUFFER_SIZE];
volatile int SAMPLE_RATE = 6000000;  // THE LTC2308 HAS max 500kHz freq

// DRAWING VARIABLES
#define XMAX 320
#define YMAX 240

// COLOURS
#define BLUE 0x0100105
#define GRAY 0xA9A9A9
#define RED 0x808080
#define WHITE 0xFFFFFF
#define BLACK 0

volatile int pixel_buffer_start = 0x08000000;  // global variable
int final_wave[320] = {0};                     // current waveform data
int delete_wave[320] = {0};  // prev waveform data (for erasing)

// SINC FUNCTION
int points[321][2] = {
    {-160, 0},  {-159, 1},  {-158, 1},  {-157, 1},  {-156, 1},  {-155, 1},
    {-154, 0},  {-153, 0},  {-152, 0},  {-151, 0},  {-150, 0},  {-149, 0},
    {-148, 0},  {-147, -1}, {-146, -1}, {-145, -1}, {-144, -1}, {-143, -1},
    {-142, -1}, {-141, 0},  {-140, 0},  {-139, 0},  {-138, 0},  {-137, 0},
    {-136, 0},  {-135, 1},  {-134, 1},  {-133, 1},  {-132, 1},  {-131, 1},
    {-130, 1},  {-129, 1},  {-128, 0},  {-127, 0},  {-126, 0},  {-125, 0},
    {-124, 0},  {-123, 0},  {-122, -1}, {-121, -1}, {-120, -1}, {-119, -1},
    {-118, -1}, {-117, -1}, {-116, -1}, {-115, 0},  {-114, 0},  {-113, 0},
    {-112, 0},  {-111, 0},  {-110, 1},  {-109, 1},  {-108, 1},  {-107, 1},
    {-106, 1},  {-105, 1},  {-104, 1},  {-103, 1},  {-102, 0},  {-101, 0},
    {-100, 0},  {-99, 0},   {-98, -1},  {-97, -1},  {-96, -1},  {-95, -1},
    {-94, -1},  {-93, -1},  {-92, -1},  {-91, -1},  {-90, -1},  {-89, 0},
    {-88, 0},   {-87, 0},   {-86, 1},   {-85, 1},   {-84, 1},   {-83, 1},
    {-82, 1},   {-81, 1},   {-80, 1},   {-79, 1},   {-78, 1},   {-77, 0},
    {-76, 0},   {-75, 0},   {-74, 0},   {-73, -1},  {-72, -1},  {-71, -1},
    {-70, -1},  {-69, -1},  {-68, -1},  {-67, -1},  {-66, -1},  {-65, -1},
    {-64, 0},   {-63, 0},   {-62, 0},   {-61, 1},   {-60, 1},   {-59, 1},
    {-58, 2},   {-57, 2},   {-56, 2},   {-55, 2},   {-54, 1},   {-53, 1},
    {-52, 1},   {-51, 0},   {-50, 0},   {-49, -1},  {-48, -1},  {-47, -1},
    {-46, -2},  {-45, -2},  {-44, -2},  {-43, -2},  {-42, -2},  {-41, -2},
    {-40, -1},  {-39, -1},  {-38, 0},   {-37, 0},   {-36, 1},   {-35, 2},
    {-34, 2},   {-33, 3},   {-32, 3},   {-31, 3},   {-30, 3},   {-29, 3},
    {-28, 2},   {-27, 2},   {-26, 1},   {-25, 0},   {-24, -1},  {-23, -2},
    {-22, -3},  {-21, -4},  {-20, -5},  {-19, -5},  {-18, -5},  {-17, -5},
    {-16, -5},  {-15, -4},  {-14, -2},  {-13, -1},  {-12, 1},   {-11, 3},
    {-10, 6},   {-9, 8},    {-8, 11},   {-7, 13},   {-6, 16},   {-5, 18},
    {-4, 20},   {-3, 22},   {-2, 23},   {-1, 24},   {0, 24},    {1, 24},
    {2, 23},    {3, 22},    {4, 20},    {5, 18},    {6, 16},    {7, 13},
    {8, 11},    {9, 8},     {10, 6},    {11, 3},    {12, 1},    {13, -1},
    {14, -2},   {15, -4},   {16, -5},   {17, -5},   {18, -5},   {19, -5},
    {20, -5},   {21, -4},   {22, -3},   {23, -2},   {24, -1},   {25, 0},
    {26, 1},    {27, 2},    {28, 2},    {29, 3},    {30, 3},    {31, 3},
    {32, 3},    {33, 3},    {34, 2},    {35, 2},    {36, 1},    {37, 0},
    {38, 0},    {39, -1},   {40, -1},   {41, -2},   {42, -2},   {43, -2},
    {44, -2},   {45, -2},   {46, -2},   {47, -1},   {48, -1},   {49, -1},
    {50, 0},    {51, 0},    {52, 1},    {53, 1},    {54, 1},    {55, 2},
    {56, 2},    {57, 2},    {58, 2},    {59, 1},    {60, 1},    {61, 1},
    {62, 0},    {63, 0},    {64, 0},    {65, -1},   {66, -1},   {67, -1},
    {68, -1},   {69, -1},   {70, -1},   {71, -1},   {72, -1},   {73, -1},
    {74, 0},    {75, 0},    {76, 0},    {77, 0},    {78, 1},    {79, 1},
    {80, 1},    {81, 1},    {82, 1},    {83, 1},    {84, 1},    {85, 1},
    {86, 1},    {87, 0},    {88, 0},    {89, 0},    {90, -1},   {91, -1},
    {92, -1},   {93, -1},   {94, -1},   {95, -1},   {96, -1},   {97, -1},
    {98, -1},   {99, 0},    {100, 0},   {101, 0},   {102, 0},   {103, 1},
    {104, 1},   {105, 1},   {106, 1},   {107, 1},   {108, 1},   {109, 1},
    {110, 1},   {111, 0},   {112, 0},   {113, 0},   {114, 0},   {115, 0},
    {116, -1},  {117, -1},  {118, -1},  {119, -1},  {120, -1},  {121, -1},
    {122, -1},  {123, 0},   {124, 0},   {125, 0},   {126, 0},   {127, 0},
    {128, 0},   {129, 1},   {130, 1},   {131, 1},   {132, 1},   {133, 1},
    {134, 1},   {135, 1},   {136, 0},   {137, 0},   {138, 0},   {139, 0},
    {140, 0},   {141, 0},   {142, -1},  {143, -1},  {144, -1},  {145, -1},
    {146, -1},  {147, -1},  {148, 0},   {149, 0},   {150, 0},   {151, 0},
    {152, 0},   {153, 0},   {154, 0},   {155, 1},   {156, 1},   {157, 1},
    {158, 1},   {159, 1},
};

// measurements
#define GOERTZEL_N 10                        // N - number of total bins
#define BIN_WIDTH (SAMPLE_RATE / GOERTZEL_N)  // 1 khz per bin

static float windowed_samples[GOERTZEL_N];  // input signal after widnwoing
static float coeff[GOERTZEL_N / 2 + 1];     // coefficients for (0 -> N-1) / 2
                                            // (symmetry for real values)
static float window[GOERTZEL_N];            // hamming coefficient storage

float dc_offset;
float frequency;
float period;
float minimum_value = 0;
float maximum_value = 0;
float vpp;
float amplitude;
int samples_in_period;

// ALL FUNCTION PROTOTYPES

// basic inputs, isrs, buffers
void trigger_function(int trigger_value);
void interrupt_handler();
void get_samples();
void pushbutton_ISR();
void update_timer(int SAMPLE_RATE);
void the_reset(void) __attribute__((section(".reset")));
void the_exception(void) __attribute__((section(".exceptions")));

// DRAWING FUNCTIONS
void plot_pixel(int x, int y, int line_color);
void plot_point(int x, int y, int line_color);
void draw_line(int x0, int y0, int x1, int y1, int line_color);
void background();
void clear_screen();
void update();
void display_freq();
void display_amplitude();
void display_dc_offset();
void trigger_draw();
void draw_graph(volatile int samples[], int index, float amplitude, int freq);
void plot_character(int x, int y, char c);
void plot_shifted_sinc(int shift, float amplitude);
void display_sample_rate(float value);

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
void produce_points();

volatile bool sampling = false;

int main(void) {
  //update_timer(SAMPLE_RATE);
  *(KEY_ptr + 2) = 0xF;  // write to the pushbutton interrupt mask register
  __builtin_wrctl(3, 0b11);
  __builtin_wrctl(0, 1);  // enable Nios II interrupts

  // ADC SETUP
  *(ADC_ptr + 1) = 1;
  clear_screen();
  background();  // Draw the background grid initially

  // MEASUREMENT SETUP
  get_window();
  get_coeff();
  int dead_points[ADC_BUFFER_SIZE] ={0};
  while (1) {
    // Add a visual indicator that the code is running
    *LEDS = 0xF;

    sampling = false;
    
    produce_points();
    //draw_graph(raw_adc_samples, 0, 1.0, 10);
    *LEDS = 0x0;
    
    /*for (int i=0; i<ADC_BUFFER_SIZE; i++){
      plot_point(i*4, dead_points[i], 0);
      plot_point(i*4, raw_adc_samples[i]*50.0/4095.0, WHITE);
      dead_points[i] =raw_adc_samples[i]*50.0/4095.0;


    }*/
    
    //draw_graph(raw_adc_samples, write_index, 1.0, 10);
    // TRIGGER FUNCTION HANDLES DRAWING BASED ON STATE
    trigger_function(TRIGGER);

    // MEASUREMENTS
    windowing_for_measurement();
    calc_freq_period(&frequency);
    calc_amplitude();
    calc_dc_offset();

    display_freq();
    display_amplitude();
    display_dc_offset();
  }
  return 0;
}
