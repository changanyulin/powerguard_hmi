#include "alarm_manager.h"

#include <math.h>

static alarm_record_t* alarm_manager_find(powerguard_model_t* model, alarm_type_t type) {
  uint32_t i = 0;

  for (i = 0; i < model->alarm_count; i++) {
    if (model->alarms[i].type == type) {
      return model->alarms + i;
    }
  }

  if (model->alarm_count < POWERGUARD_ALARM_HISTORY_MAX) {
    alarm_record_t* record = model->alarms + model->alarm_count++;
    record->type = type;
    record->active = FALSE;
    record->acknowledged = FALSE;
    record->first_seen_seconds = model->battery.runtime_seconds;
    record->last_seen_seconds = model->battery.runtime_seconds;
    return record;
  }

  return model->alarms;
}

static void alarm_manager_set(powerguard_model_t* model, alarm_type_t type, bool_t active) {
  alarm_record_t* record = alarm_manager_find(model, type);

  if (record == NULL) {
    return;
  }

  if (active && !record->active) {
    record->acknowledged = FALSE;
    record->first_seen_seconds = model->battery.runtime_seconds;
  }

  record->active = active;
  if (active) {
    record->last_seen_seconds = model->battery.runtime_seconds;
  }
}

void alarm_manager_update(powerguard_model_t* model) {
  bool_t comm_lost = (model->battery.packet_count % 97) > 88;

  alarm_manager_set(model, ALARM_OVER_TEMP,
                    model->battery.temperature > model->config.max_temperature);
  alarm_manager_set(model, ALARM_UNDER_VOLTAGE,
                    model->battery.voltage < model->config.min_voltage);
  alarm_manager_set(model, ALARM_OVER_CURRENT,
                    fabs(model->battery.current) > model->config.max_current);
  alarm_manager_set(model, ALARM_COMM_LOST, comm_lost);

  model->battery.fault = alarm_manager_active_count(model) > 0;
}

void alarm_manager_ack_all(powerguard_model_t* model) {
  uint32_t i = 0;

  for (i = 0; i < model->alarm_count; i++) {
    if (model->alarms[i].active) {
      model->alarms[i].acknowledged = TRUE;
    }
  }
}

void alarm_manager_clear_inactive(powerguard_model_t* model) {
  uint32_t read = 0;
  uint32_t write = 0;

  for (read = 0; read < model->alarm_count; read++) {
    if (model->alarms[read].active) {
      if (read != write) {
        model->alarms[write] = model->alarms[read];
      }
      write++;
    }
  }

  model->alarm_count = write;
}

uint32_t alarm_manager_active_count(const powerguard_model_t* model) {
  uint32_t i = 0;
  uint32_t count = 0;

  for (i = 0; i < model->alarm_count; i++) {
    if (model->alarms[i].active) {
      count++;
    }
  }

  return count;
}
