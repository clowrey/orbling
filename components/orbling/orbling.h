#pragma once

#include "esphome/components/display/display.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"

#include <cstring>

#include "pet_engine.h"

namespace esphome {
namespace orbling {

// Draws into an RGB565 frame (big-endian, as the panel wants it) in PSRAM.
class FrameCanvas : public petcore::Canvas {
 public:
  uint16_t *buf{nullptr};
  void fill(int x, int y, int w, int h, uint32_t rgb) override;
};

// Runs the pet, saves it to flash, and pushes whole frames to the display (no LVGL).
class Orbling : public Component, public touchscreen::TouchListener {
 public:
  void set_display(display::Display *d) { this->display_ = d; }
  void set_touchscreen(touchscreen::Touchscreen *ts) { this->touchscreen_ = ts; }
  void set_time(time::RealTimeClock *t) { this->time_ = t; }
  void set_fallback_time(time::RealTimeClock *t) { this->fallback_time_ = t; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void touch(touchscreen::TouchPoint tp) override;
  void update(const touchscreen::TouchPoints_t &tpoints) override;
  void release() override;

  petcore::PetEngine &pet() { return this->engine_; }
  float backlight() const { return this->engine_.backlight(millis()); }
  void hatch_new_egg();
  void save_now();

 protected:
  petcore::PetEngine engine_;
  FrameCanvas canvas_;
  ESPPreferenceObject pref_;
  display::Display *display_{nullptr};
  touchscreen::Touchscreen *touchscreen_{nullptr};
  time::RealTimeClock *time_{nullptr};
  time::RealTimeClock *fallback_time_{nullptr};  // used while time_ has no valid time
  uint32_t last_frame_{0};
  uint32_t last_save_{0};
  void push_changed_bands_();
  template<size_t N> static bool load_older_save_(uint32_t key, petcore::SaveData &out) {
    struct Blob {
      uint8_t bytes[N];
    } blob;
    auto pref = global_preferences->make_preference<Blob>(key);
    if (!pref.load(&blob)) return false;
    memcpy(&out, blob.bytes, N);
    ESP_LOGI("orbling", "Upgraded a %u-byte save", (unsigned) N);
    return true;
  }
  uint16_t *frames_[2]{nullptr, nullptr};
  uint8_t cur_{0};
  bool full_push_{true};  // send the whole frame, not just the changed bands
  uint32_t last_full_push_{0};
  uint32_t frame_us_{0};
  uint32_t stat_since_{0}, stat_frames_{0}, stat_draw_us_{0}, stat_push_us_{0};
};

}  // namespace orbling
}  // namespace esphome
