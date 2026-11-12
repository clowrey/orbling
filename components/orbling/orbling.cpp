#include "orbling.h"

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <cstring>

namespace esphome {
namespace orbling {

static const char *const TAG = "orbling";

void FrameCanvas::fill(int x, int y, int w, int h, uint32_t rgb) {
  uint16_t c = ((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F);
  c = (uint16_t) ((c >> 8) | (c << 8));
  for (int yy = y; yy < y + h; yy++) {
    uint16_t *p = this->buf + yy * petcore::SCREEN + x;
    for (int i = 0; i < w; i++) p[i] = c;
  }
}

void Orbling::setup() {
  RAMAllocator<uint16_t> allocator(RAMAllocator<uint16_t>::ALLOC_EXTERNAL);
  this->frames_[0] = allocator.allocate(petcore::SCREEN * petcore::SCREEN);
  this->frames_[1] = allocator.allocate(petcore::SCREEN * petcore::SCREEN);
  this->canvas_.buf = this->frames_[0];
  if (this->frames_[0] == nullptr || this->frames_[1] == nullptr) {
    ESP_LOGE(TAG, "No PSRAM for the frame buffer");
    this->mark_failed();
    return;
  }

  // The key keeps its original name so saves from earlier builds load.
  const uint32_t key = fnv1_hash("pixel_pet_save");
  this->pref_ = global_preferences->make_preference<petcore::SaveData>(key);
  petcore::SaveData saved{};
  // Older firmware saved fewer bytes under the same key. Fields are only appended, so an old save is the start
  // of a new one; new fields stay zero and the engine fills in what needs a default (it reads the magic).
  bool loaded = this->pref_.load(&saved) || load_older_save_<petcore::SAVE_V9_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V8_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V7_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V6_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V5_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V4_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V3_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V2_SIZE>(key, saved) ||
                load_older_save_<petcore::SAVE_V1_SIZE>(key, saved);
  if (loaded && this->engine_.restore(saved, random_uint32())) {
    ESP_LOGI(TAG, "Restored %s (%s, %s, generation %d)", this->engine_.name(), this->engine_.species(),
             this->engine_.stage_name(), this->engine_.generation());
  } else {
    this->engine_.new_game(random_uint32());
    ESP_LOGI(TAG, "New egg: %s", this->engine_.name());
    this->save_now();
  }
  this->touchscreen_->register_listener(this);
}

void Orbling::save_now() {
  petcore::SaveData d = this->engine_.save_data();
  this->pref_.save(&d);
  this->engine_.take_dirty();
  this->last_save_ = millis();
}

void Orbling::hatch_new_egg() {
  this->engine_.hatch_new_egg(random_uint32());
  this->save_now();
}

void Orbling::loop() {
  uint32_t now = millis();
  ESPTime t = this->time_->now();
  if (!t.is_valid() && this->fallback_time_ != nullptr) t = this->fallback_time_->now();
  if (t.is_valid()) this->engine_.set_clock((uint32_t) t.timestamp, t.hour * 60 + t.minute);
  this->engine_.loop(now);

  // Preferences are cached in RAM and written to flash on ESPHome's own schedule.
  if (now - this->last_save_ > 30000 && this->engine_.take_dirty()) this->save_now();

  int fps = this->engine_.wanted_fps();
  if (now - this->last_frame_ >= (uint32_t) (1000 / fps)) {
    this->last_frame_ = now;
    uint32_t start = micros();
    this->engine_.render(this->canvas_, now);
    uint32_t drawn = micros();
    this->push_changed_bands_();
    uint32_t end = micros();
    this->frame_us_ = end - start;
    this->stat_frames_++;
    this->stat_draw_us_ += drawn - start;
    this->stat_push_us_ += end - drawn;
  }
  if (now - this->stat_since_ >= 10000) {
    if (this->stat_frames_)
      ESP_LOGD(TAG, "%.1f fps, draw %u us, push %u us per frame",
               this->stat_frames_ * 1000.0f / (float) (now - this->stat_since_),
               (unsigned) (this->stat_draw_us_ / this->stat_frames_), (unsigned) (this->stat_push_us_ / this->stat_frames_));
    this->stat_since_ = now;
    this->stat_frames_ = this->stat_draw_us_ = this->stat_push_us_ = 0;
  }
}

// Send only the horizontal bands that differ from the previous frame (runs of them as one transfer),
// then swap buffers. Most of the scene is still, so this skips most of the 260 KB push.
void Orbling::push_changed_bands_() {
  static const int BAND = 24, BANDS = petcore::SCREEN / BAND;
  const size_t band_px = (size_t) BAND * petcore::SCREEN;
  uint16_t *cur = this->frames_[this->cur_], *prev = this->frames_[this->cur_ ^ 1];
  uint32_t now = millis();
  if (now < 5000 || now - this->last_full_push_ > 60000) {  // the panel may still be starting; refresh now and then
    this->full_push_ = true;
    this->last_full_push_ = now;
  }
  int run_start = -1;
  for (int b = 0; b <= BANDS; b++) {
    bool changed = false;
    if (b < BANDS)
      changed = this->full_push_ || memcmp(cur + b * band_px, prev + b * band_px, band_px * 2) != 0;
    if (changed && run_start < 0) run_start = b;
    if (!changed && run_start >= 0) {
      this->display_->draw_pixels_at(0, run_start * BAND, petcore::SCREEN, (b - run_start) * BAND,
                                     (const uint8_t *) (cur + run_start * band_px), display::COLOR_ORDER_RGB,
                                     display::COLOR_BITNESS_565, true, 0, 0, 0);
      run_start = -1;
    }
  }
  this->full_push_ = false;
  this->cur_ ^= 1;
  this->canvas_.buf = this->frames_[this->cur_];
}

void Orbling::touch(touchscreen::TouchPoint tp) {
  ESP_LOGD(TAG, "Touch at %d,%d", tp.x, tp.y);
  this->engine_.touch_down(tp.x, tp.y, millis());
}

void Orbling::update(const touchscreen::TouchPoints_t &tpoints) {
  if (!tpoints.empty()) this->engine_.touch_move(tpoints[0].x, tpoints[0].y);
}

void Orbling::release() { this->engine_.touch_up(millis()); }

void Orbling::dump_config() {
  ESP_LOGCONFIG(TAG, "Orbling:\n  Pet: %s (%s, %s, generation %d)\n  Last frame: %u us", this->engine_.name(),
                this->engine_.species(), this->engine_.stage_name(), this->engine_.generation(),
                (unsigned) this->frame_us_);
}

}  // namespace orbling
}  // namespace esphome
