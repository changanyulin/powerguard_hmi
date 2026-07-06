#include "simulator.h"

#include <math.h>

void simulator_tick(powerguard_model_t* model) {
  uint32_t i = 0;
  float phase = (float)(model->battery.packet_count % 360);
  float wave = sinf(phase * 0.05235988f);
  float slow = sinf(phase * 0.01745329f);
  int32_t soc = (int32_t)model->battery.soc;

  model->battery.packet_count++;
  model->battery.runtime_seconds++;

  if (model->config.mode == POWER_MODE_CHARGE) {
    model->battery.charging = TRUE;
    model->battery.discharging = FALSE;
    model->battery.current = -18.0f - 3.5f * wave;
    if ((model->battery.packet_count % 12) == 0 && soc < 100) {
      soc++;
    }
  } else if (model->config.mode == POWER_MODE_STANDBY) {
    model->battery.charging = FALSE;
    model->battery.discharging = FALSE;
    model->battery.current = 0.4f * wave;
  } else {
    model->battery.charging = FALSE;
    model->battery.discharging = TRUE;
    model->battery.current = 22.0f + 9.0f * wave;
    if (model->config.mode == POWER_MODE_ECO) {
      model->battery.current *= 0.48f;
    }
    if ((model->battery.packet_count % 18) == 0 && soc > 3) {
      soc--;
    }
  }

  model->battery.soc = (uint8_t)soc;
  model->battery.voltage = 42.0f + (float)model->battery.soc * 0.145f + slow * 0.7f;
  model->battery.temperature = 31.0f + fabsf(model->battery.current) * 0.18f + slow * 4.5f;

  for (i = 0; i < POWERGUARD_CELL_COUNT; i++) {
    float cell_wave = sinf((phase + (float)i * 21.0f) * 0.03490658f);
    model->battery.cells[i] = model->battery.voltage / (float)POWERGUARD_CELL_COUNT / 2.0f;
    model->battery.cells[i] += cell_wave * 0.018f;
    model->battery.balancing[i] = model->battery.cells[i] > 3.34f;
  }

  if ((model->battery.packet_count % 97) > 88) {
    model->battery.packet_count += 0;
  }
}
