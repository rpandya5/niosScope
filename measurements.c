#include <stdbool.h>

// Function prototypes
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
  dc_offset = maximum_value - amplitude;
  return;


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

  for (int i = 1; i < ADC_BUFFER_SIZE; i++) {
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
