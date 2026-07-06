#ifndef POWERGUARD_BATTERY_MODEL_H
#define POWERGUARD_BATTERY_MODEL_H

#include "tkc/types_def.h"

#define POWERGUARD_CELL_COUNT 8
#define POWERGUARD_ALARM_HISTORY_MAX 16
#define POWERGUARD_TREND_POINTS 96

typedef enum _power_mode_t {
  POWER_MODE_STANDBY = 0,
  POWER_MODE_CHARGE,
  POWER_MODE_DISCHARGE,
  POWER_MODE_ECO
} power_mode_t;

typedef struct _battery_status_t {
  float voltage;
  float current;
  float temperature;
  uint8_t soc;
  bool_t charging;
  bool_t discharging;
  bool_t fault;
  uint32_t runtime_seconds;
  float cells[POWERGUARD_CELL_COUNT];
  bool_t balancing[POWERGUARD_CELL_COUNT];
  uint32_t packet_count;
} battery_status_t;

typedef enum _alarm_type_t {
  ALARM_NONE = 0,
  ALARM_OVER_TEMP,
  ALARM_UNDER_VOLTAGE,
  ALARM_OVER_CURRENT,
  ALARM_COMM_LOST
} alarm_type_t;

typedef struct _alarm_record_t {
  alarm_type_t type;
  bool_t active;
  bool_t acknowledged;
  uint32_t first_seen_seconds;
  uint32_t last_seen_seconds;
} alarm_record_t;

typedef struct _user_config_t {
  float max_temperature;
  float min_voltage;
  float max_current;
  uint8_t brightness;
  power_mode_t mode;
} user_config_t;

typedef struct _powerguard_model_t {
  battery_status_t battery;
  user_config_t config;
  alarm_record_t alarms[POWERGUARD_ALARM_HISTORY_MAX];
  uint32_t alarm_count;
  float soc_trend[POWERGUARD_TREND_POINTS];
  float temp_trend[POWERGUARD_TREND_POINTS];
  uint32_t trend_pos;
} powerguard_model_t;

void battery_model_init(powerguard_model_t* model);
void battery_model_push_trend(powerguard_model_t* model);
const char* power_mode_to_string(power_mode_t mode);
const char* alarm_type_to_string(alarm_type_t type);
const char* alarm_type_to_code(alarm_type_t type);
float battery_model_min_cell(const powerguard_model_t* model, uint32_t* index);
float battery_model_max_cell(const powerguard_model_t* model, uint32_t* index);

#endif
