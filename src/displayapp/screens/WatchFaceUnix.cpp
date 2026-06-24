#include "displayapp/screens/WatchFaceUnix.h"

using namespace Pinetime::Applications::Screens;

// Built into the firmware by default (confirmed in the build log).
LV_FONT_DECLARE(jetbrains_mono_bold_20)

namespace {
  void unixRefreshTaskCallback(lv_task_t* task) {
    auto* screen = static_cast<WatchFaceUnix*>(task->user_data);
    screen->Refresh();
  }

  constexpr uint32_t Y2038 = 2147483647u; // INT32_MAX: the 32-bit epoch overflow

  lv_color_t OnColor() {
    return lv_color_hex(0x00FF00);
  }
  lv_color_t OffColor() {
    return lv_color_hex(0x1E1E1E);
  }
}

WatchFaceUnix::WatchFaceUnix(Controllers::DateTime& dateTimeController,
                             const Controllers::Settings& settingsController)
  : dateTimeController {dateTimeController}, settingsController {settingsController} {

  // Big epoch.milliseconds with blinking cursor (color set in Refresh)
  label_epoch = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_epoch, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_text_static(label_epoch, "0000000000.000_");
  lv_obj_align(label_epoch, lv_scr_act(), LV_ALIGN_CENTER, 0, -62);

  // Hex view
  label_hex = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_hex, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00AAAA));
  lv_label_set_text_static(label_hex, "0x00000000");
  lv_obj_align(label_hex, lv_scr_act(), LV_ALIGN_CENTER, 0, -40);

  // Human-readable date + time (doubles as a normal clock)
  label_human = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_human, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCCCCCC));
  lv_label_set_text_static(label_human, "0000-00-00 00:00:00");
  lv_obj_align(label_human, lv_scr_act(), LV_ALIGN_CENTER, 0, -19);

  // Binary "LED" row: low 16 bits, MSB left -> LSB right (right side flips fast)
  for (int i = 0; i < 16; i++) {
    bits[i] = lv_obj_create(lv_scr_act(), nullptr);
    lv_obj_set_size(bits[i], 10, 10);
    lv_obj_align(bits[i], lv_scr_act(), LV_ALIGN_CENTER, i * 13 - 98, 3);
    lv_obj_set_style_local_radius(bits[i], LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_width(bits[i], LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_bg_opa(bits[i], LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bits[i], LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, OffColor());
  }

  // Second-sweep bar (indicator color set in Refresh to match the epoch hue)
  bar = lv_bar_create(lv_scr_act(), nullptr);
  lv_obj_set_size(bar, 180, 6);
  lv_obj_align(bar, lv_scr_act(), LV_ALIGN_CENTER, 0, 24);
  lv_bar_set_range(bar, 0, 999);
  lv_bar_set_anim_time(bar, 0);
  lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0x111111));

  // T-2038 countdown (amber)
  label_2038 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_2038, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFAA00));
  lv_label_set_text_static(label_2038, "T-2038 0s");
  lv_obj_align(label_2038, lv_scr_act(), LV_ALIGN_CENTER, 0, 45);

  // Days since epoch (dim)
  label_days = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_days, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x666666));
  lv_label_set_text_static(label_days, "DAY 0");
  lv_obj_align(label_days, lv_scr_act(), LV_ALIGN_CENTER, 0, 64);

  taskRefresh = lv_task_create(unixRefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceUnix::~WatchFaceUnix() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceUnix::Refresh() {
  // Whole seconds from the RTC (1s resolution). NOTE: local-as-if-UTC unless
  // your companion syncs UTC; add your offset to 'secs' for true Unix UTC.
  uint32_t secs = std::chrono::duration_cast<std::chrono::seconds>(
                    dateTimeController.CurrentDateTime().time_since_epoch())
                    .count();
  uint32_t nowMs = lv_tick_get();

  // Everything that only changes once per second is updated on the boundary.
  if (secs != lastSecs) {
    lastSecs = secs;
    msAtSecond = nowMs;

    // Hue cycles the full wheel once per minute (6 deg/sec).
    lv_color_t hue = lv_color_hsv_to_rgb((secs % 60) * 6, 100, 100);
    lv_obj_set_style_local_text_color(label_epoch, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, hue);
    lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, hue);

    lv_label_set_text_fmt(label_hex, "0x%lX", static_cast<unsigned long>(secs));

    lv_label_set_text_fmt(label_human, "%04d-%02d-%02d %02d:%02d:%02d",
                          static_cast<int>(dateTimeController.Year()),
                          static_cast<int>(dateTimeController.Month()),
                          static_cast<int>(dateTimeController.Day()),
                          static_cast<int>(dateTimeController.Hours()),
                          static_cast<int>(dateTimeController.Minutes()),
                          static_cast<int>(dateTimeController.Seconds()));

    for (int i = 0; i < 16; i++) {
      bool on = (secs >> (15 - i)) & 1u;
      lv_obj_set_style_local_bg_color(bits[i], LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, on ? OnColor() : OffColor());
    }

    uint32_t remain = (secs < Y2038) ? (Y2038 - secs) : 0;
    lv_label_set_text_fmt(label_2038, "T-2038 %lus", static_cast<unsigned long>(remain));

    lv_label_set_text_fmt(label_days, "DAY %lu", static_cast<unsigned long>(secs / 86400u));
  }

  // Sub-second part, updated every frame.
  uint32_t frac = nowMs - msAtSecond;
  if (frac > 999) {
    frac = 999;
  }

  const char* cursor = ((nowMs / 500u) % 2u) ? "_" : " ";
  lv_label_set_text_fmt(label_epoch, "%lu.%03lu%s",
                        static_cast<unsigned long>(secs),
                        static_cast<unsigned long>(frac),
                        cursor);
  lv_bar_set_value(bar, static_cast<int16_t>(frac), LV_ANIM_OFF);
}
