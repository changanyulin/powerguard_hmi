#include "battery_model.h"

#include "tkc/mem.h"

void battery_model_init(powerguard_model_t* model) {
  uint32_t i = 0;

  memset(model, 0x00, sizeof(powerguard_model_t));

  model->battery.voltage = 51.2f;
  model->battery.current = 0.0f;
  model->battery.temperature = 29.5f;
  model->battery.soc = 76;
  model->battery.runtime_seconds = 2 * 3600 + 18 * 60;

  for (i = 0; i < POWERGUARD_CELL_COUNT; i++) {
    model->battery.cells[i] = 3.205f + (float)(i % 3) * 0.006f;
    model->battery.balancing[i] = FALSE;
  }

  model->config.max_temperature = 58.0f;
  model->config.min_voltage = 44.0f;
  model->config.max_current = 65.0f;
  model->config.brightness = 82;
  model->config.mode = POWER_MODE_DISCHARGE;

  for (i = 0; i < POWERGUARD_TREND_POINTS; i++) {
    model->soc_trend[i] = (float)model->battery.soc;
    model->temp_trend[i] = model->battery.temperature;
  }
}

void battery_model_push_trend(powerguard_model_t* model) {
  model->soc_trend[model->trend_pos] = (float)model->battery.soc;
  model->temp_trend[model->trend_pos] = model->battery.temperature;
  model->trend_pos = (model->trend_pos + 1) % POWERGUARD_TREND_POINTS;
}

const char* power_mode_to_string(power_mode_t mode) {
  switch (mode) {
    case POWER_MODE_STANDBY:
      return "待机";
    case POWER_MODE_CHARGE:
      return "充电";
    case POWER_MODE_DISCHARGE:
      return "放电";
    case POWER_MODE_ECO:
      return "节能";
    default:
      return "未知";
  }
}

const char* alarm_type_to_string(alarm_type_t type) {
  switch (type) {
    case ALARM_OVER_TEMP:
      return "电池包过温";
    case ALARM_UNDER_VOLTAGE:
      return "电池包欠压";
    case ALARM_OVER_CURRENT:
      return "输出过流";
    case ALARM_COMM_LOST:
      return "MCU通信中断";
    default:
      return "无告警";
  }
}

const char* alarm_type_to_code(alarm_type_t type) {
  switch (type) {
    case ALARM_OVER_TEMP:
      return "A01";
    case ALARM_UNDER_VOLTAGE:
      return "A02";
    case ALARM_OVER_CURRENT:
      return "A03";
    case ALARM_COMM_LOST:
      return "A04";
    default:
      return "--";
  }
}

float battery_model_min_cell(const powerguard_model_t* model, uint32_t* index) {
  uint32_t i = 0;
  float value = model->battery.cells[0];

  *index = 0;
  for (i = 1; i < POWERGUARD_CELL_COUNT; i++) {
    if (model->battery.cells[i] < value) {
      value = model->battery.cells[i];
      *index = i;
    }
  }

  return value;
}

float battery_model_max_cell(const powerguard_model_t* model, uint32_t* index) {
  uint32_t i = 0;
  float value = model->battery.cells[0];

  *index = 0;
  for (i = 1; i < POWERGUARD_CELL_COUNT; i++) {
    if (model->battery.cells[i] > value) {
      value = model->battery.cells[i];
      *index = i;
    }
  }

  return value;
}
