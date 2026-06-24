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

        // Only redraw when the second actually ticks over.
        uint32_t lastShown = 0;

        lv_obj_t* label_caption = nullptr;
        lv_obj_t* label_epoch = nullptr;
        lv_obj_t* label_mode = nullptr;

        // LVGL 8.x. On older (LVGL 7) branches this is lv_task_t*.
        lv_timer_t* taskRefresh = nullptr;
      };
    }

    // ---- Watchface registration (current `main` app-registry) ----
    // If your branch registers differently, delete this block and wire it up
    // the same way you did your previous epoch face.
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
