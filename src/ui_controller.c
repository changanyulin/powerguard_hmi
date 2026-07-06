#include "ui_controller.h"

#include "alarm_manager.h"
#include "config_store.h"
#include "ext_widgets/canvas_widget/canvas_widget.h"
#include "ext_widgets/progress_circle/progress_circle.h"
#include "ext_widgets/scroll_view/scroll_view.h"
#include "tkc/mem.h"
#include "tkc/utils.h"

#include <stdarg.h>

typedef enum _page_id_t {
  PAGE_DASHBOARD = 0,
  PAGE_CELLS,
  PAGE_ALARMS,
  PAGE_SETTINGS,
  PAGE_COUNT
} page_id_t;

struct _ui_controller_t {
  widget_t* win;
  powerguard_model_t* model;
  page_id_t page;

  widget_t* pages[PAGE_COUNT];
  widget_t* nav_buttons[PAGE_COUNT];

  widget_t* status_badge;
  widget_t* alarm_badge;
  widget_t* mode_badge;

  widget_t* soc_ring;
  widget_t* soc_label;
  widget_t* voltage_label;
  widget_t* current_label;
  widget_t* temp_label;
  widget_t* runtime_label;
  widget_t* state_label;
  widget_t* trend_canvas;

  widget_t* cell_labels[POWERGUARD_CELL_COUNT];
  widget_t* cell_bars[POWERGUARD_CELL_COUNT];
  widget_t* cell_summary;

  widget_t* alarm_rows[POWERGUARD_ALARM_HISTORY_MAX];
  widget_t* alarm_summary;

  widget_t* edit_max_temp;
  widget_t* edit_min_voltage;
  widget_t* edit_max_current;
  widget_t* brightness_slider;
  widget_t* brightness_label;
  widget_t* mode_label;
  widget_t* save_status;
};

static color_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return color_init(r, g, b, a);
}

static widget_t* label_ex(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h, const char* text,
                          const char* color, int32_t font_size, const char* align) {
  widget_t* label = label_create(parent, x, y, w, h);
  widget_set_text_utf8(label, text);
  widget_set_style_str(label, "normal:text_color", color);
  widget_set_style_int(label, "normal:font_size", font_size);
  widget_set_style_str(label, "normal:text_align_h", align);
  widget_set_style_str(label, "normal:text_align_v", "middle");
  return label;
}

static widget_t* panel(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* view = view_create(parent, x, y, w, h);
  widget_set_style_str(view, "bg_color", "#172332");
  widget_set_style_str(view, "border_color", "#26384d");
  widget_set_style_int(view, "border_width", 1);
  widget_set_style_int(view, "round_radius", 6);
  return view;
}

static widget_t* button_ex(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h, const char* text) {
  widget_t* button = button_create(parent, x, y, w, h);
  widget_set_text_utf8(button, text);
  widget_set_style_str(button, "normal:bg_color", "#1f3146");
  widget_set_style_str(button, "over:bg_color", "#28435f");
  widget_set_style_str(button, "pressed:bg_color", "#2f84ff");
  widget_set_style_str(button, "normal:text_color", "#edf6ff");
  widget_set_style_str(button, "over:text_color", "#ffffff");
  widget_set_style_str(button, "pressed:text_color", "#ffffff");
  widget_set_style_int(button, "normal:font_size", 15);
  widget_set_style_int(button, "round_radius", 5);
  return button;
}

static widget_t* page_scroll_create(widget_t* parent, wh_t virtual_h) {
  widget_t* page = view_create(parent, 150, 62, 810, 478);
  (void)virtual_h;

  widget_set_style_str(page, "bg_color", "#111923");

  return page;
}

static void refresh_settings(ui_controller_t* ui);

static void set_label(widget_t* label, const char* format, ...) {
  char text[128];
  va_list args;

  va_start(args, format);
  tk_vsnprintf(text, sizeof(text), format, args);
  va_end(args);

  widget_set_text_utf8(label, text);
}

static void set_page(ui_controller_t* ui, page_id_t page) {
  uint32_t i = 0;

  ui->page = page;
  for (i = 0; i < PAGE_COUNT; i++) {
    widget_set_visible(ui->pages[i], i == (uint32_t)page);
    widget_set_style_str(ui->nav_buttons[i], "normal:bg_color",
                         i == (uint32_t)page ? "#2f84ff" : "#1f3146");
  }

  if (page == PAGE_SETTINGS) {
    refresh_settings(ui);
  }
}

static ret_t on_nav(void* ctx, event_t* e) {
  ui_controller_t* ui = (ui_controller_t*)ctx;
  page_id_t page = (page_id_t)widget_get_prop_int(WIDGET(e->target), "page", 0);
  set_page(ui, page);
  return RET_OK;
}

static ret_t on_ack_alarms(void* ctx, event_t* e) {
  ui_controller_t* ui = (ui_controller_t*)ctx;
  (void)e;

  alarm_manager_ack_all(ui->model);
  ui_controller_refresh(ui);

  return RET_OK;
}

static ret_t on_clear_alarms(void* ctx, event_t* e) {
  ui_controller_t* ui = (ui_controller_t*)ctx;
  (void)e;

  alarm_manager_clear_inactive(ui->model);
  ui_controller_refresh(ui);

  return RET_OK;
}

static ret_t on_mode(void* ctx, event_t* e) {
  ui_controller_t* ui = (ui_controller_t*)ctx;
  (void)e;

  ui->model->config.mode = (power_mode_t)(((int)ui->model->config.mode + 1) % 4);
  refresh_settings(ui);
  ui_controller_refresh(ui);

  return RET_OK;
}

static float edit_float(widget_t* edit, float defval) {
  char text[64];
  if (widget_get_text_utf8(edit, text, sizeof(text)) == RET_OK && text[0] != '\0') {
    return (float)tk_atof(text);
  }
  return defval;
}

static ret_t on_save_settings(void* ctx, event_t* e) {
  ui_controller_t* ui = (ui_controller_t*)ctx;
  float max_temp = edit_float(ui->edit_max_temp, ui->model->config.max_temperature);
  float min_voltage = edit_float(ui->edit_min_voltage, ui->model->config.min_voltage);
  float max_current = edit_float(ui->edit_max_current, ui->model->config.max_current);
  (void)e;

  ui->model->config.max_temperature = tk_clamp(max_temp, 35.0f, 90.0f);
  ui->model->config.min_voltage = tk_clamp(min_voltage, 36.0f, 54.0f);
  ui->model->config.max_current = tk_clamp(max_current, 5.0f, 120.0f);
  ui->model->config.brightness =
      (uint8_t)tk_clampi((int32_t)widget_get_value(ui->brightness_slider), 20, 100);

  if (config_store_save(&ui->model->config) == RET_OK) {
    widget_set_text_utf8(ui->save_status, "已保存到 powerguard.ini");
  } else {
    widget_set_text_utf8(ui->save_status, "保存失败");
  }

  alarm_manager_update(ui->model);
  refresh_settings(ui);
  ui_controller_refresh(ui);

  return RET_OK;
}

static ret_t on_brightness_changed(void* ctx, event_t* e) {
  ui_controller_t* ui = (ui_controller_t*)ctx;
  (void)e;

  ui->model->config.brightness =
      (uint8_t)tk_clampi((int32_t)widget_get_value(ui->brightness_slider), 20, 100);
  set_label(ui->brightness_label, "%u%%", ui->model->config.brightness);
  ui_controller_refresh(ui);

  return RET_OK;
}

static ret_t on_trend_paint(void* ctx, event_t* e) {
  ui_controller_t* ui = (ui_controller_t*)ctx;
  paint_event_t* evt = paint_event_cast(e);
  canvas_t* c = evt->c;
  vgcanvas_t* vg = canvas_get_vgcanvas(c);
  widget_t* widget = WIDGET(e->target);
  float w = (float)widget->w;
  float h = (float)widget->h;
  float left = 16.0f;
  float top = 14.0f;
  float graph_w = w - 32.0f;
  float graph_h = h - 28.0f;
  uint32_t i = 0;
  point_t p = {0, 0};

  widget_to_global(widget, &p);

  vgcanvas_save(vg);
  vgcanvas_clip_rect(vg, p.x, p.y, w, h);
  vgcanvas_translate(vg, p.x, p.y);

  vgcanvas_set_fill_color(vg, rgba(13, 22, 32, 255));
  vgcanvas_rect(vg, 0, 0, w, h);
  vgcanvas_fill(vg);

  vgcanvas_set_stroke_color(vg, rgba(46, 63, 84, 255));
  vgcanvas_set_line_width(vg, 1);
  for (i = 0; i < 4; i++) {
    float y = top + graph_h * (float)i / 3.0f;
    vgcanvas_begin_path(vg);
    vgcanvas_move_to(vg, left, y);
    vgcanvas_line_to(vg, left + graph_w, y);
    vgcanvas_stroke(vg);
  }

  vgcanvas_set_stroke_color(vg, rgba(47, 132, 255, 255));
  vgcanvas_set_line_width(vg, 2);
  vgcanvas_begin_path(vg);
  for (i = 0; i < POWERGUARD_TREND_POINTS; i++) {
    uint32_t idx = (ui->model->trend_pos + i) % POWERGUARD_TREND_POINTS;
    float x = left + graph_w * (float)i / (float)(POWERGUARD_TREND_POINTS - 1);
    float y = top + graph_h - graph_h * ui->model->soc_trend[idx] / 100.0f;
    if (i == 0) {
      vgcanvas_move_to(vg, x, y);
    } else {
      vgcanvas_line_to(vg, x, y);
    }
  }
  vgcanvas_stroke(vg);

  vgcanvas_set_stroke_color(vg, rgba(255, 178, 78, 255));
  vgcanvas_set_line_width(vg, 2);
  vgcanvas_begin_path(vg);
  for (i = 0; i < POWERGUARD_TREND_POINTS; i++) {
    uint32_t idx = (ui->model->trend_pos + i) % POWERGUARD_TREND_POINTS;
    float temp = tk_clamp(ui->model->temp_trend[idx], 10.0f, 80.0f);
    float x = left + graph_w * (float)i / (float)(POWERGUARD_TREND_POINTS - 1);
    float y = top + graph_h - graph_h * (temp - 10.0f) / 70.0f;
    if (i == 0) {
      vgcanvas_move_to(vg, x, y);
    } else {
      vgcanvas_line_to(vg, x, y);
    }
  }
  vgcanvas_stroke(vg);

  vgcanvas_restore(vg);
  return RET_OK;
}

static void create_header(ui_controller_t* ui) {
  widget_t* header = view_create(ui->win, 0, 0, 960, 62);
  widget_set_style_str(header, "bg_color", "#0d1620");
  label_ex(header, 24, 8, 280, 28, "智能储能电源 HMI", "#edf6ff", 23, "left");
  label_ex(header, 25, 35, 320, 20, "48V 便携储能 / BMS 管理界面", "#8aa4bf", 13, "left");
  ui->status_badge = label_ex(header, 620, 16, 104, 30, "在线", "#bdfbd7", 14, "center");
  ui->alarm_badge = label_ex(header, 734, 16, 104, 30, "告警 0", "#bdfbd7", 14, "center");
  ui->mode_badge = label_ex(header, 848, 16, 88, 30, "模式", "#edf6ff", 14, "center");
}

static void create_nav(ui_controller_t* ui) {
  const char* names[PAGE_COUNT] = {"总览", "电芯", "告警", "设置"};
  uint32_t i = 0;
  widget_t* nav = view_create(ui->win, 0, 62, 150, 478);
  widget_set_style_str(nav, "bg_color", "#111923");

  for (i = 0; i < PAGE_COUNT; i++) {
    ui->nav_buttons[i] = button_ex(nav, 16, 28 + i * 64, 118, 44, names[i]);
    widget_set_prop_int(ui->nav_buttons[i], "page", i);
    widget_on(ui->nav_buttons[i], EVT_CLICK, on_nav, ui);
  }
}

static void create_dashboard(ui_controller_t* ui) {
  widget_t* page = page_scroll_create(ui->win, 500);
  widget_t* main = panel(page, 18, 16, 266, 250);
  widget_t* metrics = panel(page, 306, 16, 486, 250);
  widget_t* card = NULL;

  ui->pages[PAGE_DASHBOARD] = page;

  label_ex(page, 18, 282, 180, 28, "运行趋势", "#edf6ff", 20, "left");
  label_ex(page, 198, 282, 360, 28, "蓝色：SOC / 橙色：温度", "#8aa4bf", 15, "left");
  label_ex(metrics, 20, 14, 240, 28, "实时数据", "#edf6ff", 20, "left");

  ui->soc_ring = progress_circle_create(main, 43, 20, 180, 180);
  progress_circle_set_max(ui->soc_ring, 100);
  progress_circle_set_line_width(ui->soc_ring, 18);
  progress_circle_set_format(ui->soc_ring, "%d%%");
  widget_set_style_str(ui->soc_ring, "fg_color", "#2f84ff");
  widget_set_style_str(ui->soc_ring, "normal:text_color", "#edf6ff");
  widget_set_style_int(ui->soc_ring, "normal:font_size", 28);
  ui->soc_label = label_ex(main, 58, 206, 150, 32, "SOC", "#8aa4bf", 18, "center");

  card = panel(metrics, 20, 54, 208, 68);
  ui->voltage_label = label_ex(card, 16, 6, 176, 56, "总电压", "#edf6ff", 21, "left");
  card = panel(metrics, 250, 54, 208, 68);
  ui->current_label = label_ex(card, 16, 6, 176, 56, "电流", "#edf6ff", 21, "left");
  card = panel(metrics, 20, 136, 208, 68);
  ui->temp_label = label_ex(card, 16, 6, 176, 56, "温度", "#edf6ff", 21, "left");
  card = panel(metrics, 250, 136, 208, 68);
  ui->runtime_label = label_ex(card, 16, 6, 176, 56, "运行时间", "#edf6ff", 21, "left");
  ui->state_label = label_ex(metrics, 20, 210, 438, 28, "状态", "#bdfbd7", 16, "left");

  ui->trend_canvas = canvas_widget_create(page, 18, 318, 774, 132);
  widget_set_style_str(ui->trend_canvas, "bg_color", "#0d1620");
  widget_set_style_str(ui->trend_canvas, "border_color", "#26384d");
  widget_set_style_int(ui->trend_canvas, "border_width", 1);
  widget_on(ui->trend_canvas, EVT_PAINT, on_trend_paint, ui);
}

static void create_cells(ui_controller_t* ui) {
  uint32_t i = 0;
  widget_t* page = page_scroll_create(ui->win, 500);
  ui->pages[PAGE_CELLS] = page;

  label_ex(page, 18, 18, 280, 30, "电芯监控", "#edf6ff", 22, "left");
  ui->cell_summary = label_ex(page, 382, 18, 400, 30, "压差", "#8aa4bf", 15, "right");

  for (i = 0; i < POWERGUARD_CELL_COUNT; i++) {
    xy_t x = 18 + (i % 2) * 390;
    xy_t y = 68 + (i / 2) * 84;
    widget_t* row = panel(page, x, y, 360, 64);
    char name[32];
    tk_snprintf(name, sizeof(name), "电芯%02u", i + 1);
    label_ex(row, 16, 8, 86, 24, name, "#8aa4bf", 15, "left");
    ui->cell_labels[i] = label_ex(row, 102, 8, 92, 24, "-- V", "#edf6ff", 18, "right");
    ui->cell_bars[i] = progress_bar_create(row, 16, 39, 328, 10);
    widget_set_style_str(ui->cell_bars[i], "fg_color", "#2f84ff");
    widget_set_style_str(ui->cell_bars[i], "bg_color", "#0d1620");
    progress_bar_set_max(ui->cell_bars[i], 4200);
  }
}

static void create_alarms(ui_controller_t* ui) {
  uint32_t i = 0;
  widget_t* page = page_scroll_create(ui->win, 520);
  ui->pages[PAGE_ALARMS] = page;

  label_ex(page, 18, 18, 220, 30, "告警中心", "#edf6ff", 22, "left");
  ui->alarm_summary = label_ex(page, 238, 18, 260, 30, "无活动告警", "#bdfbd7", 16, "left");
  widget_on(button_ex(page, 536, 16, 112, 34, "确认告警"), EVT_CLICK, on_ack_alarms, ui);
  widget_on(button_ex(page, 666, 16, 112, 34, "清除历史"), EVT_CLICK, on_clear_alarms, ui);

  for (i = 0; i < POWERGUARD_ALARM_HISTORY_MAX; i++) {
    ui->alarm_rows[i] = label_ex(page, 18, 66 + i * 24, 764, 22, "", "#8aa4bf", 14, "left");
  }
}

static widget_t* edit_ex(widget_t* parent, xy_t x, xy_t y, const char* text) {
  widget_t* edit = edit_create(parent, x, y, 128, 34);
  widget_set_text_utf8(edit, text);
  widget_set_style_str(edit, "normal:bg_color", "#0d1620");
  widget_set_style_str(edit, "focused:bg_color", "#122238");
  widget_set_style_str(edit, "normal:text_color", "#edf6ff");
  widget_set_style_str(edit, "focused:text_color", "#ffffff");
  widget_set_style_str(edit, "normal:border_color", "#2f84ff");
  widget_set_style_int(edit, "normal:font_size", 16);
  return edit;
}

static void create_settings(ui_controller_t* ui) {
  widget_t* page = page_scroll_create(ui->win, 500);
  widget_t* box = panel(page, 18, 62, 764, 288);
  ui->pages[PAGE_SETTINGS] = page;

  label_ex(page, 18, 18, 260, 30, "系统设置", "#edf6ff", 22, "left");
  label_ex(box, 28, 28, 210, 28, "最高温度(℃)", "#8aa4bf", 16, "left");
  label_ex(box, 28, 80, 210, 28, "最低总压(V)", "#8aa4bf", 16, "left");
  label_ex(box, 28, 132, 210, 28, "最大电流(A)", "#8aa4bf", 16, "left");
  label_ex(box, 28, 184, 210, 28, "屏幕亮度", "#8aa4bf", 16, "left");
  label_ex(box, 430, 28, 140, 28, "工作模式", "#8aa4bf", 16, "left");

  ui->edit_max_temp = edit_ex(box, 244, 24, "");
  ui->edit_min_voltage = edit_ex(box, 244, 76, "");
  ui->edit_max_current = edit_ex(box, 244, 128, "");

  ui->brightness_slider = slider_create(box, 244, 190, 240, 24);
  slider_set_min(ui->brightness_slider, 20);
  slider_set_max(ui->brightness_slider, 100);
  slider_set_step(ui->brightness_slider, 1);
  widget_on(ui->brightness_slider, EVT_VALUE_CHANGED, on_brightness_changed, ui);
  ui->brightness_label = label_ex(box, 500, 184, 80, 28, "82%", "#edf6ff", 16, "left");

  ui->mode_label = label_ex(box, 430, 70, 180, 34, "放电", "#edf6ff", 22, "left");
  widget_on(button_ex(box, 430, 116, 132, 34, "切换模式"), EVT_CLICK, on_mode, ui);
  widget_on(button_ex(box, 28, 230, 128, 38, "保存"), EVT_CLICK, on_save_settings, ui);
  ui->save_status = label_ex(box, 178, 232, 330, 34, "启动时自动加载配置", "#8aa4bf", 15, "left");
}

static void refresh_header(ui_controller_t* ui) {
  uint32_t active = alarm_manager_active_count(ui->model);
  widget_set_text_utf8(ui->status_badge, ui->model->battery.fault ? "故障" : "在线");
  widget_set_style_str(ui->status_badge, "normal:text_color",
                       ui->model->battery.fault ? "#ff7777" : "#bdfbd7");
  set_label(ui->alarm_badge, "告警 %u", active);
  widget_set_style_str(ui->alarm_badge, "normal:text_color", active > 0 ? "#ffb24e" : "#bdfbd7");
  widget_set_text_utf8(ui->mode_badge, power_mode_to_string(ui->model->config.mode));
}

static void refresh_dashboard(ui_controller_t* ui) {
  uint32_t hours = ui->model->battery.runtime_seconds / 3600;
  uint32_t minutes = (ui->model->battery.runtime_seconds / 60) % 60;

  progress_circle_set_value(ui->soc_ring, ui->model->battery.soc);
  set_label(ui->voltage_label, "%.1f V\n总电压", ui->model->battery.voltage);
  set_label(ui->current_label, "%.1f A\n电流", ui->model->battery.current);
  set_label(ui->temp_label, "%.1f ℃\n温度", ui->model->battery.temperature);
  set_label(ui->runtime_label, "%02u:%02u\n运行时间", hours, minutes);
  set_label(ui->state_label, "%s | 报文 %u",
            ui->model->battery.fault ? "存在故障" : "运行正常",
            ui->model->battery.packet_count);
  widget_invalidate(ui->trend_canvas, NULL);
}

static void refresh_cells(ui_controller_t* ui) {
  uint32_t i = 0;
  uint32_t min_i = 0;
  uint32_t max_i = 0;
  float min_v = battery_model_min_cell(ui->model, &min_i);
  float max_v = battery_model_max_cell(ui->model, &max_i);

  set_label(ui->cell_summary, "最高 C%u %.3fV | 最低 C%u %.3fV | 压差 %.0fmV", max_i + 1, max_v,
            min_i + 1, min_v, (max_v - min_v) * 1000.0f);

  for (i = 0; i < POWERGUARD_CELL_COUNT; i++) {
    set_label(ui->cell_labels[i], "%.3f V %s", ui->model->battery.cells[i],
              ui->model->battery.balancing[i] ? "均衡" : "");
    progress_bar_set_value(ui->cell_bars[i], ui->model->battery.cells[i] * 1000.0f);
    widget_set_style_str(ui->cell_bars[i], "fg_color",
                         ui->model->battery.balancing[i] ? "#ffb24e" : "#2f84ff");
  }
}

static void refresh_alarms(ui_controller_t* ui) {
  uint32_t i = 0;
  uint32_t active = alarm_manager_active_count(ui->model);

  set_label(ui->alarm_summary, "%u 活动 / %u 历史", active, ui->model->alarm_count);
  widget_set_style_str(ui->alarm_summary, "normal:text_color", active > 0 ? "#ffb24e" : "#bdfbd7");

  for (i = 0; i < POWERGUARD_ALARM_HISTORY_MAX; i++) {
    if (i < ui->model->alarm_count) {
      alarm_record_t* a = ui->model->alarms + i;
      set_label(ui->alarm_rows[i], "%s  %-18s  %-8s  %-8s  首次 %us",
                alarm_type_to_code(a->type), alarm_type_to_string(a->type),
                a->active ? "活动" : "已清除", a->acknowledged ? "已确认" : "未确认",
                a->first_seen_seconds);
      widget_set_style_str(ui->alarm_rows[i], "normal:text_color",
                           a->active ? (a->acknowledged ? "#ffdd99" : "#ff7777") : "#8aa4bf");
    } else {
      widget_set_text_utf8(ui->alarm_rows[i], "");
    }
  }
}

static void refresh_settings(ui_controller_t* ui) {
  char text[64];

  tk_snprintf(text, sizeof(text), "%.1f", ui->model->config.max_temperature);
  widget_set_text_utf8(ui->edit_max_temp, text);
  tk_snprintf(text, sizeof(text), "%.1f", ui->model->config.min_voltage);
  widget_set_text_utf8(ui->edit_min_voltage, text);
  tk_snprintf(text, sizeof(text), "%.1f", ui->model->config.max_current);
  widget_set_text_utf8(ui->edit_max_current, text);

  slider_set_value(ui->brightness_slider, ui->model->config.brightness);
  set_label(ui->brightness_label, "%u%%", ui->model->config.brightness);
  widget_set_text_utf8(ui->mode_label, power_mode_to_string(ui->model->config.mode));
}

ui_controller_t* ui_controller_create(widget_t* win, powerguard_model_t* model) {
  ui_controller_t* ui = TKMEM_ZALLOC(ui_controller_t);

  return_value_if_fail(ui != NULL, NULL);

  ui->win = win;
  ui->model = model;
  ui->page = PAGE_DASHBOARD;

  create_header(ui);
  create_nav(ui);
  create_dashboard(ui);
  create_cells(ui);
  create_alarms(ui);
  create_settings(ui);

  set_page(ui, PAGE_DASHBOARD);

  return ui;
}

void ui_controller_refresh(ui_controller_t* ui) {
  return_if_fail(ui != NULL);

  refresh_header(ui);
  refresh_dashboard(ui);
  refresh_cells(ui);
  refresh_alarms(ui);
  if (ui->page != PAGE_SETTINGS) {
    refresh_settings(ui);
  }
}

void ui_controller_destroy(ui_controller_t* ui) {
  if (ui != NULL) {
    TKMEM_FREE(ui);
  }
}
