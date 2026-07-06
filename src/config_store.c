#include "config_store.h"

#include <stdio.h>

#define POWERGUARD_CONFIG_FILE "powerguard.ini"

ret_t config_store_load(user_config_t* config) {
  FILE* fp = fopen(POWERGUARD_CONFIG_FILE, "r");
  int mode = 0;
  int brightness = 0;

  if (fp == NULL) {
    return RET_FAIL;
  }

  if (fscanf(fp, "max_temperature=%f\n", &config->max_temperature) != 1 ||
      fscanf(fp, "min_voltage=%f\n", &config->min_voltage) != 1 ||
      fscanf(fp, "max_current=%f\n", &config->max_current) != 1 ||
      fscanf(fp, "brightness=%d\n", &brightness) != 1 ||
      fscanf(fp, "mode=%d\n", &mode) != 1) {
    fclose(fp);
    return RET_FAIL;
  }

  fclose(fp);

  if (brightness < 20) {
    brightness = 20;
  } else if (brightness > 100) {
    brightness = 100;
  }

  if (mode < POWER_MODE_STANDBY || mode > POWER_MODE_ECO) {
    mode = POWER_MODE_DISCHARGE;
  }

  config->brightness = (uint8_t)brightness;
  config->mode = (power_mode_t)mode;

  return RET_OK;
}

ret_t config_store_save(const user_config_t* config) {
  FILE* fp = fopen(POWERGUARD_CONFIG_FILE, "w");

  if (fp == NULL) {
    return RET_FAIL;
  }

  fprintf(fp, "max_temperature=%.2f\n", config->max_temperature);
  fprintf(fp, "min_voltage=%.2f\n", config->min_voltage);
  fprintf(fp, "max_current=%.2f\n", config->max_current);
  fprintf(fp, "brightness=%u\n", config->brightness);
  fprintf(fp, "mode=%d\n", (int)config->mode);

  fclose(fp);
  return RET_OK;
}
