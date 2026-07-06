#ifndef POWERGUARD_CONFIG_STORE_H
#define POWERGUARD_CONFIG_STORE_H

#include "battery_model.h"

ret_t config_store_load(user_config_t* config);
ret_t config_store_save(const user_config_t* config);

#endif
