#pragma once

#include <lvgl/lvgl.h>
#include <chrono>
#include <cstdint>

#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      class WatchFaceUnix : public Screen {
      public:
        WatchFaceUnix(Controllers::DateTime& dateTimeController,
                      const Controllers::Settings& settingsController);
        ~WatchFaceUnix() override;

        void Refresh() override;

      private:
        Controllers::DateTime& dateTimeController;
        const Controllers::Settings& settingsController;

        // Clock only has 1s resolution -> anchor each second, interpolate ms.
        uint32_t lastSecs = 0;
        uint32_t msAtSecond = 0;

        lv_obj_t* label_epoch = nullptr; // big, hue-cycling, with blinking cursor
        lv_obj_t* label_hex = nullptr;   // 0x.... view of the epoch
        lv_obj_t* label_human = nullptr; // YYYY-MM-DD HH:MM:SS
        lv_obj_t* label_2038 = nullptr;  // seconds until the 2038 overflow
        lv_obj_t* label_days = nullptr;  // days since 1970
        lv_obj_t* bar = nullptr;         // sweeps full each second
        lv_obj_t* bits[16] = {nullptr};  // low-16-bit binary "LED" row

        // LVGL 7 (this tree). On LVGL 8 branches this would be lv_timer_t*.
        lv_task_t* taskRefresh = nullptr;
      };
    }

    // ---- Watchface registration (app-registry) ----
    template <>
    struct WatchFaceTraits<WatchFace::Unix> {
      static constexpr WatchFace watchFace = WatchFace::Unix;
      static constexpr const char* name = "Unix epoch";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceUnix(controllers.dateTimeController,
                                          controllers.settingsController);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
