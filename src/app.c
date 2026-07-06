#define APP_NAME "智能储能电源 HMI"
#define LCD_WIDTH 960
#define LCD_HEIGHT 540
#define APP_DEFAULT_FONT "default_full"
#define APP_RES_ROOT "D:\\dev\\awtk\\res"

#include "awtk.h"

#include "alarm_manager.h"
#include "battery_model.h"
#include "config_store.h"
#include "simulator.h"
#include "ui_controller.h"

static powerguard_model_t s_model;
static ui_controller_t* s_ui = NULL;

static ret_t on_tick(const timer_info_t* timer) {
  (void)timer;

  simulator_tick(&s_model);
  alarm_manager_update(&s_model);
  battery_model_push_trend(&s_model);
  ui_controller_refresh(s_ui);

  return RET_REPEAT;
}

ret_t application_init(void) {
  widget_t* win = NULL;

  battery_model_init(&s_model);
  config_store_load(&s_model.config);
  alarm_manager_update(&s_model);

  image_manager_set_max_mem_size_of_cached_images(image_manager(), 8 * 1024 * 1024);

  win = window_create(NULL, 0, 0, LCD_WIDTH, LCD_HEIGHT);
  widget_set_name(win, "powerguard_main");
  widget_set_style_str(win, "bg_color", "#111923");
  widget_set_prop_str(win, WIDGET_PROP_CLICKABLE, "no");

  s_ui = ui_controller_create(win, &s_model);
  ui_controller_refresh(s_ui);
  widget_add_timer(win, on_tick, 500);

  return RET_OK;
}

ret_t application_exit(void) {
  if (s_ui != NULL) {
    ui_controller_destroy(s_ui);
    s_ui = NULL;
  }

  return RET_OK;
}

#include "awtk_main.inc"
