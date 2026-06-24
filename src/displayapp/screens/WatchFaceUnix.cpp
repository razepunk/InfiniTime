#include "displayapp/screens/WatchFaceUnix.h"

using namespace Pinetime::Applications::Screens;

// InfiniTime ships this font; declare it so we can use it here.
LV_FONT_DECLARE(jetbrains_mono_bold_20)

namespace {
  // LVGL 8.x callback. On LVGL 7 branches: void cb(lv_task_t* task) + lv_task_create.
  void RefreshTaskCallback(lv_timer_t* task) {
    auto* screen = static_cast<WatchFaceUnix*>(task->user_data);
    screen->Refresh();
  }
}

WatchFaceUnix::WatchFaceUnix(Controllers::DateTime& dateTimeController,
                             const Controllers::Settings& settingsController)
  : dateTimeController {dateTimeController}, settingsController {settingsController} {

  label_caption = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(label_caption, lv_color_hex(0x888888), LV_PART_MAIN);
  lv_label_set_text_static(label_caption, "UNIX EPOCH");
  lv_obj_align(label_caption, LV_ALIGN_CENTER, 0, -34);

  label_epoch = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_epoch, &jetbrains_mono_bold_20, LV_PART_MAIN);
  lv_obj_set_style_text_color(label_epoch, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_label_set_text_static(label_epoch, "0000000000");
  lv_obj_align(label_epoch, LV_ALIGN_CENTER, 0, 0);

  // Reminder that the value tracks the watch's clock, not guaranteed UTC.
  label_mode = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(label_mode, lv_color_hex(0x555555), LV_PART_MAIN);
  lv_label_set_text_static(label_mode, "watch local time");
  lv_obj_align(label_mode, LV_ALIGN_CENTER, 0, 34);

  taskRefresh = lv_timer_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, this);
  Refresh();
}

WatchFaceUnix::~WatchFaceUnix() {
  lv_timer_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceUnix::Refresh() {
  // CurrentDateTime() is a std::chrono::system_clock time_point with a 1970
  // epoch. IMPORTANT: it reflects whatever time the watch is set to. Most
  // companions (Gadgetbridge) sync LOCAL time, so this is local-as-if-UTC and
  // will be off from true Unix UTC by your timezone offset.
  //
  // To show true UTC epoch, subtract your offset in seconds, e.g.:
  //   ts -= settingsController.GetUTCOffset();   // if your branch exposes it
  // or hardcode it, e.g. for UTC-4 (EDT, Pittsburgh):
  //   ts += 4 * 3600;   // local -> UTC
  uint32_t ts = std::chrono::duration_cast<std::chrono::seconds>(
                  dateTimeController.CurrentDateTime().time_since_epoch())
                  .count();

  if (ts != lastShown) {
    lastShown = ts;
    lv_label_set_text_fmt(label_epoch, "%lu", static_cast<unsigned long>(ts));
  }
}
