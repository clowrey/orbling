// Orbling: the game and its drawing, in plain C++ with no ESPHome dependency.
// The ESPHome component (orbling.cpp) and the host simulator (sim/) both drive this.
#pragma once

#include <cstddef>
#include <cstdint>

namespace petcore {

static const int SCREEN = 360;
// Pages, left to right; a swipe moves one page and wraps around (so the times tables are one swipe right of the pet).
enum Page : uint8_t { PAGE_PET, PAGE_STATS, PAGE_SHOP, PAGE_SETTINGS, PAGE_TABLES, PAGE_COUNT };

// Everything is drawn through this one call. Colours are 0xRRGGBB.
class Canvas {
 public:
  virtual ~Canvas() = default;
  virtual void fill(int x, int y, int w, int h, uint32_t rgb) = 0;
};

enum Stage : uint8_t { STAGE_EGG, STAGE_BABY, STAGE_CHILD, STAGE_TEEN, STAGE_ADULT, STAGE_ELDER };

// The random "type" of a pet, rolled when the egg is laid. Features show up as it grows.
struct Genome {
  uint8_t shape;    // 0 round, 1 bean, 2 wide, 3 pear, 4 cube
  uint8_t color;    // body colour (palette index)
  uint8_t accent;   // pattern colour (palette index)
  uint8_t ears;     // 0 none, 1 cat, 2 bunny, 3 bear, 4 antenna, 5 horns, 6 sprout
  uint8_t eyes;     // 0 dot, 1 big, 2 wide
  uint8_t pattern;  // 0 none, 1 spots, 2 stripes, 3 belly
  uint8_t tail;     // 0 none, 1 curl, 2 spike, 3 fluff
  uint8_t feet;     // 0 stubby, 1 floaty (wavy ghost hem)
  uint8_t cheeks;   // blush
  uint8_t name_a, name_b, name_c;  // name syllables (name_c 0 = two syllables)
  uint8_t spots;    // spot layout seed
  uint8_t pad[3];
};

static const uint32_t SAVE_MAGIC = 0x50455434;     // "PET4"
static const uint32_t SAVE_MAGIC_V3 = 0x50455433;  // "PET3": before time-together growth and the shop
static const uint32_t SAVE_MAGIC_V2 = 0x50455432;  // "PET2": before discipline and family

// Shop items. Food is eaten when bought; toys and hats are kept (bit `id` in SaveData::owned).
enum ItemId : uint8_t {
  IT_CAKE, IT_STEAK, IT_JUICE, IT_CANDY,     // food
  IT_BALL, IT_TEDDY, IT_PLANT, IT_BED,       // toys
  IT_BOW, IT_CAP, IT_GLASSES, IT_PARTY_HAT,  // hats
  IT_WIZARD_HAT, IT_TROPHY, IT_ROCKET, IT_OWL,  // prizes: only won in the math games
  IT_CHEF_HAT, IT_GOLD_APPLE,                   // Munch prizes (bits 0-1 of SaveData::owned2)
  // The prize stand: only for sale at the end of a math game, one page of six per math game, one buy per game.
  IT_PROP_CAP, IT_STAR_HAT, IT_ABACUS, IT_GLOBE, IT_BOOKS, IT_BRAIN_SNACK,           // Quiz: classroom
  IT_HEADPHONES, IT_GOGGLES, IT_ROBOT, IT_CLOCK, IT_ENERGY_BAR, IT_MILK,              // Facts: gadgets
  IT_SNORKEL, IT_SAILOR_HAT, IT_FISHBOWL, IT_SHELL, IT_SODA, IT_SUSHI,                // Fizz: ocean
  IT_BERET, IT_FRUIT_HAT, IT_COOKIE_JAR, IT_TEAPOT, IT_DONUT, IT_PIZZA,               // Munch: bakery
  IT_SPACE_HELMET, IT_ALIEN_BAND, IT_TELESCOPE, IT_PLANET, IT_STAR_COOKIE, IT_ASTRO,  // Share: space
  IT_COUNT
};
static const int PRIZE_COUNT = IT_GOLD_APPLE - IT_WIZARD_HAT + 1;
static const int STAND_PAGE = 6;   // items on each math game's page of the prize stand
static const int STAND_PAGES = 5;  // Quiz, Facts, Fizz, Munch, Share
static_assert(IT_COUNT == IT_PROP_CAP + STAND_PAGE * STAND_PAGES, "stand pages");
static_assert(IT_COUNT <= 48, "SaveData::owned, owned2 and owned3 hold 48 items");
// Hats drawn into the pet's sprite (shaped to its head) and hats drawn over it from their shop art.
inline bool is_sprite_hat(int id) {
  return (id >= IT_BOW && id <= IT_PARTY_HAT) || id == IT_WIZARD_HAT || id == IT_CHEF_HAT || id == IT_PROP_CAP ||
         id == IT_STAR_HAT;
}
inline bool is_overlay_hat(int id) {
  return id == IT_HEADPHONES || id == IT_GOGGLES || id == IT_SNORKEL || id == IT_SAILOR_HAT || id == IT_BERET ||
         id == IT_FRUIT_HAT || id == IT_SPACE_HELMET || id == IT_ALIEN_BAND;
}
inline bool is_hat(int id) { return is_sprite_hat(id) || is_overlay_hat(id); }
inline bool is_treat(int id) {
  return id == IT_BRAIN_SNACK || id == IT_ENERGY_BAR || id == IT_MILK || id == IT_SODA || id == IT_SUSHI ||
         id == IT_DONUT || id == IT_PIZZA || id == IT_STAR_COOKIE || id == IT_ASTRO;
}
inline bool is_prize(int id) { return id >= IT_WIZARD_HAT && id <= IT_GOLD_APPLE; }  // won, never bought
inline bool is_stand(int id) { return id >= IT_PROP_CAP && id < IT_COUNT; }          // bought after math games
inline bool is_food(int id) { return id < IT_BALL || is_treat(id); }               // eaten, not kept
// Things for the study shelf (on the right of the room).
inline bool is_decor(int id) { return is_stand(id) && !is_hat(id) && !is_treat(id); }

// Adult forms, decided by how it was raised.
enum Form : uint8_t { FORM_PLAIN, FORM_ROYAL, FORM_SCRUFFY, FORM_SPORTY, FORM_CHUBBY, FORM_WILD, FORM_BRAINY };

// Everything that survives a reboot. Stats are 0..100.
struct SaveData {
  uint32_t magic;
  Genome g;
  uint32_t age_min;     // minutes since the egg was laid
  uint32_t last_epoch;  // UTC time of the last simulated minute (0 = unknown)
  uint16_t generation;
  uint16_t poop_timer;  // minutes until the next poop (0 = none due)
  uint8_t food, happy, energy, clean, health, weight;
  uint8_t poops, sick, asleep, lights_on, stage, form;  // form: see Form
  uint8_t dead, care_mistakes, attention_min, snacks_hour;
  uint16_t best_run, runs;  // jump game: best score, games played (v2)
  uint8_t discipline;       // 0..100 (v3)
  uint8_t tantrum;          // minutes left of a tantrum, 0 = none (v3)
  uint8_t parent_a, parent_b, parent_c;  // parent's name syllables + 1 (0 = no parent) (v3)
  uint8_t settings;    // bit 0: sound off (zero = defaults, so older saves get them)
  uint8_t brightness;  // 0 = default (80%), else 1..4 = 30/55/80/100%
  uint8_t pad2;
  uint16_t best_catch, best_pop, best_copy;  // other games' best scores (v4)
  uint16_t pad3;
  uint32_t together_s;  // growth clock: seconds with someone around, plus 1/20 of the rest (v5)
  uint16_t coins;       // (v5)
  uint16_t owned;       // bit per ItemId (v5)
  uint8_t hat;          // 0 none, else ItemId of the hat worn (v5)
  uint8_t pad4[3];
  uint16_t best_quiz, best_facts;  // the times-tables games' best scores (v6)
  uint16_t best_fizz, best_munch;  // (v7)
  uint16_t smarts_x10;             // intelligence, 0..1000 (shown /10); grows with right math answers (v7)
  uint16_t owned2;  // more owned items: bit (id - 16) for ItemId 16 and up (v7; was padding, so zero before)
  uint16_t best_share;  // the division game (v8)
  uint16_t owned3;      // owned items 32-47: bit (id - 32) (v8; was padding, so zero before)
  uint16_t chant_medals;  // Chant: bit (n - 1) once the n times table has been chanted all through (v9)
  uint16_t best_chant;    // Chant: most tables finished in one game (v9)
};

enum GameKind : uint8_t {
  GAME_JUMP, GAME_CATCH, GAME_POP, GAME_COPY, GAME_QUIZ, GAME_FACTS, GAME_FIZZ, GAME_MUNCH, GAME_SHARE, GAME_CHANT,
  GAME_COUNT
};
inline bool is_math_game(int kind) { return kind >= GAME_QUIZ && kind < GAME_COUNT; }
// Quiz-style games: ten questions, four answers to tap.
inline bool is_quiz_like(int kind) { return kind == GAME_QUIZ || kind == GAME_SHARE; }

// Older saves are shorter; fields are only ever appended, so an old save is a prefix of a new one.
static const size_t SAVE_V1_SIZE = 48;  // before the jump game
static const size_t SAVE_V2_SIZE = 52;  // before discipline and family
static const size_t SAVE_V3_SIZE = 60;  // before the game menu's best scores
static const size_t SAVE_V4_SIZE = 68;  // before time-together growth and the shop
static const size_t SAVE_V5_SIZE = 80;  // before the times-tables games
static const size_t SAVE_V6_SIZE = 84;  // before smarts, Fizz and Munch
static const size_t SAVE_V7_SIZE = 92;  // before the division game
static const size_t SAVE_V8_SIZE = 96;  // before Chant
static_assert(offsetof(SaveData, best_run) == SAVE_V1_SIZE, "v1 fields must stay where they were");
static_assert(offsetof(SaveData, discipline) == SAVE_V2_SIZE, "v2 fields must stay where they were");
static_assert(offsetof(SaveData, best_catch) == SAVE_V3_SIZE, "v3 fields must stay where they were");
static_assert(offsetof(SaveData, together_s) == SAVE_V4_SIZE, "v4 fields must stay where they were");
static_assert(offsetof(SaveData, best_quiz) == SAVE_V5_SIZE, "v5 fields must stay where they were");
static_assert(offsetof(SaveData, best_fizz) == SAVE_V6_SIZE, "v6 fields must stay where they were");
static_assert(offsetof(SaveData, best_share) == SAVE_V7_SIZE, "v7 fields must stay where they were");
static_assert(offsetof(SaveData, chant_medals) == SAVE_V8_SIZE, "v8 fields must stay where they were");
static_assert(sizeof(SaveData) == 100, "save size");

// Growth by time together (seconds of SaveData::together_s) at which each stage starts, and when an elder
// sets off and leaves an egg.
static const uint32_t STAGE_TOGETHER_S[6] = {0, 60, 20 * 60, 60 * 60, 3 * 3600, 8 * 3600};
static const uint32_t RETIRE_TOGETHER_S = 12 * 3600;

class PetEngine {
 public:
  // Things in a game: obstacles, falling fruit, bubbles.
  enum ObstacleKind : uint8_t { OB_ROCK, OB_MUSHROOM, OB_BUSH, OB_BIRD, OB_STAR, OB_APPLE, OB_CAKE, OB_BUBBLE, OB_SPIKY };
  struct Obstacle {
    float x, y;  // jump game: left edge (y unused); catch: top-left; pop: centre
    ObstacleKind kind;
    bool active, passed;  // passed: jump = scored; pop = popped
    uint32_t t;           // pop: when it appeared / was popped
    uint16_t life_ms;     // pop: how long it stays
    uint16_t value;       // fizz: the number on the bubble; munch: the fact on the food, a*16+b
  };

  // Start a fresh game (no save) or restore one. seed should be truly random (esp_random()).
  void new_game(uint32_t seed);
  bool restore(const SaveData &data, uint32_t seed);
  const SaveData &save_data() const { return s_; }
  SaveData &state() { return s_; }  // for the simulator and debugging
  bool take_dirty() {
    bool d = dirty_;
    dirty_ = false;
    return d;
  }

  // Wall clock. local_minute_of_day = hour*60+minute in local time. Call often; without it the
  // pet keeps time from millis and never sleeps by the clock.
  void set_clock(uint32_t utc_epoch, int local_minute_of_day);

  void loop(uint32_t now_ms);
  void render(Canvas &c, uint32_t now_ms);
  // Frames per second worth drawing right now (sleeping in the dark needs few).
  int wanted_fps() const;

  // Chant (times tables by heart): voice input. The ESPHome config streams the microphone to Home Assistant's
  // speech-to-text while wants_listening() and passes the text it hears to voice_heard().
  bool wants_listening() const;
  void set_listening(bool on) { listening_ = on; }
  void set_thinking(bool on) { thinking_ = on; }  // speech ended; waiting for the text to come back
  void set_voice_link(bool up) { voice_link_ = up; }  // connected to Home Assistant, so voice can work
  void voice_heard(const char *text);
  // Changes whenever Chant moves on (a table picked or finished): what was said before no longer counts.
  uint8_t voice_epoch() const { return ch_epoch_; }
  void voice_error(const char *code);
  bool voice_enabled() const { return !(s_.settings & 2); }
  void set_voice_enabled(bool on);
  int chant_medals() const;  // tables with a medal, 0..12
  // The numbers in a piece of text, as digits ("7, 14") or words ("seven fourteen", "a hundred and eight").
  static int parse_numbers(const char *text, int *out, int max);
  float backlight(uint32_t now_ms) const;

  void touch_down(int x, int y, uint32_t now_ms);
  void touch_move(int x, int y);
  void touch_up(uint32_t now_ms);

  // Actions (also used by Home Assistant buttons). Return false when the pet refuses.
  bool feed_meal();
  bool feed_snack();
  bool open_games();          // the Play button: the game menu
  bool start_game(int kind);  // GameKind
  bool clean_up();
  bool heal();
  void toggle_light();
  bool scold();
  // A new random egg (inherit=false), or one carrying about half of the current pet's traits.
  void hatch_new_egg(uint32_t seed, bool inherit = false);
  void pat();
  bool buy(int item);   // ItemId; for an owned hat, puts it on or takes it off
  void update_names();  // after changing the genome, stage or form directly

  // State for Home Assistant
  int food() const { return s_.food; }
  int happy() const { return s_.happy; }
  int energy() const { return s_.energy; }
  int cleanliness() const { return s_.clean; }
  int health() const { return s_.health; }
  int weight() const { return s_.weight; }
  float hours_together() const { return s_.together_s / 3600.0f; }
  int grows_in_min() const;  // time together until the next stage (or the egg); -1 if none
  int coins() const { return s_.coins; }
  bool owns(int item) const {
    return item < 16 ? (s_.owned >> item) & 1 : item < 32 ? (s_.owned2 >> (item - 16)) & 1 : (s_.owned3 >> (item - 32)) & 1;
  }
  void give(int item) {
    if (item < 16)
      s_.owned = (uint16_t) (s_.owned | (1u << item));
    else if (item < 32)
      s_.owned2 = (uint16_t) (s_.owned2 | (1u << (item - 16)));
    else
      s_.owned3 = (uint16_t) (s_.owned3 | (1u << (item - 32)));
  }
  // Someone touched it in the last 5 minutes: needs run fast. Otherwise it dozes and they run slow.
  bool is_active(uint32_t now_ms) const;
  bool is_dozing(uint32_t now_ms) const;
  int care_mistakes() const { return s_.care_mistakes; }
  int discipline() const { return s_.discipline; }
  int smarts() const { return s_.smarts_x10 / 10; }  // intelligence 0..100
  bool in_tantrum() const { return s_.tantrum > 0; }
  const char *parent_name() const { return parent_; }  // "" for a first-generation pet
  // Settings (also on the settings page, the third page)
  bool sound_enabled() const { return !(s_.settings & 1); }
  void set_sound_enabled(bool on);
  int brightness_level() const { return s_.brightness ? s_.brightness - 1 : 2; }  // 0..3
  void set_brightness_level(int level);
  // An RTTTL tune to play for something that just happened, once; nullptr if none.
  const char *take_sound() {
    const char *s = sound_;
    sound_ = nullptr;
    return s;
  }
  int generation() const { return s_.generation; }
  int poops() const { return s_.poops; }
  bool is_sick() const { return s_.sick; }
  bool is_asleep() const { return s_.asleep; }
  bool is_dead() const { return s_.dead; }
  bool lights_on() const { return s_.lights_on; }
  bool needs_attention() const;
  const char *name() const { return name_; }
  const char *species() const { return species_; }
  const char *stage_name() const;
  const char *mood() const;
  const char *toast() const { return toast_; }
  int best_run() const { return s_.best_run; }
  int best_score(int kind) const;
  int game_kind() const { return game_kind_; }
  int lives() const { return lives_; }
  // Simulator autoplay helpers.
  bool copy_waiting(int *next_pad) const {
    if (overlay_ != OV_GAME || game_kind_ != GAME_COPY || copy_state_ != COPY_INPUT) return false;
    *next_pad = seq_[copy_idx_];
    return true;
  }
  const Obstacle *objects(int *n) const {
    *n = 8;
    return obs_;
  }
  bool in_game() const { return overlay_ == OV_GAME; }
  bool in_stand() const { return overlay_ == OV_STAND; }
  // Times-tables games: waiting for an answer to "a x b"; the right answer; the quiz's four choices.
  bool math_waiting() const {
    return overlay_ == OV_GAME && is_math_game(game_kind_) && run_phase_ == RUN_GO && feedback_ == 0;
  }
  int munch_target() const { return munch_target_; }
  // Facts keypad: centre and width of a key (0-9 digits, 10 delete, 11 skip).
  static void facts_key_pos(int key, int &x, int &y, int &w);
  int math_answer() const { return mans_; }
  int quiz_choice(int i) const { return choices_[i & 3]; }
  // Simulator autoplay: distance from the pet to the next obstacle (-1 if none), and whether it flies.
  float next_obstacle(bool *flying) const;
  float run_speed() const { return run_speed_; }
  int run_score() const { return run_score_; }
  bool run_over() const { return run_phase_ == RUN_OVER; }

 private:
  enum Activity : uint8_t { ACT_IDLE, ACT_EAT, ACT_HAPPY, ACT_REFUSE, ACT_CLEAN, ACT_HATCH, ACT_SULK };
  enum Overlay : uint8_t { OV_NONE, OV_FEED, OV_GAME, OV_GAMES, OV_STAND };
  enum RunPhase : uint8_t { RUN_READY, RUN_GO, RUN_OVER };
  enum CopyState : uint8_t { COPY_SHOW, COPY_INPUT, COPY_PAUSE };

  void tick_minute(int minute_of_day, bool active);
  void check_growth();
  void greet();
  void draw_shop(Canvas &c, uint32_t now);
  void shop_tap(int x, int y);
  void evolve_to(uint8_t stage);
  void die();
  void retire();
  void sound(const char *rtttl, bool even_asleep = false);
  bool disobeys(const char *what);
  void set_activity(Activity a, uint32_t dur_ms);
  void show_toast(const char *msg, uint32_t dur_ms = 2500);
  void tap(int x, int y);
  void long_press(int x, int y);
  void finish_game();
  void game_press(int x, int y);
  void game_over(const char *tune);
  void run_update(float dt);
  void run_spawn();
  void catch_update(float dt);
  void pop_update();
  void copy_update();
  void copy_add_step();
  void math_new_question();
  void math_answered(bool right, uint32_t show_ms);
  void math_update();
  void draw_quiz(Canvas &c, uint32_t now);
  void draw_facts(Canvas &c, uint32_t now);
  void learn();  // a right math answer: smarts go up
  void catch_move(float dt);
  void fizz_spawn();
  void fizz_update(float dt);
  void munch_new_target();
  void munch_update(float dt);
  void draw_fizz(Canvas &c, uint32_t now);
  void draw_munch(Canvas &c, uint32_t now);
  void draw_stand(Canvas &c, uint32_t now);
  void stand_tap(int x, int y);
  void draw_overlay_hat(Canvas &c, int ox, int oy, int sc);
  void next_hat();  // hold the pet: the next hat it owns (or none)
  void draw_game_hud(Canvas &c, uint32_t now, const char *title, const char *hint1, const char *hint2);
  void draw_catch(Canvas &c, uint32_t now);
  void draw_pop(Canvas &c, uint32_t now);
  void draw_copy(Canvas &c, uint32_t now);
  void draw_games_menu(Canvas &c);
  uint32_t rnd();
  int minute_of_day(uint32_t epoch) const;
  bool is_night(int mod) const;
  int base_weight(uint8_t stage) const;

  // drawing
  void rect(Canvas &c, int x, int y, int w, int h, uint32_t rgb);
  void circle(Canvas &c, int cx, int cy, int r, uint32_t rgb);
  void pix_circle(Canvas &c, int cx, int cy, float r, int scale, uint32_t rgb);
  void icon(Canvas &c, const char *const *rows, int nrows, int x, int y, int scale, int keep_cols = 99, bool grey = false);
  void text(Canvas &c, int x, int y, const char *s, int scale, uint32_t rgb, bool center = true, bool shadow = true);
  void bar(Canvas &c, int x, int y, int w, int h, int value);
  void draw_scene(Canvas &c, uint32_t now);
  void draw_sky(Canvas &c, uint32_t now);
  void draw_pet(Canvas &c, uint32_t now, int cx, int ground_y);
  void draw_hud(Canvas &c);
  void draw_buttons(Canvas &c, bool enabled);
  void draw_feed_menu(Canvas &c);
  void draw_runner(Canvas &c, uint32_t now);
  void draw_stats(Canvas &c);
  void draw_settings(Canvas &c, uint32_t now);
  void settings_tap(int x, int y);
  float egg_hold_progress(uint32_t now) const;  // 0..1 while the New egg button is held
  void draw_page_dots(Canvas &c);
  // Times-table page: the table as a clock face, a count-up sweep, and a dot picture of a picked fact.
  void draw_tables(Canvas &c, uint32_t now);
  void tables_tap(int x, int y);
  void tables_step(int steps);  // +n: n tables on, -n: n back (wraps 12 -> 1)
  void tables_start_sweep();
  bool tables_animating(uint32_t now) const;
  // Chant: pick a table, then tap (or say) its answers in order round a circle; a gift for each table done.
  void chant_press(int x, int y);
  void chant_begin(int table);
  void chant_hit(int k);  // the k-th answer (1..12) came, tapped or said
  void chant_complete();
  void chant_update();
  void draw_chant(Canvas &c, uint32_t now);
  void draw_mic_status(Canvas &c, uint32_t now, int y);
  void food_effect(int id);  // what eating a food item does (bought, or a Chant gift)

  SaveData s_{};
  bool dirty_{false};
  uint32_t rng_{1};
  char name_[16]{};
  char species_[32]{};
  char toast_[40]{};
  char parent_[16]{};
  const char *sound_{nullptr};
  uint32_t toast_until_{0};

  bool clock_valid_{false};
  uint32_t epoch_{0};
  int tz_off_min_{0};
  uint32_t sleep_acc_ms_{0};
  float pet_acc_ms_{0};       // pet time owed to tick_minute
  float together_acc_ms_{0};  // growth time owed to together_s
  bool caught_up_{false};     // offline time made up once the clock is known
  uint32_t last_call_{0};     // last attention beep
  bool was_needy_{false};
  bool wake_greet_{false};    // this touch woke it from a doze: greet, don't act on it
  uint8_t shop_tab_{0};
  int8_t shop_sel_{-1};  // also the prize stand's picked item
  uint8_t stand_page_{0};    // which math game's page the stand shows
  bool stand_bought_{false};  // one buy per game played
  int8_t stand_got_{-1};      // what was bought this time
  uint32_t shop_sel_t_{0};
  const char *const *eat_icon_{nullptr};
  uint32_t now_{0};

  uint8_t page_{0};
  // times-table page
  uint8_t tt_table_{2};       // 1..12
  uint8_t tt_sel_{0};         // picked fact, table x tt_sel_ (1..12); 0 none
  uint32_t tt_sweep_ms_{0};   // when the count-up sweep started; 0 none
  uint32_t tt_sel_ms_{0};     // when an answer was tapped (its dots appear row by row)
  int16_t tt_drag_y_{0};      // a drag up or down runs through the tables: y of the last step
  int8_t tt_lock_{0};         // this touch: 0 undecided, 1 up/down (tables), -1 sideways (pages)
  bool tt_dragged_{false};    // this touch changed the table, so it is not a tap
  // Chant
  uint8_t ch_state_{0};       // 0 pick a table, 1 chanting, 2 the gift
  uint8_t ch_table_{1};
  uint8_t ch_next_{0};        // answers done so far, 0..12
  uint8_t ch_slot_[12]{};     // clock position (1..12) of each answer: in order at first, shuffled once learnt
  int8_t ch_wrong_{-1};       // the answer that was tapped out of turn (it wiggles)
  int8_t ch_gift_item_{-1};   // a treat in the gift, or -1 for the table's medal
  uint8_t ch_epoch_{0};
  bool ch_hard_{false};       // Chant hard: the answers are shuffled round the circle
  bool ch_champion_{false};   // this gift completed all twelve medals
  uint8_t ch_gift_coins_{0};
  uint32_t ch_wrong_ms_{0}, ch_ok_ms_{0}, ch_gift_ms_{0}, ch_heard_ms_{0}, ch_err_until_{0};
  bool listening_{false}, thinking_{false}, voice_link_{false};
  char ch_heard_[44]{};       // what speech-to-text last heard (or a voice error), shown small
  Overlay overlay_{OV_NONE};
  uint32_t overlay_since_{0};
  Activity act_{ACT_IDLE};
  uint32_t act_start_{0}, act_until_{0};
  uint8_t clean_poops_{0};

  // jump game
  RunPhase run_phase_{RUN_READY};
  uint32_t run_t_{0}, run_jump_ms_{0};
  float run_y_{0}, run_vy_{0};  // height above the ground and upward speed, px and px/s
  float run_speed_{0}, run_scroll_{0}, run_next_{0};
  uint16_t run_score_{0};
  uint8_t run_stars_{0};
  bool run_new_best_{false};
  bool game_touch_{false};  // this touch went to the game; don't treat its release as a tap
  Obstacle obs_[8]{};
  uint8_t game_kind_{GAME_JUMP};
  uint8_t lives_{3};
  float catch_x_{180}, catch_target_{180};
  uint32_t next_spawn_ms_{0}, game_end_ms_{0};
  // copy game
  uint8_t seq_[40]{};
  uint8_t copy_len_{0}, copy_idx_{0};
  CopyState copy_state_{COPY_SHOW};
  uint32_t copy_t_{0};
  int8_t lit_pad_{-1};
  uint32_t lit_until_{0};
  // times-tables games
  uint8_t ma_{0}, mb_{0};  // the question: ma_ x mb_
  uint16_t mans_{0};       // its answer
  uint16_t choices_[4]{};  // quiz: four answers to pick from
  uint8_t q_num_{0};       // quiz: question number (0..9)
  uint16_t entry_{0};      // facts: what has been typed
  uint8_t entry_len_{0};
  uint8_t feedback_{0};  // 0 waiting for an answer, 1 right, 2 wrong (the right answer is shown)
  int8_t picked_{-1};    // quiz: the choice tapped
  bool missing_{false};  // fizz: "? x b = p" (answer a) instead of "a x b = ?"
  uint16_t munch_target_{0};
  uint8_t munch_got_{0};  // right facts eaten for this target
  uint32_t goal_flash_until_{0};  // munch: the "New goal!" banner is up (and the food waits) until then
  uint32_t q_t_{0}, fb_until_{0};

  // walking
  float walk_x_{0}, walk_target_{0};
  int8_t facing_{1};
  uint32_t walk_next_{0}, last_loop_{0};

  // touch
  bool pressed_{false}, long_fired_{false};
  int tx0_{0}, ty0_{0}, tx_{0}, ty_{0};
  uint32_t press_ms_{0}, last_user_{0};
  uint32_t last_pat_{0};
};

}  // namespace petcore
