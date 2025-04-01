#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GOERTZEL_N 500                        // N - number of total bins
#define SAMPLE_RATE 500000.0f                 // 500 kHz sample rate
#define BIN_WIDTH (SAMPLE_RATE / GOERTZEL_N)  // 1 khz per bin

static float processing_buffer[GOERTZEL_N];  // input signal after widnwoing
static float coeff[GOERTZEL_N / 2 + 1];      // coefficients for (0 -> N-1) / 2
                                             // (symmetry for real values)
static float window[GOERTZEL_N];             // hamming coefficient storage

void get_coeff();
void get_window();
void process_goertzel(float sample_rate, float* freq);
void generate_sine(float freq, float* buffer);
void generate_square(float freq, float* buffer);
void generate_sawtooth(float freq, float* buffer);
void generate_triangle(float freq, float* buffer);
int run_test(float target_freq, const char* wave_type);

int main() {
  // do hamming window /cosine coefficients early
  get_window();
  get_coeff();

  int errors = 0;
  // Original tests
  errors += run_test(10000.0f, "sine");
  errors += run_test(50000.0f, "square");
  errors += run_test(100000.0f, "sawtooth");
  errors += run_test(75000.0f, "triangle");
  errors += run_test(30000.0f, "halfwave");  // 30 kHz half-rectified

  printf("\nTests completed\n");
  return;
}

/* GET THE CONSTANT COEFFICIENTS FOR ALGORITHM coefficient[k] = 2cos(2pik/N)
only do from 0 -> N/2 because dft is symmetric for real valued inputs */
void get_coeff() {
  for (int k = 0; k <= GOERTZEL_N / 2; k++) {
    coeff[k] = 2.0f * cosf(2.0f * M_PI * (float)k / (float)GOERTZEL_N);
  }
}

/* GET THE HAMMING WINDOW COEFFICIENTS FOR A BLOCK
window[n] = 0.54 - 0.46cos(2pin / (N-1))
goal is to basically taper the bin rather than sharp output so no spectral
leaking
 */
void get_window() {
  for (int n = 0; n < GOERTZEL_N; n++) {
    window[n] = 0.54f - 0.46f * cosf(2.0f * M_PI * n / (GOERTZEL_N - 1));
  }
}

void process_goertzel(float sample_rate, float* frequency) {
  float max_magnitude_sq = 0.0f;
  int max_k = 0;
  /* loop over each freq bin frm n=0 to N-1 */
  for (int k = 1; k <= GOERTZEL_N / 2; k++) {
    float q0, q1 = 0.0f, q2 = 0.0f;
    float coeff_k = coeff[k];

    /* finds RECURRENCE q(n) = processing_buffer[n] + coeff[k] * q(n-1) - q(n-2)
    q1 / q2 = n-1 and n-2 btw */
    for (int n = 0; n < GOERTZEL_N; n++) {
      q0 = coeff_k * q1 - q2 + processing_buffer[n];

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

// WAVE GENERATIONS WERE DONE WITH CHAT
void generate_sine(float frequency, float* buffer) {
  for (int i = 0; i < GOERTZEL_N; i++) {
    float t = (float)i / SAMPLE_RATE;
    buffer[i] = sinf(2.0f * M_PI * frequency * t);
  }
}

void generate_square(float frequency, float* buffer) {
  for (int i = 0; i < GOERTZEL_N; i++) {
    float t = (float)i / SAMPLE_RATE;
    float cycle_pos = fmodf(frequency * t, 1.0f);
    buffer[i] = (cycle_pos < 0.5f) ? 1.0f : -1.0f;
  }
}

void generate_sawtooth(float frequency, float* buffer) {
  for (int i = 0; i < GOERTZEL_N; i++) {
    float t = (float)i / SAMPLE_RATE;
    float cycle_pos = fmodf(frequency * t, 1.0f);
    buffer[i] = 2.0f * cycle_pos - 1.0f;
  }
}

void generate_triangle(float frequency, float* buffer) {
  for (int i = 0; i < GOERTZEL_N; i++) {
    float t = (float)i / SAMPLE_RATE;
    float cycle_pos = fmodf(frequency * t, 1.0f);

    if (cycle_pos < 0.5f) {
      buffer[i] = 4.0f * cycle_pos - 1.0f;
    } else {
      buffer[i] = 3.0f - 4.0f * cycle_pos;
    }
  }
}

void generate_halfwave(float frequency, float* buffer) {
  for (int i = 0; i < GOERTZEL_N; i++) {
    float t = (float)i / SAMPLE_RATE;
    float val = sinf(2.0f * M_PI * frequency * t);
    buffer[i] = (val > 0) ? val : 0.0f;
  }
}

int run_test(float target_freq, const char* wave_type) {
  float buffer[GOERTZEL_N];

  if (strcmp(wave_type, "sine") == 0)
    generate_sine(target_freq, buffer);
  else if (strcmp(wave_type, "square") == 0)
    generate_square(target_freq, buffer);
  else if (strcmp(wave_type, "sawtooth") == 0)
    generate_sawtooth(target_freq, buffer);
  else if (strcmp(wave_type, "triangle") == 0)
    generate_triangle(target_freq, buffer);
  else if (strcmp(wave_type, "halfwave") == 0)
    generate_halfwave(target_freq, buffer);
  else {
    fprintf(stderr, "Unknown waveform type: %s\n", wave_type);
    return 1;
  }

  // make window to copy to processing buffer
  for (int i = 0; i < GOERTZEL_N; i++) {
    processing_buffer[i] = buffer[i] * window[i];
  }

  float frequency;
  process_goertzel(SAMPLE_RATE, &frequency);

  printf("want %-8s @ %-7.0f Hz: got %-7.0f Hz\n", wave_type, target_freq,
         frequency);

  return 0;
}
