/* THIS FILE IS USED FOR TESTING MEASUREMENT VALUES. THIS CAN BE RUN
INDEPENDENTLY ACCURATE FOR MEASUREMENTS.C FILE UPDATED AS OF MARCH 23, 10PM

all measurements work EXCEPT RISE / FALL TIME 
also im lowks concerned about working with unknown periods and ciruclar buffer ending in the middle of a period need to do something about this */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADC_BUFFER_SIZE 1024
float adc_samples[ADC_BUFFER_SIZE];
int adc_sample_count = 0;
#define SAMPLING_RATE 500000.0f

// WAVE TYPES
typedef enum { WAVE_SINE, WAVE_SQUARE } WaveType;

float calc_dc_offset(const float *samples, int count);
float find_rising_crossing(const float *samples, int count, float threshold,
                           int start);
float find_falling_crossing(const float *samples, int count, float threshold,
                            int start);
void find_min_max(const float *samples, int count, float *min_val,
                  float *max_val);
float calc_vpp(const float *samples, int count);
float calc_rms(const float *samples, int count, float dc_offset);
float calc_frequency(int period_samples);
void calc_rise_fall_time(const float *samples, int count, float *rise_time,
                         float *fall_time);
void buffer_to_measurements(void);
void process_adc_data_simulation(WaveType wave_type);
void print_active_measurements(void);

// measurement states
typedef struct {
  int is_active;
  float current_value;
  int period_count;
  float sum;
} MeasurementState;

MeasurementState dc_offset_state = {0};
MeasurementState vpp_state = {0};
MeasurementState amplitude_state = {0};
MeasurementState rms_state = {0};
MeasurementState frequency_state = {0};
MeasurementState rise_time_state = {0};
MeasurementState fall_time_state = {0};

// DC OFFSET
float calc_dc_offset(const float *samples, int count) {
  if (count <= 0) return 0.0f;
  double sum = 0.0;
  for (int i = 0; i < count; i++) {
    sum += samples[i];
  }
  return (float)(sum / count);
}

// FIND RISING CROSSING
float find_rising_crossing(const float *samples, int count, float threshold,
                           int start) {
  for (int i = start; i < count - 1; i++) {
    if (samples[i] < threshold && samples[i + 1] >= threshold) {
      float delta = samples[i + 1] - samples[i];
      if (delta == 0) return i;  // no div by 0
      float fraction = (threshold - samples[i]) / delta;
      return i + fraction;
    }
  }
  return -1.0f;
}

// FIND FALLING CROSSING
float find_falling_crossing(const float *samples, int count, float threshold,
                            int start) {
  for (int i = start; i < count - 1; i++) {
    if (samples[i] >= threshold && samples[i + 1] < threshold) {
      float delta = samples[i] - samples[i + 1];
      if (delta == 0) return i;
      float fraction = (samples[i] - threshold) / delta;
      return i + fraction;
    }
  }
  return -1.0f;
}

// MIN / MAX VALUES
void find_min_max(const float *samples, int count, float *min_val,
                  float *max_val) {
  if (count <= 0) {
    *min_val = 0.0f;
    *max_val = 0.0f;
    return;
  }

  *min_val = samples[0];
  *max_val = samples[0];

  for (int i = 1; i < count; i++) {
    if (samples[i] > *max_val) *max_val = samples[i];
    if (samples[i] < *min_val) *min_val = samples[i];
  }
}

// PEAK TO PEAK VOLTAGE
float calc_vpp(const float *samples, int count) {
  if (count <= 0) return 0.0f;

  float min_val, max_val;
  find_min_max(samples, count, &min_val, &max_val);

  return max_val - min_val;
}

// RMS VOLTAGE
float calc_rms(const float *samples, int count, float dc_offset) {
  if (count <= 0) return 0.0f;
  double sum_sq = 0.0;
  for (int i = 0; i < count; i++) {
    float ac = samples[i] - dc_offset;
    sum_sq += ac * ac;
  }
  return (float)sqrt(sum_sq / count);
}

// FREQUENCY
float calc_frequency(int period_samples) {
  if (period_samples <= 0) return 0.0f;
  float period_time = period_samples / SAMPLING_RATE;  // convert to sec
  if (period_time > 0) {
    return 1.0f / period_time;
  } else {
    return 0.0f;
  }
}

// RISE AND FALL TIME
void calc_rise_fall_time(const float *samples, int count, float *rise_time,
                         float *fall_time) {
  if (count <= 0) {
    *rise_time = 0.0f;
    *fall_time = 0.0f;
    return;
  }

  float min_val, max_val;
  find_min_max(samples, count, &min_val, &max_val);

  float amplitude = max_val - min_val;
  float threshold_low = min_val + 0.1f * amplitude;
  float threshold_high = min_val + 0.9f * amplitude;

  // Rise time calculation
  float rise_start = find_rising_crossing(samples, count, threshold_low, 0);
  float rise_end = -1.0f;
  if (rise_start != -1) {
    rise_end =
        find_rising_crossing(samples, count, threshold_high, (int)rise_start);
  }
  if (rise_start != -1 && rise_end != -1) {
    *rise_time = (rise_end - rise_start) / SAMPLING_RATE;
  } else {
    *rise_time = 0.0f;
  }

  // Fall time calculation
  float fall_start = find_falling_crossing(samples, count, threshold_high, 0);
  float fall_end = -1.0f;
  if (fall_start != -1) {
    fall_end =
        find_falling_crossing(samples, count, threshold_low, (int)fall_start);
  }
  if (fall_start != -1 && fall_end != -1 && fall_end > fall_start) {
    *fall_time = (fall_end - fall_start) / SAMPLING_RATE;
  } else {
    *fall_time = 0.0f;
  }
}

// RESET MEASUREMENT
void reset_measurement(MeasurementState *state) {
  state->is_active = 0;
  state->current_value = 0.0f;
  state->period_count = 0;
  state->sum = 0.0f;
}

// ACTIVATE MEASUREMENT
void activate_measurement(MeasurementState *state) {
  reset_measurement(state);
  state->is_active = 1;
}

// DEACTIVATE MEASUREMENT
void deactivate_measurement(MeasurementState *state) { state->is_active = 0; }

// PROCESS NEW PERIOD DATA
void process_period_data(const float *period_data, int period_length) {
  if (period_length <= 0) return;

  // UPDATE DC OFFSET
  if (dc_offset_state.is_active) {
    float dc = calc_dc_offset(period_data, period_length);
    dc_offset_state.sum += dc;
    dc_offset_state.period_count++;
    dc_offset_state.current_value =
        dc_offset_state.sum / dc_offset_state.period_count;
  }

  // UPDATE VPP
  if (vpp_state.is_active) {
    float vpp = calc_vpp(period_data, period_length);
    vpp_state.sum += vpp;
    vpp_state.period_count++;
    vpp_state.current_value = vpp_state.sum / vpp_state.period_count;
  }

  // UPDATE AMPLITUDE
  if (amplitude_state.is_active) {
    float vpp = calc_vpp(period_data, period_length);
    float amplitude = vpp / 2.0f;
    amplitude_state.sum += amplitude;
    amplitude_state.period_count++;
    amplitude_state.current_value =
        amplitude_state.sum / amplitude_state.period_count;
  }

  // UPDATE VRMS
  if (rms_state.is_active) {
    float dc = calc_dc_offset(period_data, period_length);
    float rms = calc_rms(period_data, period_length, dc);
    rms_state.sum += rms;
    rms_state.period_count++;
    rms_state.current_value = rms_state.sum / rms_state.period_count;
  }

  // UPDATE FREQUENCY
  if (frequency_state.is_active) {
    float frequency = calc_frequency(period_length);
    frequency_state.sum += frequency;
    frequency_state.period_count++;
    frequency_state.current_value =
        frequency_state.sum / frequency_state.period_count;
  }

  // UPDATE RISE / FALL TIME
  if (rise_time_state.is_active || fall_time_state.is_active) {
    float rise_time, fall_time;
    calc_rise_fall_time(period_data, period_length, &rise_time, &fall_time);

    if (rise_time_state.is_active) {
      rise_time_state.sum += rise_time;
      rise_time_state.period_count++;
      rise_time_state.current_value =
          rise_time_state.sum / rise_time_state.period_count;
    }

    if (fall_time_state.is_active) {
      fall_time_state.sum += fall_time;
      fall_time_state.period_count++;
      fall_time_state.current_value =
          fall_time_state.sum / fall_time_state.period_count;
    }
  }
}

// MAIN FUNCTION TO PROCESS BUFFER AND EXTRACT PERIODS
void buffer_to_measurements(void) {
  if (adc_sample_count <= 0) return;

  // MAKE SURE AT LEAST SOMETHING IS ACTIVE
  if (!dc_offset_state.is_active && !vpp_state.is_active &&
      !amplitude_state.is_active && !rms_state.is_active &&
      !frequency_state.is_active && !rise_time_state.is_active &&
      !fall_time_state.is_active) {
    return;
  }

  float dc_offset = calc_dc_offset(adc_samples, adc_sample_count);
  int index = 0;

  while (index < adc_sample_count - 1) {
    // find rising crossing for start of period
    int start =
        find_rising_crossing(adc_samples, adc_sample_count, dc_offset, index);
    if (start == -1) break;

    // find next rising crossing for end of period
    int end = find_rising_crossing(adc_samples, adc_sample_count, dc_offset,
                                   start + 1);
    if (end == -1) break;

    int period_length = end - start;
    if (period_length <= 0) break;

    const float *period_data = &adc_samples[start];

    // UPDATE MEASUREMENTS
    process_period_data(period_data, period_length);

    index = end;  // update end for next period
  }
}

// IMPROVED SIMULATION FUNCTION WITH WAVE TYPE SELECTION
void process_adc_data_simulation(WaveType wave_type) {
  // 50 samples per call
  for (int i = 0; i < 50; i++) {
    float sample = 0.0f;

    switch (wave_type) {
      case WAVE_SINE:
        // SINE WAVE - 0.5v dc offset, 1v amplitude, period of 100 samples
        {
          int sine_position = (adc_sample_count + i) % 100;
          sample = 0.5f + 1.0f * sin(2 * M_PI * sine_position / 100.0f);
        }
        break;

      case WAVE_SQUARE:
        // SQUARE WAVE - 1.2v dc offset, 1.5v amplitude, period of 80 samples
        {
          int sq_position = (adc_sample_count + i) % 80;
          if (sq_position < 40) {
            sample = 1.2f + 1.5f;  // high level = 2.7v
          } else {
            sample = 1.2f - 1.5f;  // low level = -0.3v
          }
        }
        break;
    }

    adc_samples[(adc_sample_count + i) % ADC_BUFFER_SIZE] = sample;
  }

  adc_sample_count += 50;
  if (adc_sample_count > ADC_BUFFER_SIZE) {
    adc_sample_count = ADC_BUFFER_SIZE;
  }

  buffer_to_measurements();
}

// FUNCTIONS TO START / STOP MEASUREMENTS FROM USER
void start_dc_offset_measurement(void) {
  activate_measurement(&dc_offset_state);
  printf("DC Offset measurement started.\n");
}

void stop_dc_offset_measurement(void) {
  deactivate_measurement(&dc_offset_state);
  printf("DC Offset measurement stopped. Final value: %f V\n",
         dc_offset_state.current_value);
}

void start_vpp_measurement(void) {
  activate_measurement(&vpp_state);
  printf("VPP measurement started.\n");
}

void stop_vpp_measurement(void) {
  deactivate_measurement(&vpp_state);
  printf("VPP measurement stopped. Final value: %f V\n",
         vpp_state.current_value);
}

void start_amplitude_measurement(void) {
  activate_measurement(&amplitude_state);
  printf("Amplitude measurement started.\n");
}

void stop_amplitude_measurement(void) {
  deactivate_measurement(&amplitude_state);
  printf("Amplitude measurement stopped. Final value: %f V\n",
         amplitude_state.current_value);
}

void start_rms_measurement(void) {
  activate_measurement(&rms_state);
  printf("RMS measurement started.\n");
}

void stop_rms_measurement(void) {
  deactivate_measurement(&rms_state);
  printf("RMS measurement stopped. Final value: %f V\n",
         rms_state.current_value);
}

void start_frequency_measurement(void) {
  activate_measurement(&frequency_state);
  printf("Frequency measurement started.\n");
}

void stop_frequency_measurement(void) {
  deactivate_measurement(&frequency_state);
  printf("Frequency measurement stopped. Final value: %f Hz\n",
         frequency_state.current_value);
}

void start_rise_time_measurement(void) {
  activate_measurement(&rise_time_state);
  printf("Rise Time measurement started.\n");
}

void stop_rise_time_measurement(void) {
  deactivate_measurement(&rise_time_state);
  printf("Rise Time measurement stopped. Final value: %f s\n",
         rise_time_state.current_value);
}

void start_fall_time_measurement(void) {
  activate_measurement(&fall_time_state);
  printf("Fall Time measurement started.\n");
}

void stop_fall_time_measurement(void) {
  deactivate_measurement(&fall_time_state);
  printf("Fall Time measurement stopped. Final value: %f s\n",
         fall_time_state.current_value);
}

// GET CURRENT VALUES
float get_current_dc_offset(void) { return dc_offset_state.current_value; }
float get_current_vpp(void) { return vpp_state.current_value; }
float get_current_amplitude(void) { return amplitude_state.current_value; }
float get_current_rms(void) { return rms_state.current_value; }
float get_current_frequency(void) { return frequency_state.current_value; }
float get_current_rise_time(void) { return rise_time_state.current_value; }
float get_current_fall_time(void) { return fall_time_state.current_value; }

// PRINT ALL ACTIVE MEASUREMENTS FOR DEBUGGING
void print_active_measurements(void) {
  if (dc_offset_state.is_active) {
    printf("DC Offset: %f V (averaged over %d periods)\n",
           dc_offset_state.current_value, dc_offset_state.period_count);
  }

  if (vpp_state.is_active) {
    printf("Peak-to-Peak Voltage: %f V (averaged over %d periods)\n",
           vpp_state.current_value, vpp_state.period_count);
  }

  if (amplitude_state.is_active) {
    printf("Amplitude: %f V (averaged over %d periods)\n",
           amplitude_state.current_value, amplitude_state.period_count);
  }

  if (rms_state.is_active) {
    printf("RMS Voltage: %f V (averaged over %d periods)\n",
           rms_state.current_value, rms_state.period_count);
  }

  if (frequency_state.is_active) {
    printf("Frequency: %f Hz (averaged over %d periods)\n",
           frequency_state.current_value, frequency_state.period_count);
  }

  if (rise_time_state.is_active) {
    printf("Rise Time: %f s (averaged over %d periods)\n",
           rise_time_state.current_value, rise_time_state.period_count);
  }

  if (fall_time_state.is_active) {
    printf("Fall Time: %f s (averaged over %d periods)\n",
           fall_time_state.current_value, fall_time_state.period_count);
  }
}

// MAIN TEST FUNCTION
int main(void) {
  printf("Measurement Testing Program\n");
  printf("---------------------------\n\n");

  // TEST 1: SINE WAVE
  printf(
      "TEST 1: SINE WAVE (0.5V DC offset, 1V amplitude, period of 100 "
      "samples)\n");
  printf("Expected measurements:\n");
  printf("  DC Offset: 0.5V\n");
  printf("  Peak-to-Peak: 2.0V\n");
  printf("  Amplitude: 1.0V\n");
  printf("  RMS: ~0.707V\n");
  printf("  Frequency: 5000Hz (500kHz/100)\n\n");

  // start all measurements
  start_dc_offset_measurement();
  start_vpp_measurement();
  start_amplitude_measurement();
  start_rms_measurement();
  start_frequency_measurement();
  start_rise_time_measurement();
  start_fall_time_measurement();

  // run simulation
  printf("Running sine wave simulation...\n");
  for (int i = 0; i < 20; i++) {
    process_adc_data_simulation(WAVE_SINE);
  }

  printf("\nFinal measurements for sine wave:\n");
  print_active_measurements();

  // reset all measurements
  reset_measurement(&dc_offset_state);
  reset_measurement(&vpp_state);
  reset_measurement(&amplitude_state);
  reset_measurement(&rms_state);
  reset_measurement(&frequency_state);
  reset_measurement(&rise_time_state);
  reset_measurement(&fall_time_state);

  // clear sample buffer
  adc_sample_count = 0;
  memset(adc_samples, 0, sizeof(adc_samples));

  // TEST 2: SQUARE WAVE
  printf(
      "\n\nTEST 2: SQUARE WAVE (1.2V DC offset, 1.5V amplitude, period of 80 "
      "samples)\n");
  printf("Expected measurements:\n");
  printf("  DC Offset: 1.2V\n");
  printf("  Peak-to-Peak: 3.0V\n");
  printf("  Amplitude: 1.5V\n");
  printf("  Frequency: 6250Hz (500kHz/80)\n\n");

  // start all measurements again
  start_dc_offset_measurement();
  start_vpp_measurement();
  start_amplitude_measurement();
  start_rms_measurement();
  start_frequency_measurement();
  start_rise_time_measurement();
  start_fall_time_measurement();

  // run
  printf("Running square wave simulation...\n");
  for (int i = 0; i < 16; i++) {
    process_adc_data_simulation(WAVE_SQUARE);
  }

  printf("\nFinal measurements for square wave:\n");
  print_active_measurements();

  printf("\nTest completed.\n");

  return 0;
}
