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

        uint32_t lastSecs = 0;
        uint32_t msAtSecond = 0;

        // Matrix-rain backdrop (drawn behind the dashboard)
        static constexpr uint8_t RainCols = 12;
        lv_obj_t* rain[RainCols] = {nullptr};
        int16_t rainY[RainCols] = {0};
        uint8_t rainSpeed[RainCols] = {0};

        lv_style_t ledStyle; // shared static styling for the binary LEDs

        lv_obj_t* label_epoch = nullptr;
        lv_obj_t* label_hex = nullptr;
        lv_obj_t* label_human = nullptr;
        lv_obj_t* label_2038 = nullptr;
        lv_obj_t* label_days = nullptr;
        lv_obj_t* bar = nullptr;
        lv_obj_t* bits[16] = {nullptr};

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
