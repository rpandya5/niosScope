#include <stdbool.h>

// Function prototypes
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

void trigger_draw() {
  draw_line(10, (YMAX / 2) - (TRIGGER * 10), XMAX - 1,
            (YMAX / 2) - (TRIGGER * 10), RED);
}

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

void display_freq() {
  int display_value = frequency;
	
  if (frequency >-1000){display_value/=1000;}
  plot_character(17, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(16, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(15, 58, '0' + display_value % 10);
}

void display_amplitude() {
  int display_value = amplitude * 100;
  plot_character(7,58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(6, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(4,58, '0' + display_value % 10);
}

void display_sample_rate(float value) {
  int display_value = value * 100;
  plot_character(23, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(24, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(25,58, '0' + display_value % 10);
}

void display_dc_offset() {
  int display_value = dc_offset * 100;
  plot_character(39, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(37, 58, '.');
  plot_character(38, 58, '0' + display_value % 10);
  display_value /= 10;
  plot_character(36, 58, '0' + display_value % 10);
}

void background() {
  draw_line(0, YMAX / 2, XMAX - 1, YMAX / 2, BLUE);
  draw_line(0, YMAX / 2 + 1, XMAX - 1, YMAX / 2 + 1, BLUE);
  draw_line(0, YMAX / 2 - 1, XMAX - 1, YMAX / 2 - 1, BLUE);

  draw_line(XMAX / 2, 0, XMAX / 2, YMAX - 1, BLUE);
  draw_line(XMAX / 2 + 1, 0, XMAX / 2 + 1, YMAX - 1, BLUE);
  draw_line(XMAX / 2 - 1, 0, XMAX / 2 - 1, YMAX - 1, BLUE);

  for (int i = 0; i < XMAX - 1; i = i + 10) {
    draw_line(i, 0, i, YMAX - 1, BLUE);
  }
  for (int i = 0; i < YMAX - 1; i = i + 10) {
    draw_line(0, i, XMAX - 1, i, BLUE);
  }

  // PLOT AXES + VALUES
  //plot_character(2, 15, '5');
  plot_character(2, 31, '0');
  draw_line(12, YMAX / 2 + 1, XMAX - 1, YMAX / 2 + 1, WHITE);  // X AXIS
  draw_line(12, 0, 12, YMAX - 15, WHITE);                      // y axis

  // PLOT MEASUREMENTS
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
  if (frequency >1000){
    plot_character(18, 58, 'k');
    plot_character(19, 58, 'H');
    plot_character(20, 58, 'z');
  }
  else{
    plot_character(18, 58, 'H');
    plot_character(19, 58, 'z');
    plot_character(20, 58, ' ');
  }

  plot_character(22, 58, 'T');
  plot_character(23, 58, 's');
  plot_character(24, 58, '=');

  plot_character(32, 58, 'D');
  plot_character(33, 58, 'C');
  plot_character(34, 58, '=');
  plot_character(40, 58, 'v');


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
      plot_character(30, 58, ' ');
    }
  }
}


void draw_graph(volatile int samples[], int index, float amplitude, int freq) {
  // CLEAR THE FINAL_WAVE ARRAY BEFORE PLOTTING NEW DATA
  for (int i = 0; i < XMAX; i++) {
    final_wave[i] = 0;
  }
  float voltageFloat;
  for (int i = index; i < ADC_BUFFER_SIZE; i++) {
    // for (int j = 0; j<SAMPLE_RATE; j++){}
    voltageFloat = samples[i] * (1.0 / 4095.0);
    plot_shifted_sinc((i-index) * 4 - XMAX / 2, voltageFloat);
  }

  for (int i = 0; i < XMAX - 1; i++) {
    draw_line(i, YMAX / 2 - delete_wave[i], i + 1,
              YMAX / 2 - delete_wave[i + 1], BLACK);}
    background();
  for (int i = 0; i < XMAX - 1; i++) {
    draw_line(i, YMAX / 2 - final_wave[i], i + 1, YMAX / 2 - final_wave[i + 1],
              WHITE);
    delete_wave[i] = final_wave[i];
    final_wave[i] = 0;
  }

  delete_wave[XMAX - 1] = final_wave[XMAX - 1];
  final_wave[XMAX - 1] = 0;
  // display_freq(frequency);
  // display_amplitude(amplitude);
}
