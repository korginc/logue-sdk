/*
 *  File: effect.h
 *
 *  NTS-3 kaoss pad kit Auto Tune DSP logic
 *
 */

#ifndef EFFECT_H
#define EFFECT_H

#include "unit_genericfx.h"
#include "note_api.h"
#include <math.h>

// Constants
#define AMDF_BUFFER_SIZE 1024
#define SHIFT_BUFFER_SIZE 2048
#define MIN_LAG 48   // ~1000 Hz
#define MAX_LAG 600  // ~80 Hz
#define AMDF_STRIDE 512
#define CROSSFADE_LEN 256
#define GATE_THRESHOLD 0.001f

class AutoTune {
public:
    int8_t Init(const unit_runtime_desc_t *desc) {
      (void)desc;
      Reset();
      return k_unit_err_none;
    }

    void Teardown() {}

    void Reset() {
      for (int i = 0; i < AMDF_BUFFER_SIZE; ++i) amdf_buf[i] = 0;
      for (int i = 0; i < SHIFT_BUFFER_SIZE; ++i) {
        shift_buf_l[i] = 0;
        shift_buf_r[i] = 0;
      }
      amdf_idx = 0;
      shift_idx = 0;
      amdf_timer = 0;
      
      detected_lag = MAX_LAG;
      target_lag = MAX_LAG;
      current_lag = MAX_LAG;
      
      read_ptr1 = 0;
      read_ptr2 = CROSSFADE_LEN;
      xfade_active = 0;
      xfade_counter = 0;
      
      root = 0;
      scale = CHROMATIC;
      speed = 0;
      mix = 1.0f;
      is_gated = true;
    }

    void Resume() {}
    void Suspend() {}

    void Process(const float *in, float *out, size_t frames) {
      const float *src = in;
      float *dst = out;

      for (size_t f = 0; f < frames; f++) {
        float l = *src++;
        float r = *src++;

        // 1. Update Buffers
        amdf_buf[amdf_idx] = l;
        shift_buf_l[shift_idx] = l;
        shift_buf_r[shift_idx] = r;

        // 2. Pitch Detection (AMDF) - Periodic calculation
        if (++amdf_timer >= AMDF_STRIDE) {
          amdf_timer = 0;
          detectPitch();
        }

        // 3. Pitch Shifting Logic
        float tuned_l, tuned_r;
        if (is_gated) {
          tuned_l = l;
          tuned_r = r;
        } else {
          // Calculate target smoothing
          float alpha = 0.0001f + (1.0f - (speed / 1023.0f)) * 0.1f;
          current_lag = (alpha * target_lag) + ((1.0f - alpha) * current_lag);
          
          float ratio = (float)detected_lag / current_lag;

          // Dual-tap crossfade resampling
          tuned_l = readSample(shift_buf_l, ratio);
          tuned_r = readSample(shift_buf_r, ratio);
        }

        // 4. Mix
        *dst++ = l * (1.0f - mix) + tuned_l * mix;
        *dst++ = r * (1.0f - mix) + tuned_r * mix;

        // Increment indices
        amdf_idx = (amdf_idx + 1) % AMDF_BUFFER_SIZE;
        shift_idx = (shift_idx + 1) % SHIFT_BUFFER_SIZE;
      }
    }

    void setParameter(uint8_t id, int32_t value) {
      switch (id) {
        case 0: // ROOT
          root = (uint8_t)value;
          break;
        case 1: // SCALE
          scale = (ScaleType)value;
          break;
        case 2: // SPEED
          speed = value;
          break;
        case 3: // MIX
          mix = value / 1023.0f;
          break;
        default:
          break;
      }
    }

    int32_t getParameterValue(uint8_t id) {
      switch (id) {
        case 0: return root;
        case 1: return (int32_t)scale;
        case 2: return speed;
        case 3: return (int32_t)(mix * 1023.0f);
        default: return 0;
      }
    }

    const char *getParameterStrValue(uint8_t id, int32_t value) {
      switch (id) {
        case 0: return rootNames[value % 12];
        case 1: return scaleNames[value % SCALE_MAX];
        default: return nullptr;
      }
    }

    void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
      (void)id; (void)phase;
      setParameter(0, (x * 12) >> 10); // X -> ROOT
      setParameter(1, (y * SCALE_MAX) >> 10); // Y -> SCALE
    }

private:
    // Circular Buffers
    float amdf_buf[AMDF_BUFFER_SIZE];
    float shift_buf_l[SHIFT_BUFFER_SIZE];
    float shift_buf_r[SHIFT_BUFFER_SIZE];
    uint32_t amdf_idx;
    uint32_t shift_idx;
    uint32_t amdf_timer;

    // Pitch Detection State
    uint32_t detected_lag;
    float target_lag;
    float current_lag;
    bool is_gated;

    // Pitch Shifting State
    float read_ptr1;
    float read_ptr2;
    int xfade_active; // 0: using ptr1, 1: using ptr2, 2: transitioning
    uint32_t xfade_counter;

    // Parameters
    uint8_t root;
    ScaleType scale;
    int32_t speed;
    float mix;

    void detectPitch() {
      // 1. RMS Gate check
      float sum_sq = 0;
      for (int i = 0; i < AMDF_STRIDE; i++) {
        float s = amdf_buf[(amdf_idx - i + AMDF_BUFFER_SIZE) % AMDF_BUFFER_SIZE];
        sum_sq += s * s;
      }
      float rms = sqrtf(sum_sq / AMDF_STRIDE);
      if (rms < GATE_THRESHOLD) {
        is_gated = true;
        return;
      }
      is_gated = false;

      // 2. AMDF calculation
      float min_diff = 1e10f;
      uint32_t best_lag = MAX_LAG;

      for (uint32_t lag = MIN_LAG; lag <= MAX_LAG; lag++) {
        float diff = 0;
        // Optimization: Use a smaller window for AMDF to save CPU
        for (uint32_t i = 0; i < 256; i++) {
          int idx1 = (amdf_idx - i + AMDF_BUFFER_SIZE) % AMDF_BUFFER_SIZE;
          int idx2 = (amdf_idx - i - lag + AMDF_BUFFER_SIZE) % AMDF_BUFFER_SIZE;
          diff += fabsf(amdf_buf[idx1] - amdf_buf[idx2]);
        }
        if (diff < min_diff) {
          min_diff = diff;
          best_lag = lag;
        }
      }

      detected_lag = best_lag;
      float detected_freq = 48000.0f / (float)detected_lag;
      
      // Calculate target frequency via quantization
      // Note = 12 * log2(f/440) + 69
      float note = 12.0f * log2f(detected_freq / 440.0f) + 69.0f;
      uint8_t quantized = quantizeToScale((uint8_t)(note + 0.5f), scale, root);
      
      float target_freq = 440.0f * powf(2.0f, (float)(quantized - 69) / 12.0f);
      target_lag = 48000.0f / target_freq;
    }

    float readSample(const float *buf, float ratio) {
      // This is a simplified dual-tap crossfader.
      // Every time one tap gets too far from the write pointer, we switch.
      
      // Update read pointer 1
      read_ptr1 += ratio;
      if (read_ptr1 >= SHIFT_BUFFER_SIZE) read_ptr1 -= SHIFT_BUFFER_SIZE;
      
      // For this version, we'll implement simple linear interpolation
      // A more robust version would use cross-fading taps to avoid clicks,
      // but let's see if this fits the CPU first.
      
      uint32_t i1 = (uint32_t)read_ptr1;
      uint32_t i2 = (i1 + 1) % SHIFT_BUFFER_SIZE;
      float frac = read_ptr1 - i1;
      
      // Click prevention: if read_ptr crosses write_idx, jump it.
      // (Simplified logic for now)
      if (abs((int)i1 - (int)shift_idx) < 10) {
          read_ptr1 = (shift_idx + SHIFT_BUFFER_SIZE / 2) % SHIFT_BUFFER_SIZE;
      }

      return buf[i1] * (1.0f - frac) + buf[i2] * frac;
    }
};

#endif // EFFECT_H
