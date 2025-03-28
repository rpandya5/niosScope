/* updated as of march 28 11:30am. wave is plotted as sawtooth wave on cpulator.
 also plotting L->R instead of R->L. trigger implemented and it works. questions
- are we expecting sawtooth type wave when running on cpulator?
- need to adjust so that it goes R->L instead
- measurement function prototypes added but measurement functions not added.
integration for this is gonna take a second to do
- would be good to also add a horizontal line for where the trigger value is
- add y axis labels ALSO ADJUST RANGE FOR NEGATIVE VOLTAGES????
*/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// ADDRESSES
volatile int* LEDs = 0xFF200000;
volatile int* BUTTON = 0xFF200050;
volatile int pixel_buffer_start = 0x08000000;

#define XMAX 320
#define YMAX 240

#define ADC_BASE 0xFF204000
volatile int* ADC_ptr = (int*)ADC_BASE;

// COLOURS
#define GRAY 0x0100105
#define BLUE 0x0000FF
#define RED 0xFF0000
#define WHITE 0xFFFFFF
#define BLACK 0

// MANUALLY SET SETTINGS
int SAMPLE_RATE = 5;
int TRIGGER = 3;

// TRIGGER VARIABLES
bool trigger_mode_active = true;
bool trigger_hold = false;

// WAVEFORM BUFFERS
int final_wave[320] = {0};   // current waveform data
int delete_wave[320] = {0};  // prev waveform data (for erasing)

// FUNCTIONS
// drawing
void plot_pixel(int x, int y, int line_color);
void plot_point(int x, int y, int line_color);
void draw_line(int x0, int y0, int x1, int y1, int line_color);
void background();
void clear_screen();
void update();

// logic
void trigger_function(int trigger_value);
void plot_shifted_sinc(int shift, float amplitude);

// measurement displays
void display_freq(int value);
void display_amplitude(float value);

// measurement calculations
float calc_dc_offset(const float* samples, int count);
float find_rising_crossing(const float* samples, int count, float threshold,
                           int start);
float find_falling_crossing(const float* samples, int count, float threshold,
                            int start);
void find_min_max(const float* samples, int count, float* min_val,
                  float* max_val);
float calc_vpp(const float* samples, int count);
float calc_rms(const float* samples, int count, float dc_offset);
float calc_frequency(int period_samples);
void calc_rise_fall_time(const float* samples, int count, float* rise_time,
                         float* fall_time);

// POINTS FOR SINC FUNCTION
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

int count = 0;

int main(void) {
  *(ADC_ptr + 1) = 0xFFFFFFFF;
  *(BUTTON + 3) = 0b1111;
  clear_screen();
  background();

  while (1) {
    // BUTTONS CONTROL OSCILLOSCOPE

    // BUTTON 0: Increase sample rate
    if ((*(BUTTON + 3)) & 0b0001) {
      *(BUTTON + 3) = 0b0001;
      SAMPLE_RATE = (SAMPLE_RATE < 20) ? SAMPLE_RATE * 2 : SAMPLE_RATE;
      trigger_hold = false;
    }

    // BUTTON 1: Decrease sample rate
    if ((*(BUTTON + 3)) & 0b0010) {
      *(BUTTON + 3) = 0b0010;
      SAMPLE_RATE = (SAMPLE_RATE > 1) ? SAMPLE_RATE / 2 : SAMPLE_RATE;
      trigger_hold = false;
    }

    // BUTTON 2: INCREASE TRIGGER VALUE
    if ((*(BUTTON + 3)) & 0b0100) {
      *(BUTTON + 3) = 0b0100;
      TRIGGER++;
      trigger_hold = false;
    }

    // BUTTON 3: DECREASE TRIGGER LEVEL or toggle trigger mode
    if ((*(BUTTON + 3)) & 0b1000) {
      *(BUTTON + 3) = 0b1000;
      TRIGGER = (TRIGGER > 0) ? TRIGGER - 1 : 0;
      trigger_hold = false;
    }

    // UPDATE WAVEFORM BUFFER (only if not in trigger hold mode)
    if (!trigger_hold) {
      update();
    }

    background();

    // TRIGGER FUNCTION HANDLES DRAWING BASED ON STATE
    trigger_function(TRIGGER);

    // DISPLAY MEASUREMENTS
    display_freq(0);

    float amplitude = 0.0;
    display_amplitude(amplitude);
  }

  return 0;
}

// DRAWING FUNCTIONS
void plot_pixel(int x, int y, int line_color) {
  volatile int* one_pixel_address;

  one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);

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
}

// OSCILLOSCOPE LOGIC FUNCTIONS
void trigger_function(int trigger_value) {
  // IF WE'RE ALREADY HOLDING A TRIGGERED WAVEFORM, KEEP DISPLAYING IT
  if (trigger_hold) {
    // Just redraw the current waveform (don't update)
    for (int i = 0; i < XMAX - 1; i++) {
      draw_line(i, YMAX / 2 - final_wave[i], i + 1,
                YMAX / 2 - final_wave[i + 1], WHITE);
    }
    return;
  }

  // DO WE HAVE A VALUE >= TRIGGER IN SAMPLES???
  bool trigger_found = false;
  int trigger_index = -1;

  // Look for the first sample that meets or exceeds the trigger value
  for (int i = 0; i < 80; i++) {
    if (final_wave[i] >= trigger_value) {
      trigger_found = true;
      trigger_index = i;
      break;
    }
  }

  if (trigger_found && trigger_mode_active) {
    // WE FOUND A TRIGGER POINT - PLOT 80 SAMPLES FROM TRIGGER INDEX

    // FIRST ERASE THE PREVIOUS WAVEFORM
    for (int i = 0; i < XMAX - 1; i++) {
      draw_line(i, YMAX / 2 - delete_wave[i], i + 1,
                YMAX / 2 - delete_wave[i + 1], BLACK);
    }

    // DRAW THE TRIGGERED SAMPLES
    for (int i = 0; i < XMAX - 1; i++) {
      if (i + trigger_index < XMAX) {
        // Draw the samples starting from trigger point
        draw_line(i, YMAX / 2 - final_wave[i + trigger_index], i + 1,
                  YMAX / 2 - final_wave[i + trigger_index + 1], WHITE);
      } else {
        // ONCE WE'VE DRAWN ALL AVAILABLE SAMPLES, STOP
        break;
      }
    }

    // SET HOLD MODE UNTIL USER CHANGES SOMETHING
    trigger_hold = true;
  } else {
    // NO TRIGGER FOUND, OR TRIGGER MODE NOT ACTIVE - PLOT NORMALLY
    for (int i = 0; i < XMAX - 1; i++) {
      draw_line(i, YMAX / 2 - delete_wave[i], i + 1,
                YMAX / 2 - delete_wave[i + 1], BLACK);
      if (i < XMAX - 1) {
        draw_line(i, YMAX / 2 - final_wave[i], i + 1,
                  YMAX / 2 - final_wave[i + 1], WHITE);
      }
    }
  }

  // STORE CURRENT WAVEFORM FOR BACKGROUND ERASURE ON NEXT UPDATE
  for (int i = 0; i < XMAX; i++) {
    delete_wave[i] = final_wave[i];
  }
}

void update() {
  // CLEAR THE FINAL_WAVE ARRAY BEFORE PLOTTING NEW DATA
  for (int i = 0; i < XMAX; i++) {
    final_wave[i] = 0;
  }

  // SAMPLE DATA FROM ADC
  int samples[80] = {0};
  float voltageFloat;

  // COLLECT 80 SAMPLES FROM ADC WITH APPROPRIATE SAMPLING RATE
  for (int i = 0; i < 80; i++) {
    // IMPLEMENT DELAY BASED ON SAMPLE RATE IF NEEDED
    for (int j = 0; j < SAMPLE_RATE; j++) {
      // Simple delay loop
    }

    // READ SAMPLE FROM ADC (12-BIT VALUE: 0-4095)
    samples[i] = *(ADC_ptr) & 0xFFF;

    // CONVERT TO VOLTAGE (0-1.0 RANGE)
    voltageFloat = samples[i] * (1.0 / 4095.0);

    // SCALE THE VOLTAGE TO APPROPRIATE DISPLAY SIZE
    int scaled_value = (int)(voltageFloat * 100);

    // PLOT THE SAMPLE USING THE SINC INTERPOLATION
    plot_shifted_sinc(i * 4 - XMAX / 2, voltageFloat);
  }
}
