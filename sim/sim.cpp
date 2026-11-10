// Host simulator: renders Orbling screens to PNG files and runs the game rules fast.
// Build and run (WSL or Linux):  sh sim/run.sh   -> sim/out/*.png
#include "../components/orbling/pet_engine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace petcore;

// ---- tiny PNG writer (stored deflate) ----
static uint32_t crc_table[256];
static void crc_init() {
  for (uint32_t n = 0; n < 256; n++) {
    uint32_t c = n;
    for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
  }
}
static uint32_t crc(const uint8_t *buf, size_t len, uint32_t c = 0xFFFFFFFFu) {
  for (size_t i = 0; i < len; i++) c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
  return c;
}
static void put32(std::vector<uint8_t> &v, uint32_t x) {
  v.push_back(x >> 24);
  v.push_back(x >> 16);
  v.push_back(x >> 8);
  v.push_back(x);
}
static void chunk(FILE *f, const char *type, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> v;
  put32(v, (uint32_t) data.size());
  size_t start = v.size();
  v.insert(v.end(), type, type + 4);
  v.insert(v.end(), data.begin(), data.end());
  uint32_t c = crc(v.data() + start, v.size() - start) ^ 0xFFFFFFFFu;
  put32(v, c);
  fwrite(v.data(), 1, v.size(), f);
}
static void write_png(const std::string &path, int w, int h, const std::vector<uint32_t> &px) {
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) return;
  const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  fwrite(sig, 1, 8, f);
  std::vector<uint8_t> ihdr;
  put32(ihdr, w);
  put32(ihdr, h);
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
  chunk(f, "IHDR", ihdr);
  std::vector<uint8_t> raw;
  for (int y = 0; y < h; y++) {
    raw.push_back(0);
    for (int x = 0; x < w; x++) {
      uint32_t c = px[y * w + x];
      raw.push_back(c >> 16);
      raw.push_back(c >> 8);
      raw.push_back(c);
    }
  }
  std::vector<uint8_t> z = {0x78, 0x01};
  uint32_t a = 1, b = 0;
  for (uint8_t ch : raw) {
    a = (a + ch) % 65521;
    b = (b + a) % 65521;
  }
  size_t pos = 0;
  while (pos < raw.size()) {
    size_t n = std::min<size_t>(65535, raw.size() - pos);
    z.push_back(pos + n == raw.size() ? 1 : 0);
    z.push_back(n & 0xFF);
    z.push_back(n >> 8);
    z.push_back(~n & 0xFF);
    z.push_back((~n >> 8) & 0xFF);
    z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
    pos += n;
  }
  put32(z, (b << 16) | a);
  chunk(f, "IDAT", z);
  chunk(f, "IEND", {});
  fclose(f);
}

// ---- canvas ----
class Buffer : public Canvas {
 public:
  std::vector<uint32_t> px = std::vector<uint32_t>(SCREEN * SCREEN, 0);
  void fill(int x, int y, int w, int h, uint32_t rgb) override {
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++)
        if (xx >= 0 && yy >= 0 && xx < SCREEN && yy < SCREEN) px[yy * SCREEN + xx] = rgb;
  }
  // The panel is round: grey out what the glass hides.
  void mask() {
    for (int y = 0; y < SCREEN; y++)
      for (int x = 0; x < SCREEN; x++) {
        float dx = x + 0.5f - 180, dy = y + 0.5f - 180;
        if (dx * dx + dy * dy > 180.0f * 180.0f) px[y * SCREEN + x] = 0x303030;
      }
  }
};

static const uint32_t NOON_UTC = 1794268800u + 22 * 3600;  // 2026-11-09 12:00 HST
static std::string out_dir = "sim/out/";
static std::vector<std::pair<std::string, std::vector<uint32_t>>> shots;

static void shot(PetEngine &e, const char *name, uint32_t now) {
  Buffer b;
  e.render(b, now);
  b.mask();
  write_png(out_dir + name + ".png", SCREEN, SCREEN, b.px);
  shots.push_back({name, b.px});
}

static void prep(PetEngine &e, uint32_t seed, uint8_t stage, int local_min = 12 * 60) {
  e.new_game(seed);
  SaveData &s = e.state();
  static const uint32_t AGE[6] = {0, 30, 300, 2 * 1440, 5 * 1440, 15 * 1440};
  s.stage = stage;
  s.age_min = AGE[stage];
  s.weight = (uint8_t[]){0, 5, 10, 20, 30, 30}[stage];
  s.food = 80;
  s.happy = 60;
  s.energy = 80;
  s.clean = 90;
  s.health = 100;
  e.update_names();
  uint32_t epoch = NOON_UTC + (local_min - 12 * 60) * 60;
  e.set_clock(epoch, local_min);
  e.loop(1000);
}

static void tap(PetEngine &e, int x, int y, uint32_t &now) {
  e.touch_down(x, y, now);
  now += 80;
  e.touch_up(now);
  e.loop(now);
}

static void button_xy(int i, int &x, int &y) {
  float a = (-70.0f + 28.0f * i) * 3.14159265f / 180.0f;
  x = (int) lroundf(180 + 138 * sinf(a));
  y = (int) lroundf(180 + 138 * cosf(a));
}

// Autoplay the jump game: jump when the next ground obstacle is `lead` seconds away, never at birds.
// Returns the score at the crash (or after 3 minutes). With shots, saves frames mid-run and at the crash.
// lead < 0: a sloppy player, each jump timed at a random lead between 0.05 and 0.50 s.
// crash_after_ms: from then on jump far too early, to end the run (for the screenshots).
static int play_runner(PetEngine &e, uint32_t &now, float lead, bool shots, uint32_t crash_after_ms = 0) {
  bool down = false;
  uint32_t down_at = 0;
  int shot_n = 0;
  uint32_t start = now;
  float base_lead = lead;
  static uint32_t jitter = 12345;
  auto roll = [&]() {
    jitter = jitter * 1103515245u + 12345u;
    return 0.05f + ((jitter >> 8) % 1000) / 1000.0f * 0.45f;
  };
  if (base_lead < 0) lead = roll();
  e.touch_down(110, 180, now);  // first press starts the run
  now += 40;
  e.touch_up(now);
  while (now - start < 180000) {
    now += 16;
    e.loop(now);
    if (e.run_over()) break;
    if (crash_after_ms && now - start > crash_after_ms) lead = 0.9f;
    bool flying = false;
    float d = e.next_obstacle(&flying);
    if (!down && !flying && d > 0 && d < e.run_speed() * lead) {
      e.touch_down(110, 180, now);
      down = true;
      down_at = now;
      if (base_lead < 0) lead = roll();
    }
    if (down && now - down_at > 60) {
      e.touch_up(now);
      down = false;
    }
    if (shots && ((shot_n == 0 && now - start > 6000 && down) || (shot_n == 1 && flying && d < 90 && d > 30))) {
      shot(e, shot_n == 0 ? "13-run-jump" : "14-run-bird", now);
      shot_n++;
    }
  }
  if (down) e.touch_up(now);
  int score = e.run_score();
  if (shots) {
    now += 300;
    e.loop(now);
    shot(e, "15-run-ouch", now);
  }
  now += 900;
  e.loop(now);
  tap(e, 180, 180, now);  // finish
  return score;
}

// Autoplay one of the newer games for up to `ms`. skill 0..1: how often the bot does the right thing.
// Shots: a frame mid-game and one at the end, named after the game.
static int play_other(PetEngine &e, uint32_t &now, int kind, float skill, uint32_t ms, const char *shot_name) {
  static uint32_t r = 777;
  auto chance = [&]() {
    r = r * 1103515245u + 12345u;
    return ((r >> 8) % 1000) / 1000.0f;
  };
  e.start_game(kind);
  tap(e, 180, 330, now);  // start
  uint32_t start = now, next_act = now;
  bool down = false, shot_taken = false;
  while (now - start < ms && !e.run_over() && e.in_game()) {
    now += 16;
    e.loop(now);
    int n;
    const PetEngine::Obstacle *objs = e.objects(&n);
    if (kind == GAME_CATCH) {
      // Head for the lowest fruit; sometimes (1-skill) dawdle.
      const PetEngine::Obstacle *best = nullptr;
      for (int i = 0; i < n; i++)
        if (objs[i].active && objs[i].kind != PetEngine::OB_ROCK && (!best || objs[i].y > best->y)) best = &objs[i];
      if (best && now >= next_act) {
        int x = (int) best->x + 18;
        if (!down) {
          e.touch_down(x, 300, now);
          down = true;
        }
        e.touch_move(x, 300);
        next_act = now + (chance() < skill ? 0 : 400);
      }
    } else if (kind == GAME_POP) {
      if (down) {
        e.touch_up(now);
        down = false;
      }
      if (now >= next_act) {
        for (int i = 0; i < n; i++) {
          const auto &o = objs[i];
          if (!o.active || o.passed || now - o.t < 120) continue;
          bool tap_it = o.kind != PetEngine::OB_SPIKY || chance() > skill;
          if (!tap_it) continue;
          e.touch_down((int) o.x, (int) o.y, now);
          down = true;
          break;
        }
        next_act = now + 250 + (uint32_t) ((1 - skill) * 500);
      }
    } else if (kind == GAME_FIZZ) {
      // Pops a bubble once it has risen a bit; the right one `skill` of the time.
      if (down) {
        e.touch_up(now);
        down = false;
      }
      if (e.math_waiting() && now >= next_act) {
        const PetEngine::Obstacle *right = nullptr, *wrong = nullptr;
        for (int i = 0; i < n; i++) {
          const auto &o = objs[i];
          if (!o.active || o.passed || o.y > 280) continue;
          if (o.value == e.math_answer()) right = &o;
          else wrong = &o;
        }
        // Decide once per question (when the right bubble is in reach) whether this one is known.
        static uint32_t decided_for = 0;
        static bool knows = true;
        if (right && decided_for != right->t) {
          decided_for = right->t;
          knows = chance() < skill;
        }
        // Not sure: guess among the bubbles still there (the right one included), like a person would.
        const PetEngine::Obstacle *pick = nullptr;
        if (right && knows) {
          pick = right;
        } else if (right) {
          const PetEngine::Obstacle *avail[4];
          int m = 0;
          for (int i = 0; i < n && m < 4; i++)
            if (objs[i].active && !objs[i].passed && objs[i].y <= 280) avail[m++] = &objs[i];
          pick = m ? avail[(int) (chance() * m) % m] : right;
        }
        (void) wrong;
        if (pick) {
          e.touch_down((int) pick->x, (int) pick->y, now);
          down = true;
          next_act = now + 900;
        }
      }
    } else if (kind == GAME_MUNCH) {
      // Heads for the lowest food that makes the target; `skill` of the time it reads the fact right.
      const PetEngine::Obstacle *best = nullptr;
      for (int i = 0; i < n; i++) {
        const auto &o = objs[i];
        if (!o.active) continue;
        bool right = (o.value / 16) * (o.value % 16) == e.munch_target();
        uint32_t h = (o.t * 2654435761u) ^ (o.value * 40503u);  // the same verdict for an item every frame
        bool thinks_right = ((h >> 8) % 1000) / 1000.0f < skill ? right : !right;
        if (thinks_right && (!best || o.y > best->y)) best = &o;
      }
      int x = best ? (int) best->x : 180;
      if (!down) {
        e.touch_down(x, 300, now);
        down = true;
      }
      e.touch_move(x, 300);
    } else if (is_quiz_like(kind) || kind == GAME_FACTS) {
      // A third grader: thinks 1.5-4 s, knows the answer `skill` of the time.
      if (down) {
        e.touch_up(now);
        down = false;
      }
      static std::string typing;
      static uint32_t think_until = 0;
      if (!e.math_waiting()) {
        typing.clear();
        think_until = 0;
      } else if (think_until == 0) {
        think_until = now + 1500 + (uint32_t) (chance() * 2500);
      } else if (now >= think_until && now >= next_act) {
        int answer = e.math_answer();
        bool knows = chance() < skill;
        if (is_quiz_like(kind)) {
          int pick = 0;
          for (int i = 0; i < 4; i++)
            if ((e.quiz_choice(i) == answer) == knows) pick = i;
          static const int QX[4] = {125, 235, 125, 235}, QY[4] = {176, 176, 268, 268};
          e.touch_down(QX[pick], QY[pick], now);
          down = true;
          think_until = 0;
        } else {
          if (typing.empty()) typing = std::to_string(knows ? answer : answer + (answer % 3 ? 1 : -1));
          int digit = typing[0] - '0';
          typing.erase(0, 1);
          int kx, ky, kw;
          PetEngine::facts_key_pos(digit, kx, ky, kw);
          e.touch_down(kx, ky, now);
          down = true;
          next_act = now + 300;
        }
      }
    } else if (kind == GAME_COPY) {
      if (down) {
        e.touch_up(now);
        down = false;
      }
      int pad;
      if (now >= next_act && e.copy_waiting(&pad)) {
        if (chance() > skill) pad = (pad + 1) % 4;  // a slip
        int px = 180 + (pad % 2 ? 62 : -62), py = 180 + (pad / 2 ? 62 : -62);
        e.touch_down(px, py, now);
        down = true;
        next_act = now + 350;
      }
    }
    static bool goal_shot = false;
    if (now - start < 100) goal_shot = false;
    if (shot_name && kind == GAME_MUNCH && !goal_shot && now - start > 700) {  // the first goal banner
      shot(e, (std::string(shot_name) + "-goal").c_str(), now);
      goal_shot = true;
    }
    if (shot_name && !shot_taken && now - start > 6000) {
      shot(e, shot_name, now);
      shot_taken = true;
    }
  }
  if (down) e.touch_up(now);
  int score = e.run_score();
  if (shot_name) {
    now += 300;
    e.loop(now);
    shot(e, (std::string(shot_name) + "-end").c_str(), now);
  }
  now += 900;
  e.loop(now);
  if (e.in_game()) tap(e, 180, 180, now);  // finish
  if (e.in_stand()) {  // the prize stand after a math game
    if (shot_name) {
      // Buy the first item you can afford (two taps), then try a second: only one per game.
      int coins0 = e.coins();
      static const int SX[6] = {96, 180, 264, 96, 180, 264}, SY[6] = {150, 150, 150, 246, 246, 246};
      for (int i = 0; i < 6; i++) {
        tap(e, SX[i], SY[i], now);
        tap(e, SX[i], SY[i], now);
        if (e.coins() != coins0) break;
      }
      int coins1 = e.coins();
      for (int i = 5; i >= 0; i--) {
        tap(e, SX[i], SY[i], now);
        tap(e, SX[i], SY[i], now);
      }
      printf("  stand: coins %d -> %d after one buy, %d after trying a second (should stay)\n", coins0, coins1,
             e.coins());
      shot(e, (std::string(shot_name) + "-stand").c_str(), now);
    }
    tap(e, 180, 324, now);  // Done
  }
  return score;
}

// Random taps and drags in every game and menu, to shake out crashes (run under the sanitizers).
static void fuzz(PetEngine &e) {
  uint32_t r = 4242, now = 50000;
  auto rnd = [&]() {
    r = r * 1103515245u + 12345u;
    return r >> 8;
  };
  for (int round = 0; round < 40; round++) {
    e.start_game(round % GAME_COUNT);
    for (int i = 0; i < 3000; i++) {
      now += 5 + rnd() % 40;
      e.loop(now);
      int x = (int) (rnd() % 400) - 20, y = (int) (rnd() % 400) - 20;
      switch (rnd() % 6) {
        case 0: e.touch_down(x, y, now); break;
        case 1: e.touch_move(x, y); break;
        case 2: e.touch_up(now); break;
        default: break;
      }
      if (i % 50 == 0) {
        struct Null : Canvas {
          void fill(int, int, int, int, uint32_t) override {}
        } nc;
        e.render(nc, now);
      }
    }
  }
}

// Random touches on the pages (not in a game), drawing into a canvas that checks every rectangle is on the screen.
static void fuzz_pages(PetEngine &e) {
  struct Checked : Canvas {
    void fill(int x, int y, int w, int h, uint32_t) override {
      if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > SCREEN || y + h > SCREEN) {
        printf("off-screen fill %d,%d %dx%d\n", x, y, w, h);
        abort();
      }
    }
  } cc;
  uint32_t r = 777, now = 900000;
  auto rnd = [&]() {
    r = r * 1103515245u + 12345u;
    return r >> 8;
  };
  for (int i = 0; i < 200000; i++) {
    now += 5 + rnd() % 60;
    e.loop(now);
    int x = (int) (rnd() % 400) - 20, y = (int) (rnd() % 400) - 20;
    switch (rnd() % 5) {
      case 0: e.touch_down(x, y, now); break;
      case 1: e.touch_move(x, y); break;
      case 2: e.touch_up(now); break;
      default: break;
    }
    if (e.in_game() || e.in_stand()) continue;  // the games have their own fuzz
    if (i % 7 == 0) e.render(cc, now);
  }
}

// Half-size thumbnails of the shots whose names start with `prefix` (all of them for "").
static void contact_sheet(const char *name, int cols, const char *prefix = "") {
  std::vector<const std::vector<uint32_t> *> pick;
  for (auto &s : shots)
    if (s.first.rfind(prefix, 0) == 0) pick.push_back(&s.second);
  int n = (int) pick.size(), rows = (n + cols - 1) / cols, cell = 180;
  std::vector<uint32_t> px(cols * cell * rows * cell, 0x202020);
  for (int i = 0; i < n; i++) {
    int ox = (i % cols) * cell, oy = (i / cols) * cell;
    for (int y = 0; y < cell; y++)
      for (int x = 0; x < cell; x++) px[(oy + y) * cols * cell + ox + x] = (*pick[i])[(y * 2) * SCREEN + x * 2];
  }
  write_png(out_dir + name + ".png", cols * cell, rows * cell, px);
}

// A person who plays at set times of day: during a session they touch the screen every 20 s (so the pet is
// "with someone"), and once a minute look after it. Days in [away_from, away_to] they don't come at all.
struct Session {
  int start_min, len_min;
};
static void simulate(const char *label, std::vector<Session> sessions, int days, int away_from = -1,
                     int away_to = -1) {
  PetEngine e;
  e.new_game(42);
  const uint32_t epoch0 = NOON_UTC - 6 * 3600;  // 06:00 local on day 0
  uint32_t t = 0, ms = 1000;                     // seconds since 00:00 local day 0 (from 06:00)
  t = 6 * 3600;
  e.set_clock(epoch0, 6 * 60);
  e.loop(ms);
  int died_at = -1, last_day = -1;
  uint32_t last_touch = 0, last_care = 0, last_play = 0;
  while (t < (uint32_t) days * 86400) {
    int day = (int) (t / 86400), mod = (int) ((t / 60) % 1440);
    bool away = day >= away_from && day <= away_to;
    bool in = false;
    for (auto &s : sessions)
      if (!away && mod >= s.start_min && mod < s.start_min + s.len_min) in = true;
    uint32_t step = in ? 1 : 60;
    t += step;
    ms += step * 1000;
    e.set_clock(epoch0 + t - 6 * 3600, mod);
    e.loop(ms);
    if (in) {
      if (t - last_touch >= 20) {
        tap(e, 300, 150, ms);
        last_touch = t;
      }
      if (t - last_care >= 60) {
        last_care = t;
        if (e.poops() || e.cleanliness() < 60) e.clean_up();
        if (e.is_sick()) e.heal();
        if (e.in_tantrum()) e.scold();
        for (int i = 0; i < 4 && !e.is_asleep() && e.food() < 70; i++) e.feed_meal();
        if (!e.is_asleep() && e.energy() < 25) e.toggle_light();  // a nap
      }
      if (t - last_play >= 300 && !e.is_asleep() && e.energy() > 30) {  // a game every 5 minutes
        last_play = t;
        e.state().happy = (uint8_t) std::min(100, e.happy() + 20);
        e.state().coins += 5;
        e.state().runs++;
      }
    }
    if (e.is_dead() && died_at < 0) died_at = (int) (t / 60);
    if ((day != last_day && mod >= 23 * 60 + 58) || (e.is_dead() && died_at == (int) (t / 60))) {
      last_day = day;
      printf("  %-16s day %2d %s gen %d %-6s %-20s %5.1fh together  food %3d happy %3d energy %3d clean %3d "
             "health %3d disc %3d wt %2d oops %d coins %3d %s\n",
             label, day, away ? "(away)" : "      ", e.generation(), e.stage_name(), e.species(), e.hours_together(),
             e.food(), e.happy(), e.energy(), e.cleanliness(), e.health(), e.discipline(), e.weight(),
             e.care_mistakes(), e.coins(), e.mood());
    }
    if (e.is_dead()) break;
  }
  printf("  %-16s => %s\n", label,
         died_at < 0 ? "alive" : (std::string("died after ") + std::to_string(died_at / 1440.0) + " days").c_str());
}

int main(int argc, char **argv) {
  if (argc > 1) out_dir = std::string(argv[1]) + "/";
  crc_init();
  PetEngine e;
  uint32_t now = 5000;

  prep(e, 7, STAGE_EGG);
  shot(e, "01-egg", now);

  prep(e, 7, STAGE_BABY);
  e.state().happy = 90;
  shot(e, "02-baby", now);

  prep(e, 7, STAGE_CHILD);
  shot(e, "03-child", now);

  prep(e, 7, STAGE_TEEN);
  shot(e, "04-teen", now);

  prep(e, 7, STAGE_ADULT);
  e.state().form = 1;
  e.update_names();
  shot(e, "05-adult-crown", now);

  prep(e, 7, STAGE_ELDER, 17 * 60 + 50);
  shot(e, "06-elder-dusk", now);

  prep(e, 11, STAGE_ADULT);
  e.state().sick = 1;
  e.state().poops = 3;
  e.state().food = 15;
  e.state().clean = 30;
  shot(e, "07-sick-poops", now);

  prep(e, 11, STAGE_ADULT);
  e.state().tantrum = 10;
  e.state().form = FORM_WILD;
  e.update_names();
  shot(e, "07b-tantrum", 5300);

  prep(e, 11, STAGE_ADULT, 23 * 60);
  e.state().asleep = 1;
  e.state().lights_on = 0;
  shot(e, "08-asleep-night", now);
  e.state().lights_on = 1;
  shot(e, "09-asleep-lights-on", now);

  prep(e, 23, STAGE_TEEN);
  int bx, by;
  button_xy(0, bx, by);
  now = 20000;
  tap(e, bx, by, now);
  shot(e, "10-feed-menu", now);
  tap(e, 122, 170, now);
  now += 700;
  e.loop(now);
  shot(e, "11-eating", now);

  now += 5000;
  e.loop(now);
  button_xy(1, bx, by);
  tap(e, bx, by, now);
  shot(e, "12a-games-menu", now);
  tap(e, 124, 138, now);  // JUMP
  shot(e, "12-run-ready", now);
  int score = play_runner(e, now, 0.25f, true, 20000);
  printf("jump game (screens): score %d\n", score);
  now += 3000;
  e.loop(now);
  e.state().energy = 100;
  printf("catch (screens): score %d\n", play_other(e, now, GAME_CATCH, 0.8f, 60000, "12c-catch"));
  e.state().energy = 100;
  printf("pop (screens): score %d\n", play_other(e, now, GAME_POP, 0.8f, 40000, "12d-pop"));
  e.state().energy = 100;
  printf("copy (screens): score %d\n", play_other(e, now, GAME_COPY, 0.9f, 120000, "12e-copy"));
  e.state().energy = 100;
  printf("quiz (screens): score %d\n", play_other(e, now, GAME_QUIZ, 0.8f, 120000, "12f-quiz"));
  e.state().energy = 100;
  printf("facts (screens): score %d\n", play_other(e, now, GAME_FACTS, 0.8f, 120000, "12g-facts"));
  e.state().energy = 100;
  printf("fizz (screens): score %d\n", play_other(e, now, GAME_FIZZ, 0.85f, 180000, "12h-fizz"));
  e.state().energy = 100;
  printf("munch (screens): score %d\n", play_other(e, now, GAME_MUNCH, 0.85f, 180000, "12i-munch"));
  e.state().energy = 100;
  e.state().coins += 40;  // something to spend at the stand
  printf("share (screens): score %d\n", play_other(e, now, GAME_SHARE, 0.8f, 300000, "12j-share"));
  printf("smarts after the math games: %d, prizes owned: wizard %d trophy %d rocket %d owl %d\n", e.smarts(),
         e.owns(IT_WIZARD_HAT), e.owns(IT_TROPHY), e.owns(IT_ROCKET), e.owns(IT_OWL));
  for (int i = 0; i < 4; i++) {  // the result toast
    now += 400;
    e.loop(now);
  }

  // swipe to the stats page
  for (int i = 0; i < 6; i++) {
    now += 1500;
    e.loop(now);
  }
  e.state().parent_a = 13;  // as if it were the child of "Zubo"
  e.state().parent_b = 4;
  e.state().generation = 2;
  e.update_names();
  e.touch_down(300, 180, now);
  e.touch_move(120, 185);
  now += 200;
  e.touch_up(now);
  e.loop(now);
  shot(e, "16-stats", now);
  e.state().coins = 34;
  e.touch_down(300, 180, now);  // on to the shop
  e.touch_move(120, 185);
  now += 200;
  e.touch_up(now);
  e.loop(now);
  shot(e, "16a-shop", now);
  tap(e, 268, 68, now);   // HATS tab
  tap(e, 234, 152, now);  // pick the cap
  shot(e, "16a2-shop-hats-picked", now);
  tap(e, 234, 152, now);  // buy it
  e.state().owned |= 1u << IT_TROPHY;  // as if won
  e.give(IT_GOLD_APPLE);
  tap(e, 282, 68, now);    // PRIZE tab
  tap(e, 126, 152, now);   // a locked prize: how to win it
  shot(e, "16a3-shop-prizes", now);
  e.touch_down(300, 180, now);  // on to settings
  e.touch_move(120, 185);
  now += 200;
  e.touch_up(now);
  e.loop(now);
  shot(e, "16b-settings", now);
  e.touch_down(180, 268, now);  // hold New egg for 1 s, then let go
  for (int i = 0; i < 10; i++) {
    now += 100;
    e.loop(now);
  }
  shot(e, "16c-settings-holding", now);
  e.touch_up(now);
  e.loop(now);

  // Times tables: one more swipe left (or one right from the pet). The page opens counting up the table.
  auto swipe = [&](int x0, int y0, int x1, int y1) {
    e.touch_down(x0, y0, now);
    e.touch_move(x1, y1);
    now += 200;
    e.touch_up(now);
    e.loop(now);
  };
  auto ring_xy = [](int k, int &x, int &y) {
    x = 180 + (int) lroundf(142 * sinf(k * 3.14159265f / 6));
    y = 180 - (int) lroundf(142 * cosf(k * 3.14159265f / 6));
  };
  int rx, ry;
  now += 2000;  // let the settings page's "Hold to confirm" go
  e.loop(now);
  swipe(300, 180, 120, 185);
  now += 4 * 160 + 60;  // the page opens counting up (the 2 times table)
  e.loop(now);
  shot(e, "21a-tables-counting", now);
  swipe(180, 250, 180, 140);  // one 110 px drag up: five tables on, to 7, straight away
  shot(e, "21b-tables", now);
  ring_xy(8, rx, ry);
  tap(e, rx, ry, now);  // pick 7 x 8
  now += 1500;
  e.loop(now);
  shot(e, "21c-tables-7x8", now);
  swipe(180, 250, 180, 140);  // on to 12, keeping the picked x8
  ring_xy(12, rx, ry);
  tap(e, rx, ry, now);  // pick x12: the biggest picture
  now += 1500;
  e.loop(now);
  shot(e, "21d-tables-12x12", now);

  prep(e, 31, STAGE_ADULT);
  e.state().dead = 1;
  shot(e, "17-gone", now);

  // Toys and hats: a pet with a cap, the plant and the ball; one dozing with its teddy; one asleep in bed.
  prep(e, 7, STAGE_ADULT);
  e.state().coins = 500;
  for (int it : {IT_BALL, IT_TEDDY, IT_PLANT, IT_BED, IT_GLASSES}) e.buy(it);
  uint32_t t2 = 60000;
  tap(e, 300, 150, t2);  // someone's here
  t2 = 62000;
  e.loop(t2);  // (the ball comes out 0-5 s of every 20)
  shot(e, "18-glasses-ball-plant", t2 + 1500);
  e.buy(IT_PARTY_HAT);
  for (int i = 0; i < 70; i++) {  // 7 minutes without a touch: it dozes
    t2 += 6000;
    e.loop(t2);
  }
  shot(e, "19-dozing-teddy", t2);
  e.toggle_light();
  e.state().lights_on = 1;
  shot(e, "20-asleep-bed", t2);

  // Every hat from the prize stand pages, and the study shelf taking turns.
  prep(e, 7, STAGE_ADULT);
  for (int id = IT_PROP_CAP; id < IT_COUNT; id++) e.give(id);
  uint32_t t3 = 90000;
  tap(e, 300, 150, t3);
  int hats_shown = 0;
  for (int id = 0; id < IT_COUNT && hats_shown < 12; id++) {
    if (!is_hat(id)) continue;
    e.state().hat = (uint8_t) id;
    t3 += 6000;
    e.loop(t3);
    tap(e, 300, 150, t3);
    shot(e, (std::string("hat-") + std::to_string(id)).c_str(), t3);
    hats_shown++;
  }
  contact_sheet("hats", 4, "hat-");
  shots.erase(std::remove_if(shots.begin(), shots.end(), [](auto &s) { return s.first.rfind("hat-", 0) == 0; }),
              shots.end());

  contact_sheet("screens", 4);
  contact_sheet("tables", 4, "21");
  contact_sheet("games", 4, "12");
  contact_sheet("math", 4, "12f");
  {
    std::vector<std::pair<std::string, std::vector<uint32_t>>> keep;
    for (auto &s : shots)
      if (s.first.rfind("12f", 0) == 0 || s.first.rfind("12g", 0) == 0 || s.first.rfind("12h", 0) == 0 ||
          s.first.rfind("12i", 0) == 0 || s.first.rfind("12j", 0) == 0 || s.first.rfind("12a", 0) == 0 || s.first.rfind("16-stats", 0) == 0 ||
          s.first.rfind("16a3", 0) == 0)
        keep.push_back(s);
    std::swap(keep, shots);
    contact_sheet("math", 4);
    std::swap(keep, shots);
  }
  {  // shop and toys
    std::vector<std::pair<std::string, std::vector<uint32_t>>> keep;
    for (auto &s : shots)
      if (s.first.rfind("16a", 0) == 0 || s.first.rfind("18", 0) == 0 || s.first.rfind("19", 0) == 0 ||
          s.first.rfind("20", 0) == 0)
        keep.push_back(s);
    std::swap(keep, shots);
    contact_sheet("shop", 5);
    std::swap(keep, shots);
  }

  // Gallery of random pets at each stage
  shots.clear();
  for (int i = 0; i < 24; i++) {
    prep(e, 1000 + i * 7919, i < 4 ? STAGE_CHILD : (i < 8 ? STAGE_TEEN : STAGE_ADULT));
    e.state().form = i < 8 ? 0 : (uint8_t) ((i - 8) % 7);
    e.state().hat = (i == 13 || i == 20) ? (uint8_t) IT_WIZARD_HAT : (i == 17 ? (uint8_t) IT_CHEF_HAT : 0);
    e.update_names();
    Buffer b;
    e.render(b, 5000 + i * 300);
    b.mask();
    shots.push_back({e.species(), b.px});
    printf("gallery %2d: %-8s %s\n", i, e.name(), e.species());
  }
  contact_sheet("gallery", 6);

  // Is the jump game fair? Bots with different reaction leads, each stage size, a few runs each.
  printf("\nJump game, autoplay scores (5 runs each):\n");
  for (uint8_t st : {STAGE_BABY, STAGE_TEEN, STAGE_ADULT}) {
    for (float lead : {0.15f, 0.22f, 0.30f, 0.40f, -1.0f}) {
      if (lead < 0)
        printf("  %-6s sloppy    :", st == STAGE_BABY ? "baby" : st == STAGE_TEEN ? "teen" : "adult");
      else
        printf("  %-6s lead %.2fs:", st == STAGE_BABY ? "baby" : st == STAGE_TEEN ? "teen" : "adult", lead);
      for (int run = 0; run < 5; run++) {
        prep(e, 500 + run * 77, st);
        uint32_t t = 10000;
        e.loop(t);
        e.state().energy = 100;
        e.start_game(GAME_JUMP);
        printf(" %3d", play_runner(e, t, lead, false));
      }
      printf("\n");
    }
  }

  printf("\nOther games, autoplay scores (5 runs each) by bot skill:\n");
  static const char *const GAME_NAMES[GAME_COUNT] = {"jump", "catch", "pop", "copy", "quiz",
                                                     "facts", "fizz", "munch", "share"};
  for (int kind = GAME_CATCH; kind < GAME_COUNT; kind++)
    for (float skill : {0.6f, 0.85f, 0.97f}) {
      printf("  %-5s skill %.2f:", GAME_NAMES[kind], skill);
      for (int run = 0; run < 5; run++) {
        prep(e, 900 + run * 31, STAGE_TEEN);
        uint32_t t = 10000;
        e.loop(t);
        printf(" %3d", play_other(e, t, kind, skill, 180000, nullptr));
      }
      printf("\n");
    }

  prep(e, 4, STAGE_ADULT);
  fuzz(e);
  printf("\nfuzz: ok\n");
  prep(e, 5, STAGE_ADULT);
  fuzz_pages(e);
  printf("fuzz pages: ok\n");

  printf("\nRules, simulated:\n");
  simulate("30 min at 18:00", {{18 * 60, 30}}, 16);
  simulate("15 min 2x a day", {{8 * 60, 15}, {18 * 60, 15}}, 12);
  simulate("1 h a day", {{19 * 60, 60}}, 10);
  simulate("30 min, away 3d", {{18 * 60, 30}}, 10, 3, 5);
  simulate("abandoned day 1", {{18 * 60, 30}}, 12, 1, 99);
  return 0;
}
