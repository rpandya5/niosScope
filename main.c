/* UPDATED AS OF 10:31 PM
ADC , INPUTS , ISRS
MEASUREMENTS
DRAWINGS*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADC_BASE 0xFF204000
#define LEDs_BASE 0xFF200000
#define GPIO_BASE 0xFF200070
#define BUTTON_BASE 0xFF200050
#define KEY_BASE 0xFF200050
#define TIMER_BASE 0xFF202000

#define ADC_BUFFER_SIZE 1024
#define REF_VOLTAGE 5.0f  // 5V reference

volatile int write_index = 0;
volatile int read_index = 0;

volatile int* LEDS = (int*)LEDs_BASE;
volatile int* BUTTON = (int*)BUTTON_BASE;

volatile int* ADC_ptr = (int*)ADC_BASE;
volatile int* KEY_ptr = (int*)KEY_BASE;
volatile int* interval_timer_ptr =
    (int*)TIMER_BASE;  // interal timer base address

// MANUALLY SET SETTINGS
int TRIGGER = 1000;

// TRIGGER VARIABLES
bool trigger_mode_active = true;
bool trigger_hold = false;

int raw_adc_samples[ADC_BUFFER_SIZE];
float voltage_samples[ADC_BUFFER_SIZE];
int SAMPLE_RATE = 500000;  // THE LTC2308 HAS max 500kHz freq

// ALL FUNCTION PROTOTYPES

// basic inputs, isrs, buffers
void interrupt_handler();
void get_samples();
void pushbutton_ISR();
void update_timer(int SAMPLE_RATE);
void the_reset(void) __attribute__((section(".reset")));

void the_reset(void) {
  asm(".set noat");       /* Instruct the assembler NOT to use reg at (r1) as
                           * a temp register for performing optimizations */
  asm(".set nobreak");    /* Suppresses a warning message that says that
                           * some debuggers corrupt regs bt (r25) and ba
                           * (r30)
                           */
  asm("movia r2, main");  // Call the C language main program
  asm("jmp r2");
}

void the_exception(void) __attribute__((section(".exceptions")));
void the_exception(void) {
  asm("subi sp, sp, 128");
  asm("stw et, 96(sp)");
  asm("rdctl et, ctl4");
  asm("beq et, r0, SKIP_EA_DEC");  // Interrupt is not external
  asm("subi ea, ea, 4");           /* Must decrement ea by one instruction
                                    * for external interupts, so that the
                                    * interrupted instruction will be run */
  asm("SKIP_EA_DEC:");
  asm("stw r1, 4(sp)");  // Save all registers
  asm("stw r2, 8(sp)");
  asm("stw r3, 12(sp)");
  asm("stw r4, 16(sp)");
  asm("stw r5, 20(sp)");
  asm("stw r6, 24(sp)");
  asm("stw r7, 28(sp)");
  asm("stw r8, 32(sp)");
  asm("stw r9, 36(sp)");
  asm("stw r10, 40(sp)");
  asm("stw r11, 44(sp)");
  asm("stw r12, 48(sp)");
  asm("stw r13, 52(sp)");
  asm("stw r14, 56(sp)");
  asm("stw r15, 60(sp)");
  asm("stw r16, 64(sp)");
  asm("stw r17, 68(sp)");
  asm("stw r18, 72(sp)");
  asm("stw r19, 76(sp)");
  asm("stw r20, 80(sp)");
  asm("stw r21, 84(sp)");
  asm("stw r22, 88(sp)");
  asm("stw r23, 92(sp)");
  asm("stw r25, 100(sp)");  // r25 = bt (skip r24 = et, because it is saved
  // above)
  asm("stw r26, 104(sp)");  // r26 = gp
  // skip r27 because it is sp, and there is no point in saving this
  asm("stw r28, 112(sp)");  // r28 = fp
  asm("stw r29, 116(sp)");  // r29 = ea
  asm("stw r30, 120(sp)");  // r30 = ba
  asm("stw r31, 124(sp)");  // r31 = ra
  asm("addi fp, sp, 128");
  asm("call interrupt_handler");  // Call the C language interrupt handler
  asm("ldw r1, 4(sp)");           // Restore all registers
  asm("ldw r2, 8(sp)");
  asm("ldw r3, 12(sp)");
  asm("ldw r4, 16(sp)");
  asm("ldw r5, 20(sp)");
  asm("ldw r6, 24(sp)");
  asm("ldw r7, 28(sp)");
  asm("ldw r8, 32(sp)");
  asm("ldw r9, 36(sp)");
  asm("ldw r10, 40(sp)");
  asm("ldw r11, 44(sp)");
  asm("ldw r12, 48(sp)");
  asm("ldw r13, 52(sp)");
  asm("ldw r14, 56(sp)");
  asm("ldw r15, 60(sp)");
  asm("ldw r16, 64(sp)");
  asm("ldw r17, 68(sp)");
  asm("ldw r18, 72(sp)");
  asm("ldw r19, 76(sp)");
  asm("ldw r20, 80(sp)");
  asm("ldw r21, 84(sp)");
  asm("ldw r22, 88(sp)");
  asm("ldw r23, 92(sp)");
  asm("ldw r24, 96(sp)");
  asm("ldw r25, 100(sp)");  // r25 = bt
  asm("ldw r26, 104(sp)");  // r26 = gp
  // skip r27 because it is sp, and we did not save this on the stack
  asm("ldw r28, 112(sp)");  // r28 = fp
  asm("ldw r29, 116(sp)");  // r29 = ea
  asm("ldw r30, 120(sp)");  // r30 = ba
  asm("ldw r31, 124(sp)");  // r31 = ra
  asm("addi sp, sp, 128");
  asm("eret");
}

// DRAWING FUNCTIONS
void plot_pixel(int x, int y, int line_color);
void plot_point(int x, int y, int line_color);
void draw_line(int x0, int y0, int x1, int y1, int line_color);
void background();
void clear_screen();
void update();
void display_freq(int value);
void display_amplitude(float value);

// DRAWING VARIABLES
#define XMAX 320
#define YMAX 240

// COLOURS
#define GRAY 0x0100105
#define BLUE 0x0000FF
#define RED 0xFF0000
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
#define GOERTZEL_N 500                        // N - number of total bins
#define BIN_WIDTH (SAMPLE_RATE / GOERTZEL_N)  // 1 khz per bin

static float windowed_samples[GOERTZEL_N];  // input signal after widnwoing
static float coeff[GOERTZEL_N / 2 + 1];     // coefficients for (0 -> N-1) / 2
                                            // (symmetry for real values)
static float window[GOERTZEL_N];            // hamming coefficient storage

float dc_offset;
float frequency;
float period;
float minimum_value;
float maximum_value;
float vpp;
float amplitude;
float rms;
int samples_in_period;

void get_coeff();
void get_window();
void process_goertzel(int sample_rate, float* freq);
void calc_freq_period(float* frequency);
void windowing_for_measurement();

void calc_rise_fall_time(float* rise_time, float* fall_time);
void calc_rms();
void calc_amplitude();
void calc_vpp();
void find_min_max();
void calc_dc_offset();
float find_falling_crossing(float threshold);

void draw_graph(int samples[], int index, float amplitude, int freq);

int main(void) {
  update_timer(SAMPLE_RATE);
  //*(KEY_ptr + 2) = 0x3; // enable interrupts for all pushbuttons
  *(KEY_ptr + 2) = 0xF;  // write to the pushbutton interrupt mask register
  __builtin_wrctl(3, 0b11);
  __builtin_wrctl(0, 1);  // enable Nios II interrupts

  // ADC SETUP
  *(ADC_ptr + 1) = 0xFFFFFFFF;
  clear_screen();

  // MEASUREMENT SETUP
  get_window();
  get_coeff();

  while (1) {
    // UPDATE WAVEFORM BUFFER (only if not in trigger hold mode)

    // TRIGGER FUNCTION HANDLES DRAWING BASED ON STATE
    trigger_function(TRIGGER);
    //*LEDS = write_index; //WRITE OR READ?

    // MEASUREMENTS
    windowing_for_measurement();
    calc_freq_period(&frequency);
  }
  return 0;
}

//////////////////// DISPLAY CODE ///////////////////////
void plot_pixel(int x, int y, int line_color) {
  volatile int* one_pixel_address;

  one_pixel_address = (int*)(0x08000000 + (y << 10) + (x << 1));

  *one_pixel_address = line_color;
}

void plot_character(int x, int y, char c) {
  volatile char* character_buffer = (char*)(0x09000000 + (y << 7) + x);
  *character_buffer = c;
}

void clear_screen() {
  for (int i = 0; i < 60; i++) {
    for (int j = 0; j < 80; j++) {
      plot_character(i, j, ' ');
    }
  }
  for (int i = 0; i < XMAX; i++) {
    for (int j = 0; j < YMAX; j++) {
      plot_pixel(i, j, 0);
    }
  }
}

void draw_line(int x0, int y0, int x1, int y1, int line_color) {
  int deltaX = x1 - x0;
  int deltaY = y1 - y0;

  if ((deltaX == 0) && (deltaY == 0)) {
    plot_pixel(x0, y0, line_color);
  } else if (abs(deltaX) >= abs(deltaY)) {
    if (deltaX > 0) {
      for (int i = 0; i <= deltaX; i++) {
        plot_pixel(x0 + i, (deltaY * (i)) / deltaX + y0, line_color);
      }
    } else {
      for (int i = deltaX; i < 0; i++) {
        plot_pixel(x0 + i, (deltaY * (i)) / deltaX + y0, line_color);
      }
    }
  } else {
    if (deltaY > 0) {
      for (int i = 0; i < deltaY; i++) {
        plot_pixel((deltaX * (i)) / deltaY + x0, y0 + i, line_color);
      }
    } else {
      for (int i = deltaY; i < 0; i++) {
        plot_pixel((deltaX * (i)) / deltaY + x0, y0 + i, line_color);
      }
    }
  }
}

void plot_point(int x, int y, int line_color) {
  if (abs(y) < YMAX / 2) {
    plot_pixel(x, YMAX / 2 - y, line_color);
  }
}
void plot_shifted_sinc(int shift, float amplitude) {
  for (int i = 0; i < 321; i++) {
    // the linear offset I believe is needed because of rounding errors, I think
    // I will need a second list for the larger SINC
    if (abs(points[i][0] + shift) < XMAX / 2) {
      final_wave[points[i][0] + shift + XMAX / 2] +=
          (int)points[i][1] * amplitude;
      // points[i][0]+shift+XMAX/2
    }
  }
}
void display_freq(int value) {
  int display_value = value;
  plot_character(15, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(16, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(17, 58, '0' + display_value % 10);
}

void display_amplitude(float value) {
  int display_value = value * 100;
  plot_character(4, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(6, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(7, 58, '0' + display_value % 10);
}

void display_sample_rate(float value) {
  int display_value = value * 100;
  plot_character(4, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(6, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(7, 58, '0' + display_value % 10);
}

void background() {
  draw_line(0, YMAX / 2, XMAX - 1, YMAX / 2, GRAY);
  draw_line(0, YMAX / 2 + 1, XMAX - 1, YMAX / 2 + 1, GRAY);
  draw_line(0, YMAX / 2 - 1, XMAX - 1, YMAX / 2 - 1, GRAY);

  draw_line(XMAX / 2, 0, XMAX / 2, YMAX - 1, GRAY);
  draw_line(XMAX / 2 + 1, 0, XMAX / 2 + 1, YMAX - 1, GRAY);
  draw_line(XMAX / 2 - 1, 0, XMAX / 2 - 1, YMAX - 1, GRAY);

  for (int i = 0; i < XMAX - 1; i = i + 10) {
    draw_line(i, 0, i, YMAX - 1, GRAY);
  }
  for (int i = 0; i < YMAX - 1; i = i + 10) {
    draw_line(0, i, XMAX - 1, i, GRAY);
  }

  plot_character(0, 58, 'A');
  plot_character(1, 58, 'M');
  plot_character(2, 58, 'P');
  plot_character(3, 58, '=');
  plot_character(5, 58, '.');
  // AMP=0.00V

  plot_character(8, 58, 'V');

  plot_character(10, 58, 'F');
  plot_character(11, 58, 'R');
  plot_character(12, 58, 'E');
  plot_character(13, 58, 'Q');
  plot_character(14, 58, '=');

  plot_character(18, 58, 'H');
  plot_character(19, 58, 'z');

  plot_character(22, 58, 'T');
  plot_character(23, 58, 's');
  plot_character(24, 58, '=');

  int sample_time_seconds = SAMPLE_RATE / 25;
  if (sample_time_seconds < 1000) {
    plot_character(25, 58, '0' + (sample_time_seconds / 100) % 10);
    plot_character(26, 58, '0' + (sample_time_seconds / 10) % 10);
    plot_character(27, 58, '0' + (sample_time_seconds) % 10);
    plot_character(29, 58, 'u');
    plot_character(30, 58, 's');
  } else {
    sample_time_seconds = sample_time_seconds / 1000;
    if (sample_time_seconds < 1000) {
      plot_character(25, 58, '0' + (sample_time_seconds / 100) % 10);
      plot_character(26, 58, '0' + (sample_time_seconds / 10) % 10);
      plot_character(27, 58, '0' + (sample_time_seconds) % 10);
      plot_character(29, 58, 'm');
      plot_character(30, 58, 's');
    } else {
      sample_time_seconds = sample_time_seconds / 1000;
      plot_character(25, 58, '0' + (sample_time_seconds / 100) % 10);
      plot_character(26, 58, '0' + (sample_time_seconds / 10) % 10);
      plot_character(27, 58, '0' + (sample_time_seconds) % 10);
      plot_character(29, 58, 's');
      plot_character(29, 58, ' ');
    }
  }
}

void draw_graph(int samples[], int index, float amplitude, int freq) {
  // CLEAR THE FINAL_WAVE ARRAY BEFORE PLOTTING NEW DATA
  for (int i = 0; i < XMAX; i++) {
    final_wave[i] = 0;
  }
  float voltageFloat;
  for (int i = 0; i < 80; i++) {
    // for (int j = 0; j<SAMPLE_RATE; j++){}
    voltageFloat = samples[(index + i) % 80] * (1.0 / 4095.0);
    plot_shifted_sinc(i * 4 - XMAX / 2, voltageFloat);
  }
  background();
  for (int i = 0; i < XMAX - 1; i++) {
    draw_line(i, YMAX / 2 - delete_wave[i], i + 1,
              YMAX / 2 - delete_wave[i + 1], BLACK);
    draw_line(i, YMAX / 2 - final_wave[i], i + 1, YMAX / 2 - final_wave[i + 1],
              WHITE);
    delete_wave[i] = final_wave[i];
    final_wave[i] = 0;
  }
  delete_wave[XMAX - 1] = final_wave[XMAX - 1];
  final_wave[XMAX - 1] = 0;
  display_freq(0);
  display_amplitude(0.0);
}

////////////////// ACQUIRE CODE /////////////////////

void update_timer(int sample_rate) {
  *(interval_timer_ptr + 0x2) = (sample_rate & 0xFFFF);
  *(interval_timer_ptr + 0x3) = (sample_rate >> 16) & 0xFFFF;
  *(interval_timer_ptr + 1) = 0x7;  // STOP = 0, START = 1, CONT = 1, ITO = 1
}

void interrupt_handler(void) {
  int ipending;
  ipending = __builtin_rdctl(4);

  if (ipending & 0x1) {
    get_samples();
  } else {
    pushbutton_ISR();
  }
  return;
}

void get_samples() {
  interval_timer_ptr = (int*)0xFF202000;
  *(interval_timer_ptr) = 0;  // clear the interrupt

  raw_adc_samples[write_index] = (*ADC_ptr) & 0xFFF;

  voltage_samples[write_index] =
      raw_adc_samples[write_index] * REF_VOLTAGE /
      4095.0;  // CONVERT TO VOLTAGE AND ADD TO VOLTAGE_SAMPLES LIST
  write_index = (write_index + 1) % ADC_BUFFER_SIZE;
}

void pushbutton_ISR() {
  trigger_hold = false;
  volatile int* KEY_ptr = (int*)0xFF200050;
  int press;
  press = *(KEY_ptr + 3);  // read the pushbutton interrupt register
  *(KEY_ptr + 3) = press;  // clear the interrupt
  if (press & 0x1) {
    SAMPLE_RATE =
        ((SAMPLE_RATE & (1 << 30)) == 0) ? SAMPLE_RATE * 2 : SAMPLE_RATE;
    update_timer(SAMPLE_RATE);
  } else if (press & 0x2) {
    SAMPLE_RATE = (SAMPLE_RATE > 1) ? SAMPLE_RATE / 2 : SAMPLE_RATE;
    update_timer(SAMPLE_RATE);
  } else if (press & 0x4) {
    TRIGGER++;
  } else {
    TRIGGER = (TRIGGER > 0) ? TRIGGER - 1 : 0;
  }
  return;
}

/////////// LOGIC FUNCTIONS //////////////////////

// OSCILLOSCOPE LOGIC FUNCTIONS
void trigger_function(int trigger_value) {
  // IF WE'RE ALREADY HOLDING A TRIGGERED WAVEFORM, KEEP DISPLAYING IT
  /*if (trigger_hold) {
    // Just redraw the current waveform (don't update)
    for (int i = 0; i < XMAX - 1; i++) {
      draw_line(i, YMAX / 2 - final_wave[i], i + 1,
                YMAX / 2 - final_wave[i + 1], WHITE);
    }
    return;
  }*/

  // DO WE HAVE A VALUE >= TRIGGER IN SAMPLES???
  bool trigger_found = false;
  int trigger_index = -1;

  // Look for the first sample that meets or exceeds the trigger value
  for (int i = 0; i < 80; i++) {
    if ((raw_adc_samples[i] >= trigger_value) &&
        (raw_adc_samples[(i + 1) % 80] <= trigger_value)) {
      trigger_found = true;
      trigger_index = i;
      break;
    }
  }

  if (trigger_found && trigger_mode_active) {
    // WE FOUND A TRIGGER POINT - PLOT 80 SAMPLES FROM TRIGGER INDEX

    draw_graph(raw_adc_samples, write_index, 1.0, 10);
    // SET HOLD MODE UNTIL USER CHANGES SOMETHING
    trigger_hold = true;
  } else { /*
     // NO TRIGGER FOUND, OR TRIGGER MODE NOT ACTIVE - PLOT NORMALLY
     for (int i = 0; i < XMAX - 1; i++) {
       draw_line(i, YMAX / 2 - delete_wave[i], i + 1,
                 YMAX / 2 - delete_wave[i + 1], BLACK);
       if (i < XMAX - 1) {
         draw_line(i, YMAX / 2 - final_wave[i], i + 1,
                   YMAX / 2 - final_wave[i + 1], WHITE);
       }
     }*/
  }
}

/////// MEASUREMENT FUNCTIONS ////////
float cosine(float x) {
  float result = 1;
  float x_square = x * x;
  float power = x_square;
  int factorial = 2;
  int sign = -1;

  for (int i = 1; i < 9; i++) {
    result += sign * power / factorial;
    power *= x_square;
    factorial *= (2 * i + 1) * (2 * i + 2);
    sign = -sign;
  }
  return result;
}

// WINDOWING FUNCTION NEEDED THAT SPLITS INTO CHUNKS OF 500
void windowing_for_measurement() {
  for (int i = 0; i < GOERTZEL_N; i++) {
    windowed_samples[i] = voltage_samples[i] * window[i];
  }
}

void calc_freq_period(float* frequency) {
  process_goertzel(SAMPLE_RATE, frequency);
  period = 1.0 / *frequency;
  samples_in_period = (int)period * SAMPLE_RATE;
}

/* GET THE CONSTANT COEFFICIENTS FOR ALGORITHM coefficient[k] = 2cos(2pik/N)
only do from 0 -> N/2 because dft is symmetric for real valued inputs */
void get_coeff() {
  for (int k = 0; k <= GOERTZEL_N / 2; k++) {
    coeff[k] = 2.0f * cosine(2.0f * M_PI * (float)k / (float)GOERTZEL_N);
  }
}

/* GET THE HAMMING WINDOW COEFFICIENTS FOR A BLOCK
window[n] = 0.54 - 0.46cos(2pin / (N-1))
goal is to basically taper the bin rather than sharp output so no spectral
leaking
 */
void get_window() {
  for (int n = 0; n < GOERTZEL_N; n++) {
    window[n] = 0.54f - 0.46f * cosine(2.0f * M_PI * n / (GOERTZEL_N - 1));
  }
}

void process_goertzel(int sample_rate, float* frequency) {
  float max_magnitude_sq = 0.0f;
  int max_k = 0;
  /* loop over each freq bin frm n=0 to N-1 */
  for (int k = 1; k <= GOERTZEL_N / 2; k++) {
    float q0, q1 = 0.0f, q2 = 0.0f;
    float coeff_k = coeff[k];

    /* finds RECURRENCE q(n) = windowed_samples[n] + coeff[k] * q(n-1) - q(n-2)
    q1 / q2 = n-1 and n-2 btw */
    for (int n = 0; n < GOERTZEL_N; n++) {
      q0 = coeff_k * q1 - q2 + windowed_samples[n];

      // SHIFT
      q2 = q1;
      q1 = q0;
    }

    // find power aka mangitude squared: |X[k]|^2 = q1^2 + q2^2 - coeff[k] * q1
    // * q2
    float magnitude_sq = q1 * q1 + q2 * q2 - q1 * q2 * coeff_k;

    if (magnitude_sq > max_magnitude_sq) {  // IS IT MAX POWER FOUND SO FAR? if
                                            // y then freq found
      max_magnitude_sq = magnitude_sq;
      max_k = k;
    }
  }

  *frequency = (max_k * sample_rate) / (float)GOERTZEL_N;
}

// FIND THE DC OFFSET
void calc_dc_offset() {
  // Find the average value (sum / count) to determine the center point of the
  // signal
  if (period <= 0.0f) {
    dc_offset = 0.0f;
    return;
  }

  float sum = 0.0f;

  for (int i = 0; i < samples_in_period; i++) {
    sum += voltage_samples[i];
  }

  dc_offset = sum / samples_in_period;
}

/* FIND RISING CROSSING
rising crossing is when signal crosses the centre point (dc offset) */
float find_rising_crossing(float threshold) {
  // returns the first point where the signal crosses the threshold value (dc
  // offset)
  if (period <= 0) return 0.0f;  // no samples

  for (int i = 0; i < samples_in_period - 1; i++) {
    if (voltage_samples[i] < threshold && voltage_samples[i + 1] >= threshold) {
      float delta = voltage_samples[i + 1] - voltage_samples[i];
      if (delta == 0) return i;  // Avoid division by zero
      float fraction = (threshold - voltage_samples[i]) / delta;
      return i + fraction;
    }
  }
  return -1.0f;
}

// FIND FALLING CROSSING
float find_falling_crossing(float threshold) {
  if (period <= 0) return 0.0f;  // no samples

  for (int i = 0; i < samples_in_period - 1; i++) {
    if (voltage_samples[i] >= threshold && voltage_samples[i + 1] < threshold) {
      float delta = voltage_samples[i] - voltage_samples[i + 1];
      if (delta == 0) return i;
      float fraction = (voltage_samples[i] - threshold) / delta;
      return i + fraction;
    }
  }
  return -1.0f;
}

// MIN / MAX VALUES
void find_min_max() {
  if (period <= 0) {  // no samples
    minimum_value = 0.0f;
    maximum_value = 0.0f;
    return;
  }

  minimum_value = voltage_samples[0];
  maximum_value = voltage_samples[0];

  for (int i = 1; i < samples_in_period; i++) {
    if (voltage_samples[i] > maximum_value) maximum_value = voltage_samples[i];
    if (voltage_samples[i] < minimum_value) minimum_value = voltage_samples[i];
  }
}

// PEAK TO PEAK VOLTAGE
void calc_vpp() {
  // vpp = max - min
  if (period <= 0) vpp = 0.0f;  // no samples

  if (minimum_value == 0 && maximum_value == 0) {
    find_min_max();
  }

  vpp = maximum_value - minimum_value;
}

// AMPLITUDE = VPP / 2
void calc_amplitude() {
  if (vpp == 0) {
    calc_vpp();
  }
  amplitude = vpp / 2.0;
}

// RMS VOLTAGE
void calc_rms() {
  // Vrms = ((Vi^2) / count)^0.5 - WORKS FOR ALL NOT JUST SINE WAVES

  if (period <= 0) return 0.0f;  // no samples
  if (dc_offset == 0) {
    calc_dc_offset();
  }

  float sum_sq = 0.0;

  for (int i = 0; i < samples_in_period; i++) {
    float ac = voltage_samples[i] - dc_offset;
    sum_sq += ac * ac;
  }
  rms = powf((sum_sq / samples_in_period), 0.5);
}

/* RISE AND FALL TIME
rise = 10% - 90% of amplitude
fall = 90% - 10% of amplitude */

void calc_rise_fall_time(float* rise_time, float* fall_time) {
  if (period <= 0) {  // NO SAMPLES
    *rise_time = 0.0f;
    *fall_time = 0.0f;
    return;
  }

  if (minimum_value == 0 && maximum_value == 0) {
    find_min_max();
  }

  if (amplitude == 0) {
    amplitude = maximum_value - minimum_value;
  }

  float threshold_low = minimum_value + 0.1f * amplitude;
  float threshold_high = minimum_value + 0.9f * amplitude;

  // FIND RISE TIME - find first time where crosses low threshold then when it
  // crosses high threshold and subtract
  float rise_start = find_rising_crossing(threshold_low);
  float rise_end = -1.0f;
  if (rise_start != -1) {
    rise_end = find_rising_crossing(threshold_high);
  }
  if (rise_start != -1 && rise_end != -1) {
    *rise_time = (rise_end - rise_start) / SAMPLE_RATE;
  } else {
    *rise_time = 0.0f;
  }

  /* FIND FALL TIME - find first time where crosses high threshold then when it
  crosses low threshold and subtract only start AFTER PEAK */

  float fall_start = find_falling_crossing(threshold_high);
  float fall_end = -1.0f;
  if (fall_start != -1) {
    fall_end = find_falling_crossing(threshold_low);
  }
  if (fall_start != -1 && fall_end != -1 && fall_end > fall_start) {
    *fall_time = (fall_end - fall_start) / SAMPLE_RATE;
  } else {
    *fall_time = 0.0f;
  }
}
