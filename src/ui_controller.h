#ifndef POWERGUARD_UI_CONTROLLER_H
#define POWERGUARD_UI_CONTROLLER_H

#include "awtk.h"
#include "battery_model.h"

typedef struct _ui_controller_t ui_controller_t;

ui_controller_t* ui_controller_create(widget_t* win, powerguard_model_t* model);
void ui_controller_refresh(ui_controller_t* ui);
void ui_controller_destroy(ui_controller_t* ui);

#endif
