#ifndef POWERGUARD_ALARM_MANAGER_H
#define POWERGUARD_ALARM_MANAGER_H

#include "battery_model.h"

void alarm_manager_update(powerguard_model_t* model);
void alarm_manager_ack_all(powerguard_model_t* model);
void alarm_manager_clear_inactive(powerguard_model_t* model);
uint32_t alarm_manager_active_count(const powerguard_model_t* model);

#endif
