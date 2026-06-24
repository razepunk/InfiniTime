#include "displayapp/screens/WatchFaceUnix.h"

using namespace Pinetime::Applications::Screens;

// Built into the firmware by default (confirmed in the build log).
LV_FONT_DECLARE(jetbrains_mono_bold_20)

namespace {
  void unixRefreshTaskCallback(lv_task_t* task) {
    auto* screen = static_cast<WatchFaceUnix*>(task->user_data);
    screen->Refresh();
  }
}

WatchFaceUnix::WatchFaceUnix(Controllers::DateTime& dateTimeController,
                             const Controllers::Settings& settingsController)
  : dateTimeController {dateTimeController}, settingsController {settingsController} {

  label_caption = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_caption, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x888888));
  lv_label_set_text_static(label_caption, "UNIX EPOCH");
  lv_obj_align(label_caption, lv_scr_act(), LV_ALIGN_CENTER, 0, -55);

  // Big, fast-ticking epoch.milliseconds
  label_epoch = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_epoch, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_set_style_local_text_color(label_epoch, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text_static(label_epoch, "0000000000.000");
  lv_obj_align(label_epoch, lv_scr_act(), LV_ALIGN_CENTER, 0, -25);

  // Hex view of the whole-second epoch
  label_hex = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_hex, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00AAAA));
  lv_label_set_text_static(label_hex, "0x00000000");
  lv_obj_align(label_hex, lv_scr_act(), LV_ALIGN_CENTER, 0, 5);

  // Bar that sweeps full once per second
  bar = lv_bar_create(lv_scr_act(), nullptr);
  lv_obj_set_size(bar, 180, 6);
  lv_obj_align(bar, lv_scr_act(), LV_ALIGN_CENTER, 0, 32);
  lv_bar_set_range(bar, 0, 999);
  lv_bar_set_anim_time(bar, 0);
  lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0x111111));
  lv_obj_set_style_local_bg_color(bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));

  // Reminder that the value tracks the watch's clock, not guaranteed UTC.
  label_mode = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_mode, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x555555));
  lv_label_set_text_static(label_mode, "watch local time");
  lv_obj_align(label_mode, lv_scr_act(), LV_ALIGN_CENTER, 0, 55);

  taskRefresh = lv_task_create(unixRefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceUnix::~WatchFaceUnix() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceUnix::Refresh() {
  // Whole seconds from the RTC. CurrentDateTime() only has 1s resolution, so we
  // anchor each second boundary and interpolate the fraction with the LVGL ms
  // tick. (NOTE: still local-as-if-UTC unless your companion syncs UTC; add
  // your offset to 'secs' for true Unix UTC, e.g. secs += 4 * 3600 for UTC-4.)
  uint32_t secs = std::chrono::duration_cast<std::chrono::seconds>(
                    dateTimeController.CurrentDateTime().time_since_epoch())
                    .count();
  uint32_t nowMs = lv_tick_get();

  if (secs != lastSecs) {
    lastSecs = secs;
    msAtSecond = nowMs; // re-anchor on every real second tick
    lv_label_set_text_fmt(label_hex, "0x%lX", static_cast<unsigned long>(secs));
  }

  uint32_t frac = nowMs - msAtSecond; // ms elapsed since the second boundary
  if (frac > 999) {
    frac = 999; // clamp (e.g. after the screen was off and ticks were missed)
  }

  lv_label_set_text_fmt(label_epoch, "%lu.%03lu",
                        static_cast<unsigned long>(secs),
                        static_cast<unsigned long>(frac));
  lv_bar_set_value(bar, static_cast<int16_t>(frac), LV_ANIM_OFF);
}
