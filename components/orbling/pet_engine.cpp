#include "pet_engine.h"
#include "pet_art.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace petcore {

// ---------------------------------------------------------------------------------------------
// Tables

static const uint32_t BODY_COLORS[12] = {0xF7A1C4, 0xFFB37A, 0xFFE066, 0x8EE3B0, 0x7CC8FF, 0xB9A3FF,
                                         0xFF7A7A, 0xC4F06A, 0x4FD1C5, 0xF5E6C8, 0xB8C0CC, 0xC77DFF};
static const char *const SYLLABLES[20] = {"mo", "chi", "pi", "bo", "ku", "ri", "na", "to", "lu", "ma",
                                          "po", "ki", "zu", "fi", "ne", "ta", "yu", "ro", "bu", "mi"};
static const char *const SHAPE_NAMES[5] = {"Blob", "Bean", "Pudding", "Pear", "Cube"};
static const char *const EAR_NAMES[7] = {"Plain", "Kitty", "Bunny", "Bear", "Buzzy", "Horned", "Sprout"};
static const char *const STAGE_NAMES[6] = {"Egg", "Baby", "Child", "Teen", "Adult", "Elder"};

// Time runs at two speeds. With someone around (a touch in the last 5 minutes) one real minute is 15 pet
// minutes: hungry in about 40 minutes, tired in about an hour. Left alone it dozes at 0.5 pet minutes per real
// minute, 30x slower: food lasts about 20 hours. Growth (SaveData::together_s) counts time with you fully and
// dozing time at 1/20.
static const uint32_t ACTIVE_WINDOW_MS = 5 * 60 * 1000;
static const float SPEED_ACTIVE = 15.0f;
static const float SPEED_DOZE = 0.5f;
static const float DOZE_GROWTH = 1.0f / 20.0f;
static const uint32_t MAX_OFFLINE_S = 3 * 24 * 3600;  // after a power cut, catch up at most 3 days of dozing

// The shop.
struct ShopItem {
  const char *name;
  uint16_t price;
  const char *const *icon;
  uint8_t rows;
};
static const ShopItem SHOP[IT_COUNT] = {
    {"Cake", 12, art::CAKE, 12},     {"Steak", 8, art::STEAK, 12},       {"Juice", 10, art::JUICE, 12},
    {"Candy", 4, art::SNACK, 12},    {"Ball", 25, art::PLAY, 12},        {"Teddy", 30, art::TEDDY, 12},
    {"Plant", 20, art::PLANT, 12},   {"Bed", 40, art::BED, 12},          {"Bow", 15, art::BOW, 12},
    {"Cap", 20, art::CAP, 12},       {"Specs", 20, art::GLASSES, 12},    {"Party", 25, art::PARTY_HAT, 12},
    // prizes: never for sale, only won in the math games
    {"Wizard", 0, art::WIZARD_HAT, 12}, {"Trophy", 0, art::TROPHY, 12}, {"Rocket", 0, art::ROCKET, 12},
    {"Owl", 0, art::OWL, 12},           {"Chef", 0, art::CHEF_HAT, 12}, {"Apple", 0, art::GOLD_APPLE, 12},
    // the prize stand, only open at the end of a math game
    {"Prop", 15, art::PROP_CAP, 12},    {"Star", 20, art::STAR_HAT, 12},   {"Abacus", 12, art::ABACUS, 12},
    {"Globe", 18, art::GLOBE, 12},      {"Books", 10, art::BOOKS, 12},     {"Snack", 5, art::BRAIN_SNACK, 12},
    {"Phones", 15, art::HEADPHONES, 12}, {"Goggle", 15, art::GOGGLES, 12},  {"Robot", 20, art::ROBOT, 12},
    {"Clock", 12, art::CLOCK, 12},       {"Energy", 6, art::ENERGY_BAR, 12}, {"Milk", 5, art::MILK, 12},
    {"Mask", 15, art::SNORKEL, 12},      {"Sailor", 18, art::SAILOR_HAT, 12}, {"Fish", 20, art::FISHBOWL, 12},
    {"Shell", 10, art::SHELL, 12},       {"Soda", 5, art::SODA, 12},         {"Sushi", 6, art::SUSHI, 12},
    {"Beret", 15, art::BERET, 12},       {"Fruity", 18, art::FRUIT_HAT, 12}, {"Jar", 12, art::COOKIE_JAR, 12},
    {"Teapot", 14, art::TEAPOT, 12},     {"Donut", 5, art::DONUT, 12},       {"Pizza", 7, art::PIZZA, 12},
    {"Helmet", 20, art::SPACE_HELMET, 12}, {"Alien", 15, art::ALIEN_BAND, 12}, {"Scope", 20, art::TELESCOPE, 12},
    {"Planet", 16, art::PLANET, 12},     {"Cookie", 5, art::STAR_COOKIE, 12}, {"Astro", 6, art::ASTRO, 12},
    // the shop's second row: food, toys, hats, and the ROOM tab
    {"Cone", 6, art::ICE_CREAM, 12},     {"Burger", 9, art::BURGER, 12},
    {"Duck", 25, art::DUCK, 12},         {"Light", 30, art::NIGHT_LIGHT, 12},
    {"Cowboy", 20, art::COWBOY_HAT, 12}, {"Flower", 18, art::FLOWER_CROWN, 12},
    {"Lava", 22, art::LAVA_LAMP, 12},    {"Art", 30, art::PAINTING, 12},       {"Cactus", 15, art::CACTUS, 12},
    {"Snow", 20, art::SNOW_GLOBE, 12},   {"Radio", 25, art::RADIO, 12},        {"Dino", 18, art::DINO, 12},
};
// The prize stand page for each math game, in GameKind order from GAME_QUIZ.
static const char *const STAND_TITLE[STAND_PAGES] = {"CLASSROOM", "GADGETS", "OCEAN", "BAKERY", "SPACE"};
static const uint32_t STAND_BOARD[STAND_PAGES] = {0x234536, 0x2E2350, 0x1E3A5A, 0x4A2E22, 0x14183A};
static const int SHOP_TAB_COUNT = 5, SHOP_PRIZE_TAB = 4;
static const char *const SHOP_TABS[SHOP_TAB_COUNT] = {"FOOD", "TOYS", "HATS", "ROOM", "PRIZE"};
// Six things on each tab.
static const uint8_t SHOP_TAB_ITEMS[SHOP_TAB_COUNT][6] = {
    {IT_CAKE, IT_STEAK, IT_JUICE, IT_CANDY, IT_ICE_CREAM, IT_BURGER},
    {IT_BALL, IT_TEDDY, IT_PLANT, IT_BED, IT_DUCK, IT_NIGHT_LIGHT},
    {IT_BOW, IT_CAP, IT_GLASSES, IT_PARTY_HAT, IT_COWBOY_HAT, IT_FLOWER_CROWN},
    {IT_LAVA_LAMP, IT_PAINTING, IT_CACTUS, IT_SNOW_GLOBE, IT_RADIO, IT_DINO},
    {IT_WIZARD_HAT, IT_TROPHY, IT_ROCKET, IT_OWL, IT_CHEF_HAT, IT_GOLD_APPLE},
};
// How each prize is won (shown on the PRIZE tab).
static const char *const PRIZE_HOW[PRIZE_COUNT] = {"10/10 in Quiz", "20 in Facts", "15 in Fizz",
                                                   "Smarts 60",     "15 in Munch", "25 in Munch"};

// Tunes (RTTTL) for the 3.5 mm jack.
static const char *const SND_CALL = "call:d=16,o=6,b=180:c,p,c,p,c";
static const char *const SND_HATCH = "hatch:d=16,o=6,b=180:c,e,g,c7,p,g,c7";
static const char *const SND_GROW = "grow:d=8,o=6,b=160:c,e,g,c7,p,g,c7";
static const char *const SND_EAT = "eat:d=32,o=6,b=200:c,p,c,p,c";
static const char *const SND_HAPPY = "happy:d=16,o=6,b=200:c,e,g,c7";
static const char *const SND_JUMP = "jump:d=32,o=6,b=250:c,g";
static const char *const SND_STAR = "star:d=32,o=7,b=250:e,g,c8";
static const char *const SND_CRASH = "crash:d=16,o=5,b=180:g,e,c";
static const char *const SND_SCOLD = "scold:d=8,o=5,b=160:c,p,c";
static const char *const SND_BYE = "bye:d=8,o=5,b=100:g,e,c,p,c6";

static const int GROUND_Y = 238;
static const int SPRITE_SCALE = 5;
static const int BTN_R = 24;

// Jump game tuning (px, px/s, px/s^2). A tap rises about 95 px, a held jump about 150 px.
static const float RUN_PET_X = 110;
static const float RUN_START_SPEED = 210;
static const float RUN_MAX_SPEED = 480;
static const float RUN_JUMP_V = 600;
static const float RUN_GRAVITY = 1900;

static inline int clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
static inline void add(uint8_t &stat, int delta) { stat = (uint8_t) clamp100((int) stat + delta); }

static uint32_t mix(uint32_t a, uint32_t b, float t) {
  if (t <= 0) return a;
  if (t >= 1) return b;
  int r = (int) (((a >> 16) & 0xFF) * (1 - t) + ((b >> 16) & 0xFF) * t);
  int g = (int) (((a >> 8) & 0xFF) * (1 - t) + ((b >> 8) & 0xFF) * t);
  int bl = (int) ((a & 0xFF) * (1 - t) + (b & 0xFF) * t);
  return (uint32_t) (r << 16 | g << 8 | bl);
}

static void button_pos(int i, int &x, int &y) {
  float a = (-70.0f + 28.0f * i) * 3.14159265f / 180.0f;  // six buttons around the bottom
  x = (int) lroundf(180 + 138 * sinf(a));
  y = (int) lroundf(180 + 138 * cosf(a));
}

static inline int sq(int v) { return v * v; }
static const int HOME_SWIPE_Y = 285;  // a swipe up starting below this goes home
static const int TT_DRAG_PX = 22;  // times tables: one table per this much drag up or down

// ---------------------------------------------------------------------------------------------
// Procedural sprite

namespace {

const int G = 32;
enum Expr { EX_NORMAL, EX_BLINK, EX_HAPPY, EX_SAD, EX_SLEEP, EX_EAT_OPEN, EX_EAT_SHUT, EX_SICK, EX_DEAD, EX_ANGRY };
enum Px : uint8_t {
  C_NONE, C_LINE, C_BODY, C_SHADE, C_ACCENT, C_EYE, C_SHINE, C_BLUSH, C_MOUTH, C_FEATURE, C_WHITE, C_GOLD, C_LEAF,
  C_HAT, C_HAT2,
  C_COUNT
};

struct Sprite {
  uint8_t px[G][G];
  uint32_t pal[C_COUNT];

  void put(int x, int y, uint8_t c) {
    if (x >= 0 && x < G && y >= 0 && y < G) px[y][x] = c;
  }
  bool is_body(int x, int y) const {
    return x >= 0 && x < G && y >= 0 && y < G && (px[y][x] == C_BODY || px[y][x] == C_SHADE);
  }
  void ellipse(float ex, float ey, float rx, float ry, uint8_t c, bool only_body = false) {
    for (int y = (int) floorf(ey - ry - 1); y <= (int) ceilf(ey + ry + 1); y++)
      for (int x = (int) floorf(ex - rx - 1); x <= (int) ceilf(ex + rx + 1); x++) {
        float dx = (x + 0.5f - ex) / rx, dy = (y + 0.5f - ey) / ry;
        if (dx * dx + dy * dy <= 1.0f && (!only_body || is_body(x, y))) put(x, y, c);
      }
  }
  void triangle(float ax, float ay, float bx, float by, float cx, float cy, uint8_t c) {
    auto edge = [](float x0, float y0, float x1, float y1, float px, float py) {
      return (x1 - x0) * (py - y0) - (y1 - y0) * (px - x0);
    };
    float area = edge(ax, ay, bx, by, cx, cy);
    for (int y = (int) floorf(fminf(ay, fminf(by, cy))); y <= (int) ceilf(fmaxf(ay, fmaxf(by, cy))); y++)
      for (int x = (int) floorf(fminf(ax, fminf(bx, cx))); x <= (int) ceilf(fmaxf(ax, fmaxf(bx, cx))); x++) {
        float px = x + 0.5f, py = y + 0.5f;
        float w0 = edge(bx, by, cx, cy, px, py), w1 = edge(cx, cy, ax, ay, px, py), w2 = edge(ax, ay, bx, by, px, py);
        if (area < 0) {
          w0 = -w0;
          w1 = -w1;
          w2 = -w2;
        }
        if (w0 >= 0 && w1 >= 0 && w2 >= 0) put(x, y, c);
      }
  }
  void outline() {
    static uint8_t copy[G][G];
    memcpy(copy, px, sizeof(px));
    for (int y = 0; y < G; y++)
      for (int x = 0; x < G; x++) {
        if (copy[y][x] != C_NONE) continue;
        bool n = (x > 0 && copy[y][x - 1]) || (x < G - 1 && copy[y][x + 1]) || (y > 0 && copy[y - 1][x]) ||
                 (y < G - 1 && copy[y + 1][x]);
        if (n) px[y][x] = C_LINE;
      }
  }
};

void base_palette(Sprite &sp, uint32_t body, uint32_t accent) {
  sp.pal[C_NONE] = 0;
  sp.pal[C_LINE] = mix(body, 0x1A1020, 0.78f);
  sp.pal[C_BODY] = body;
  sp.pal[C_SHADE] = mix(body, 0x301838, 0.22f);
  sp.pal[C_ACCENT] = accent;
  sp.pal[C_EYE] = 0x2B2233;
  sp.pal[C_SHINE] = 0xFFFFFF;
  sp.pal[C_BLUSH] = 0xFF8FA8;
  sp.pal[C_MOUTH] = 0x8E2A3A;
  sp.pal[C_FEATURE] = 0xFFF1C9;
  sp.pal[C_WHITE] = 0xFFFFFF;
  sp.pal[C_GOLD] = 0xFFC93C;
  sp.pal[C_LEAF] = 0x5BC85B;
  sp.pal[C_HAT] = 0xF0454B;
  sp.pal[C_HAT2] = 0xB02A36;
}

// Body radii in sprite pixels; the body's bottom sits on sprite row 28.5.
void body_size(const Genome &g, uint8_t stage, uint8_t form, float &rx, float &ry) {
  switch (stage) {
    case STAGE_BABY: rx = 6; ry = 5; break;
    case STAGE_CHILD: rx = 8; ry = 7; break;
    case STAGE_TEEN: rx = 9; ry = 8; break;
    default: rx = 10; ry = 9; break;
  }
  int shape = stage >= STAGE_CHILD ? g.shape % 5 : 0;
  if (shape == 1) { rx -= 1.5f; ry += 1.5f; }
  if (shape == 2) { rx += 2; ry -= 1.5f; }
  if (stage >= STAGE_ADULT && form == FORM_CHUBBY) { rx += 2; ry += 0.5f; }
  if (stage >= STAGE_ADULT && form == FORM_SPORTY) rx -= 1;
}

void build_egg(const Genome &g, int crack, Sprite &sp) {
  memset(sp.px, 0, sizeof(sp.px));
  base_palette(sp, 0xFFF6E5, BODY_COLORS[g.color % 12]);
  sp.pal[C_SHADE] = 0xEADCC4;
  sp.pal[C_LINE] = 0x6B5B4E;
  const float cx = 16, cy = 19.5f, rx = 7.5f, ry = 9.5f;
  for (int y = 0; y < G; y++)
    for (int x = 0; x < G; x++) {
      float dy = (y + 0.5f - cy) / ry;
      float w = dy < 0 ? 1.0f - 0.28f * (-dy) : 1.0f;
      float dx = (x + 0.5f - cx) / (rx * w);
      if (dx * dx + dy * dy <= 1.0f) sp.px[y][x] = (dx * 0.4f + dy * 0.75f > 0.5f) ? C_SHADE : C_BODY;
    }
  sp.ellipse(13, 15, 1.8f, 1.5f, C_ACCENT, true);
  sp.ellipse(19.5f, 20, 2.0f, 1.7f, C_ACCENT, true);
  sp.ellipse(14, 24, 1.5f, 1.2f, C_ACCENT, true);
  sp.put(12, 13, C_SHINE);
  sp.put(12, 14, C_SHINE);
  if (crack > 0) {
    int x0 = crack > 1 ? 9 : 12, x1 = crack > 1 ? 23 : 19;
    for (int x = x0; x <= x1; x++) sp.put(x, 18 + ((x % 4) < 2 ? 0 : 1), C_LINE);
  }
  sp.outline();
}

void build_pet(const Genome &g, uint8_t stage, uint8_t form, Expr ex, int look, bool squash, bool ghost, Sprite &sp,
               int hat = 0) {
  memset(sp.px, 0, sizeof(sp.px));
  uint32_t body = BODY_COLORS[g.color % 12];
  if (stage == STAGE_ELDER) body = mix(body, 0xC8C8C8, 0.35f);
  if (form == FORM_SCRUFFY && stage >= STAGE_ADULT) body = mix(body, 0x8A7A70, 0.25f);
  uint32_t accent = g.pattern == 3 ? mix(body, 0xFFFFFF, 0.6f) : BODY_COLORS[g.accent % 12];
  if (ghost) {
    body = 0xE6ECFF;
    accent = 0xFFFFFF;
  }
  base_palette(sp, body, accent);
  if (ghost) sp.pal[C_LINE] = 0x8C9AD0;

  float rx, ry;
  body_size(g, stage, form, rx, ry);
  int shape = stage >= STAGE_CHILD ? g.shape % 5 : 0;
  if (squash) { rx += 0.6f; ry -= 0.6f; }
  const float cx = 16, bottom = 28.5f;
  const float cy = bottom - ry;
  const int top = (int) floorf(cy - ry);

  auto inside = [&](int x, int y) {
    float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
    switch (shape) {
      case 3: {
        float w = 0.72f + 0.28f * (dy + 1) * 0.5f;
        dx /= w;
        return dx * dx + dy * dy <= 1.0f;
      }
      case 4: return dx * dx * dx * dx + dy * dy * dy * dy <= 1.0f;
      default: return dx * dx + dy * dy <= 1.0f;
    }
  };

  // Tail (adults), behind the body.
  if (stage >= STAGE_ADULT && g.tail && !ghost) {
    float tx = cx + rx * 0.92f, ty = bottom - ry * 0.55f;
    if (g.tail == 1) {
      sp.ellipse(tx + 1.8f, ty, 2.7f, 2.7f, C_BODY);
      sp.ellipse(tx + 1.8f, ty, 1.1f, 1.1f, C_NONE);
    } else if (g.tail == 2) {
      sp.triangle(tx - 1, ty - 2.5f, tx - 1, ty + 2, tx + 5, ty - 1.5f, C_ACCENT);
    } else {
      sp.ellipse(tx + 1.5f, ty, 2.6f, 2.3f, C_ACCENT);
    }
  }

  // Ears / head features (child and up), behind the body.
  if (stage >= STAGE_CHILD && g.ears) {
    float ey = cy - ry * 0.72f;
    for (int s = -1; s <= 1; s += 2) {
      float ex = cx + s * rx * 0.55f;
      switch (g.ears) {
        case 1:
          sp.triangle(ex - 3, ey + 1.5f, ex + 3, ey + 1.5f, ex + s * 1.0f, ey - 5, C_BODY);
          sp.triangle(ex - 1.3f, ey + 0.8f, ex + 1.3f, ey + 0.8f, ex + s * 0.7f, ey - 2.6f, C_BLUSH);
          break;
        case 2:
          sp.ellipse(ex + s * 0.6f, ey - 4.5f, 1.8f, 5.5f, C_BODY);
          sp.ellipse(ex + s * 0.6f, ey - 4.2f, 0.75f, 3.8f, C_BLUSH);
          break;
        case 3:
          sp.ellipse(ex + s * 0.8f, ey - 0.3f, 3, 3, C_BODY);
          sp.ellipse(ex + s * 0.8f, ey - 0.3f, 1.4f, 1.4f, C_SHADE);
          break;
        case 4: {
          float x0 = cx + s * 2.5f, y0 = cy - ry + 1.5f, x1 = cx + s * 4.5f, y1 = cy - ry - 4;
          for (int i = 0; i <= 8; i++) {
            float t = i / 8.0f;
            sp.put((int) floorf(x0 + (x1 - x0) * t), (int) floorf(y0 + (y1 - y0) * t), C_LINE);
          }
          sp.ellipse(x1 + 0.5f, y1 - 0.5f, 1.6f, 1.6f, C_ACCENT);
          break;
        }
        case 5:
          sp.triangle(ex - 1.7f, ey + 1.2f, ex + 1.7f, ey + 1.2f, ex + s * 1.2f, ey - 3.6f, C_FEATURE);
          break;
        case 6:
          if (s == 1) {
            sp.put(16, top, C_LEAF);
            sp.put(16, top - 1, C_LEAF);
            sp.ellipse(14.2f, top - 2.3f, 2.0f, 1.1f, C_LEAF);
            sp.ellipse(18.0f, top - 2.9f, 2.0f, 1.1f, C_LEAF);
          }
          break;
      }
    }
  }

  // Body
  bool hem = ghost || (stage >= STAGE_TEEN && g.feet == 1);
  for (int y = 0; y < G; y++)
    for (int x = 0; x < G; x++) {
      if (!inside(x, y)) continue;
      if (hem && y + 0.5f > bottom - 1.6f && ((x / 2) % 2 == 0)) continue;
      float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
      sp.px[y][x] = (dx * 0.35f + dy * 0.8f > 0.5f) ? C_SHADE : C_BODY;
    }

  // Pattern (teen and up)
  if (stage >= STAGE_TEEN && !ghost) {
    if (g.pattern == 3) {
      sp.ellipse(cx, cy + ry * 0.4f, rx * 0.55f, ry * 0.5f, C_ACCENT, true);
    } else if (g.pattern == 1) {
      static const float SP[4][2] = {{-0.55f, -0.4f}, {0.5f, -0.55f}, {0.62f, 0.2f}, {-0.35f, 0.45f}};
      for (int i = 0; i < 4; i++) {
        if (((g.spots >> i) & 1) && i != 0) continue;  // spot 0 always, others by seed
        float j = ((g.spots >> (4 + i)) & 1) ? 0.12f : -0.08f;
        sp.ellipse(cx + (SP[i][0] + j) * rx, cy + SP[i][1] * ry, 1.7f, 1.4f, C_ACCENT, true);
      }
    } else if (g.pattern == 2) {
      for (int y = 0; y < G; y++)
        for (int x = 0; x < G; x++) {
          float dy = (y + 0.5f - cy) / ry;
          if (dy < 0.0f && ((y - top) % 3 == 1) && sp.is_body(x, y)) sp.px[y][x] = C_ACCENT;
        }
    }
  }

  // Shine
  {
    int sx = (int) floorf(cx - rx * 0.55f), sy = (int) floorf(cy - ry * 0.55f);
    if (sp.is_body(sx, sy)) sp.put(sx, sy, C_SHINE);
    if (sp.is_body(sx + 1, sy - 1)) sp.put(sx + 1, sy - 1, C_SHINE);
  }

  // Feet
  if (stage >= STAGE_TEEN && g.feet == 0 && !ghost) {
    sp.ellipse(cx - rx * 0.45f, bottom, 2.4f, 1.4f, C_SHADE);
    sp.ellipse(cx + rx * 0.45f, bottom, 2.4f, 1.4f, C_SHADE);
  }

  // Adult forms: crown (royal), messy tuft (scruffy), spiky crest (wild), headband (sporty).
  bool adult_look = stage >= STAGE_ADULT && !ghost;
  if (adult_look && form == FORM_WILD) {
    sp.triangle(12.5f, top + 1.5f, 15.0f, top + 1.5f, 13.2f, top - 3.0f, C_SHADE);
    sp.triangle(14.8f, top + 1.5f, 17.2f, top + 1.5f, 16.0f, top - 4.2f, C_SHADE);
    sp.triangle(17.0f, top + 1.5f, 19.5f, top + 1.5f, 18.8f, top - 3.0f, C_SHADE);
  }
  if (adult_look && form == FORM_SPORTY) {
    int band_y = (int) lroundf(cy - ry * 0.22f) - 4;
    for (int y = band_y; y <= band_y + 1; y++)
      for (int x = 0; x < G; x++)
        if (sp.is_body(x, y)) sp.put(x, y, C_MOUTH);
    sp.put((int) (cx + rx) - 1, band_y + 2, C_MOUTH);  // the knot's tails
    sp.put((int) (cx + rx), band_y + 3, C_MOUTH);
  }
  // Hats from the shop (a hat hides the crown or tuft). Glasses go on after the eyes.
  if (!ghost && is_sprite_hat(hat) && hat != IT_GLASSES) {
    if (hat == IT_PROP_CAP) {  // striped beanie with a propeller
      sp.pal[C_HAT] = 0xF0454B;
      sp.pal[C_HAT2] = 0x4AA8FF;
      for (int y = top - 3; y <= top + 1; y++)
        for (int x = 0; x < G; x++) {
          float dx = (x + 0.5f - cx) / 5.5f, dy = (y + 0.5f - (top + 1.5f)) / 4.5f;
          if (dx * dx + dy * dy <= 1.0f) sp.put(x, y, (x / 2) % 2 ? C_HAT : C_HAT2);
        }
      sp.put((int) cx, top - 4, C_EYE);
      sp.put((int) cx, top - 5, C_EYE);
      for (int x = (int) cx - 4; x <= (int) cx + 3; x++) sp.put(x, top - 6, C_GOLD);
    } else if (hat == IT_STAR_HAT) {  // a big gold star
      static const char *const STAR[6] = {"...#...", "..###..", "#######", ".#####.", "..#.#..", ".#...#."};
      for (int r = 0; r < 6; r++)
        for (int k = 0; k < 7; k++)
          if (STAR[r][k] == '#') sp.put((int) cx - 3 + k, top - 6 + r, C_GOLD);
    } else if (hat == IT_CHEF_HAT) {  // a Munch prize: tall and puffy
      sp.pal[C_HAT] = 0xFFFFFF;
      sp.pal[C_HAT2] = 0xD8D8E0;
      for (int y = top - 4; y <= top + 1; y++)
        for (int x = (int) cx - 4; x <= (int) cx + 3; x++) sp.put(x, y, y >= top ? C_HAT2 : C_HAT);
      sp.ellipse(cx - 2.5f, top - 5.5f, 2.8f, 2.4f, C_HAT);
      sp.ellipse(cx + 2.0f, top - 5.5f, 2.8f, 2.4f, C_HAT);
      sp.ellipse(cx, top - 7.0f, 2.6f, 2.2f, C_HAT);
    } else if (hat == IT_WIZARD_HAT) {  // the Quiz prize: tall, blue, starry
      sp.pal[C_HAT] = 0x4A5AD8;
      sp.pal[C_HAT2] = 0x2E3A9A;
      sp.triangle(cx - 5.0f, top + 1.5f, cx + 5.0f, top + 1.5f, cx + 1.0f, top - 9.5f, C_HAT);
      for (int x = (int) cx - 7; x <= (int) cx + 6; x++) sp.put(x, top + 1, C_HAT2);
      sp.put((int) cx - 1, top - 1, C_GOLD);
      sp.put((int) cx + 1, top - 4, C_GOLD);
      sp.put((int) cx, top - 7, C_GOLD);
      sp.put((int) cx + 2, top - 1, C_GOLD);
    } else if (hat == IT_BOW) {
      float bx = cx + rx * 0.45f, by = top + 0.5f;
      sp.triangle(bx - 3.6f, by - 1.8f, bx - 3.6f, by + 1.8f, bx, by, C_HAT);
      sp.triangle(bx + 3.6f, by - 1.8f, bx + 3.6f, by + 1.8f, bx, by, C_HAT);
      sp.ellipse(bx, by, 1.0f, 1.0f, C_HAT2);
    } else if (hat == IT_CAP) {
      sp.pal[C_HAT] = 0x4AA8FF;
      sp.pal[C_HAT2] = 0x2F7FD0;
      for (int y = top - 3; y <= top + 1; y++)
        for (int x = 0; x < G; x++) {
          float dx = (x + 0.5f - cx) / 6.0f, dy = (y + 0.5f - (top + 1.5f)) / 4.5f;
          if (dx * dx + dy * dy <= 1.0f) sp.put(x, y, C_HAT);
        }
      for (int x = (int) cx - 6; x <= (int) cx + 9; x++) sp.put(x, top + 1, C_HAT2);  // peak to the right
      sp.put((int) cx - 2, top - 2, C_WHITE);
    } else {
      sp.pal[C_HAT] = 0xFFD23F;
      sp.triangle(cx - 3.8f, top + 1.5f, cx + 3.8f, top + 1.5f, cx, top - 7.5f, C_HAT);
      for (int x = 0; x < G; x++) {  // two red stripes
        if (top >= 1 && sp.px[top - 1][x] == C_HAT) sp.px[top - 1][x] = C_HAT2;
        if (top >= 4 && sp.px[top - 4][x] == C_HAT) sp.px[top - 4][x] = C_HAT2;
      }
      sp.ellipse(cx, top - 8.0f, 1.3f, 1.3f, C_WHITE);
    }
  }
  if (adult_look && !hat && form == FORM_BRAINY) {  // graduation cap with a gold tassel
    int b = top - 1;
    for (int x = (int) cx - 7; x <= (int) cx + 6; x++) sp.put(x, b - 1, C_EYE);
    for (int x = (int) cx - 4; x <= (int) cx + 3; x++) sp.put(x, b, C_EYE);
    for (int y = b - 1; y <= b + 3; y++) sp.put((int) cx + 6, y, C_GOLD);
    sp.put((int) cx + 7, b + 3, C_GOLD);
  }
  if (adult_look && !hat && g.ears != 6 && g.ears != 4) {
    if (form == FORM_ROYAL) {
      int y0 = top - 1;
      for (int x = 13; x <= 18; x++) {
        sp.put(x, y0, C_GOLD);
        sp.put(x, y0 - 1, C_GOLD);
      }
      sp.put(13, y0 - 2, C_GOLD);
      sp.put(15, y0 - 2, C_GOLD);
      sp.put(16, y0 - 2, C_GOLD);
      sp.put(18, y0 - 2, C_GOLD);
      sp.put(15, y0 - 1, C_BLUSH);
      sp.put(16, y0 - 1, C_BLUSH);
    } else if (form == FORM_SCRUFFY) {
      sp.put(14, top - 1, C_SHADE);
      sp.put(15, top - 2, C_SHADE);
      sp.put(16, top - 1, C_SHADE);
      sp.put(17, top - 2, C_SHADE);
      sp.put(15, top - 1, C_SHADE);
    }
  }

  // Eyes
  int w = g.eyes == 0 ? 1 : (g.eyes == 1 ? 2 : 3);
  if (stage == STAGE_BABY && w == 3) w = 2;
  int h = g.eyes == 0 ? 2 : 3;
  int eo = (int) lroundf(rx * 0.32f);
  int lx = 16 - eo - w + look, rxe = 16 + eo + look;
  int ey = (int) lroundf(cy - ry * 0.22f) - 1;
  int my = ey + h + 1;
  for (int side = 0; side < 2; side++) {
    int x = side == 0 ? lx : rxe;
    int c = x + w / 2;  // eye centre column
    switch (ex) {
      case EX_BLINK:
      case EX_SLEEP:
        for (int i = 0; i < (w < 2 ? 2 : w); i++) sp.put(x - (w < 2 ? side == 0 ? 1 : 0 : 0) + i, ey + h - 1, C_EYE);
        break;
      case EX_HAPPY:
        sp.put(c - 1, ey + 1, C_EYE);
        sp.put(c, ey, C_EYE);
        sp.put(c + 1, ey + 1, C_EYE);
        break;
      case EX_SICK:
        sp.put(c - 1, ey, C_EYE);
        sp.put(c, ey, C_EYE);
        sp.put(c + 1, ey, C_EYE);
        sp.put(c + 1, ey + 1, C_EYE);
        sp.put(c + 1, ey + 2, C_EYE);
        sp.put(c, ey + 2, C_EYE);
        sp.put(c - 1, ey + 2, C_EYE);
        sp.put(c - 1, ey + 1, C_EYE);
        break;
      case EX_DEAD:
        if (ghost) {
          sp.put(c - 1, ey + 1, C_EYE);
          sp.put(c, ey + 2, C_EYE);
          sp.put(c + 1, ey + 1, C_EYE);
        } else {
          sp.put(c - 1, ey, C_EYE);
          sp.put(c + 1, ey, C_EYE);
          sp.put(c, ey + 1, C_EYE);
          sp.put(c - 1, ey + 2, C_EYE);
          sp.put(c + 1, ey + 2, C_EYE);
        }
        break;
      default:
        for (int yy = 0; yy < h; yy++)
          for (int xx = 0; xx < w; xx++) sp.put(x + xx, ey + yy, C_EYE);
        if (w > 1) sp.put(x, ey, C_SHINE);
        break;
    }
    int dir = side == 0 ? 1 : -1;  // towards the middle of the face
    if (ex == EX_ANGRY) {
      sp.put(c - dir * 1, ey - 3, C_EYE);
      sp.put(c, ey - 2, C_EYE);
      sp.put(c + dir * 1, ey - 2, C_EYE);
    } else if (ex == EX_SAD) {
      sp.put(c - dir * 1, ey - 2, C_EYE);
      sp.put(c, ey - 2, C_EYE);
      sp.put(c + dir * 1, ey - 3, C_EYE);
    } else if (stage == STAGE_ELDER && !ghost) {
      sp.put(c - 1, ey - 2, C_WHITE);
      sp.put(c, ey - 2, C_WHITE);
      sp.put(c + 1, ey - 2, C_WHITE);
    }
  }

  if (hat == IT_GLASSES && !ghost) {
    for (int side = 0; side < 2; side++) {
      int x0 = (side == 0 ? lx : rxe) - 1, x1 = (side == 0 ? lx : rxe) + w, y0 = ey - 1, y1 = ey + h;
      for (int x = x0; x <= x1; x++) {
        sp.put(x, y0, C_EYE);
        sp.put(x, y1, C_EYE);
      }
      for (int y = y0; y <= y1; y++) {
        sp.put(x0, y, C_EYE);
        sp.put(x1, y, C_EYE);
      }
    }
    for (int x = lx + w + 1; x < rxe - 1; x++) sp.put(x, ey, C_EYE);  // bridge
  }

  // Cheeks
  if ((g.cheeks || ex == EX_HAPPY) && ex != EX_DEAD && !ghost) {
    int cy2 = ey + h;
    for (int i = 0; i < 2; i++) {
      if (sp.is_body(lx - 2 + i, cy2)) sp.put(lx - 2 + i, cy2, C_BLUSH);
      if (sp.is_body(rxe + w + i, cy2)) sp.put(rxe + w + i, cy2, C_BLUSH);
    }
  }

  // Mouth
  int m = 15 + look;
  switch (ex) {
    case EX_HAPPY:
      sp.put(m - 1, my, C_MOUTH);
      sp.put(m + 2, my, C_MOUTH);
      sp.put(m, my + 1, C_MOUTH);
      sp.put(m + 1, my + 1, C_MOUTH);
      break;
    case EX_EAT_OPEN:
      for (int x = m - 1; x <= m + 2; x++) sp.put(x, my, C_MOUTH);
      sp.put(m - 1, my + 1, C_MOUTH);
      sp.put(m, my + 1, C_BLUSH);
      sp.put(m + 1, my + 1, C_BLUSH);
      sp.put(m + 2, my + 1, C_MOUTH);
      break;
    case EX_EAT_SHUT:
    case EX_SICK:
      sp.put(m - 1, my, C_MOUTH);
      sp.put(m, my + 1, C_MOUTH);
      sp.put(m + 1, my, C_MOUTH);
      sp.put(m + 2, my + 1, C_MOUTH);
      break;
    case EX_SAD:
    case EX_ANGRY:
      sp.put(m - 1, my + 1, C_MOUTH);
      sp.put(m, my, C_MOUTH);
      sp.put(m + 1, my, C_MOUTH);
      sp.put(m + 2, my + 1, C_MOUTH);
      break;
    default:
      sp.put(m, my, C_MOUTH);
      sp.put(m + 1, my, C_MOUTH);
      break;
  }
  if (adult_look && form == FORM_WILD && ex != EX_DEAD) sp.put(m + 2, my + 1, C_WHITE);  // a little fang

  sp.outline();
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Game state

uint32_t PetEngine::rnd() {
  uint32_t x = rng_;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_ = x;
  return x;
}

void PetEngine::new_game(uint32_t seed) {
  s_ = SaveData{};
  s_.generation = 0;
  toast_[0] = 0;
  hatch_new_egg(seed);
}

bool PetEngine::restore(const SaveData &data, uint32_t seed) {
  rng_ = seed ? seed : 0x1234567;
  if (data.magic != SAVE_MAGIC && data.magic != SAVE_MAGIC_V3 && data.magic != SAVE_MAGIC_V2) return false;
  s_ = data;
  // Upgrade older saves one step at a time. Their newer bytes are zero; start them sensibly.
  if (s_.magic == SAVE_MAGIC_V2) {  // before discipline and family
    s_.discipline = 50;
    s_.tantrum = 0;
    s_.parent_a = s_.parent_b = s_.parent_c = 0;
    s_.magic = SAVE_MAGIC_V3;
  }
  if (s_.magic == SAVE_MAGIC_V3) {  // before time-together growth and the shop
    if (s_.stage > STAGE_ELDER) s_.stage = STAGE_ELDER;
    s_.together_s = STAGE_TOGETHER_S[s_.stage];  // carry on from the start of its current stage
    s_.coins = 20;
    s_.owned = 0;
    s_.hat = 0;
    s_.magic = SAVE_MAGIC;
  }
  if (s_.stage > STAGE_ELDER) s_.stage = STAGE_ELDER;
  if (s_.hat >= IT_COUNT || !owns(s_.hat)) s_.hat = 0;
  if (s_.form > FORM_BRAINY) s_.form = FORM_PLAIN;
  update_names();
  return true;
}

void PetEngine::hatch_new_egg(uint32_t seed, bool inherit) {
  rng_ = seed ? seed : 0x1234567;
  for (int i = 0; i < 4; i++) rnd();
  uint16_t gen = s_.generation + 1;
  // The household carries on to the next pet: high scores, coins, toys and hats, settings.
  const SaveData old = s_;
  const bool first = s_.magic == 0;
  const Genome parent = s_.g;
  s_ = SaveData{};
  s_.best_run = old.best_run;
  s_.best_catch = old.best_catch;
  s_.best_pop = old.best_pop;
  s_.best_copy = old.best_copy;
  s_.best_quiz = old.best_quiz;
  s_.best_facts = old.best_facts;
  s_.best_fizz = old.best_fizz;
  s_.best_munch = old.best_munch;
  s_.best_share = old.best_share;
  s_.smarts_x10 = inherit ? (uint16_t) (old.smarts_x10 / 4) : 0;  // a head start from a clever parent
  s_.settings = old.settings;
  s_.brightness = old.brightness;
  s_.owned = old.owned;
  s_.owned2 = old.owned2;
  s_.owned3 = old.owned3;
  s_.owned4 = old.owned4;
  s_.chant_medals = old.chant_medals;
  s_.best_chant = old.best_chant;
  s_.coins = first ? 20 : old.coins;
  uint32_t last = old.last_epoch;
  s_.magic = SAVE_MAGIC;
  s_.generation = gen;
  s_.last_epoch = last;
  Genome &g = s_.g;
  g.shape = rnd() % 5;
  g.color = rnd() % 12;
  do {
    g.accent = rnd() % 12;
  } while (g.accent == g.color);
  g.ears = rnd() % 7;
  g.eyes = rnd() % 3;
  g.pattern = rnd() % 4;
  g.tail = rnd() % 4;
  g.feet = (rnd() % 5) == 0;
  g.cheeks = rnd() % 2;
  g.name_a = rnd() % 20;
  g.name_b = rnd() % 20;
  g.name_c = (rnd() % 3 == 0) ? 1 + rnd() % 20 : 0;
  g.spots = rnd() & 0xFF;
  if (inherit) {
    // Each trait comes from the parent half the time (colour a bit more often); the first syllable of the
    // name always does, so a family sounds like one.
    auto from_parent = [&](uint8_t &field, uint8_t value) {
      if (rnd() % 2) field = value;
    };
    from_parent(g.shape, parent.shape);
    if (rnd() % 10 < 6) g.color = parent.color;
    from_parent(g.accent, parent.accent);
    if (g.accent == g.color) g.accent = (uint8_t) ((g.color + 5) % 12);
    from_parent(g.ears, parent.ears);
    from_parent(g.eyes, parent.eyes);
    from_parent(g.pattern, parent.pattern);
    from_parent(g.tail, parent.tail);
    from_parent(g.feet, parent.feet);
    from_parent(g.cheeks, parent.cheeks);
    g.name_a = parent.name_a;
    if (g.name_b == parent.name_b) g.name_b = (uint8_t) ((g.name_b + 7) % 20);
    s_.parent_a = (uint8_t) (parent.name_a + 1);
    s_.parent_b = (uint8_t) (parent.name_b + 1);
    s_.parent_c = parent.name_c;  // already 0 = none, else syllable + 1
  }
  s_.discipline = 50;
  s_.food = 70;
  s_.happy = 70;
  s_.energy = 90;
  s_.clean = 100;
  s_.health = 100;
  s_.lights_on = 1;
  s_.stage = STAGE_EGG;
  s_.weight = 0;
  walk_x_ = walk_target_ = 0;
  page_ = 0;
  overlay_ = OV_NONE;
  act_ = ACT_IDLE;
  update_names();
  dirty_ = true;
}

void PetEngine::update_names() {
  const Genome &g = s_.g;
  snprintf(name_, sizeof(name_), "%s%s%s", SYLLABLES[g.name_a % 20], SYLLABLES[g.name_b % 20],
           g.name_c ? SYLLABLES[(g.name_c - 1) % 20] : "");
  if (name_[0] >= 'a' && name_[0] <= 'z') name_[0] -= 32;
  if (s_.parent_a) {
    snprintf(parent_, sizeof(parent_), "%s%s%s", SYLLABLES[(s_.parent_a - 1) % 20], SYLLABLES[(s_.parent_b - 1) % 20],
             s_.parent_c ? SYLLABLES[(s_.parent_c - 1) % 20] : "");
    if (parent_[0] >= 'a' && parent_[0] <= 'z') parent_[0] -= 32;
  } else {
    parent_[0] = 0;
  }
  if (s_.stage == STAGE_EGG) {
    snprintf(species_, sizeof(species_), "Mystery egg");
  } else if (s_.stage == STAGE_BABY) {
    snprintf(species_, sizeof(species_), "Babyblob");
  } else {
    static const char *const FORM_PREFIX[7] = {"", "Royal ", "Scruffy ", "Sporty ", "Chubby ", "Wild ", "Brainy "};
    const char *prefix = s_.stage >= STAGE_ADULT ? FORM_PREFIX[s_.form % 7] : "";
    snprintf(species_, sizeof(species_), "%s%s %s", prefix, EAR_NAMES[g.ears % 7], SHAPE_NAMES[g.shape % 5]);
  }
}

int PetEngine::base_weight(uint8_t stage) const {
  static const int W[6] = {0, 5, 10, 20, 30, 30};
  return W[stage > 5 ? 5 : stage];
}

int PetEngine::minute_of_day(uint32_t epoch) const {
  return (int) (((epoch / 60) + (uint32_t) tz_off_min_) % 1440);
}

bool PetEngine::is_night(int mod) const { return mod >= 0 && (mod >= 21 * 60 || mod < 7 * 60); }

void PetEngine::set_clock(uint32_t utc_epoch, int local_minute_of_day) {
  int utc_mod = (int) ((utc_epoch / 60) % 1440);
  tz_off_min_ = ((local_minute_of_day - utc_mod) % 1440 + 1440) % 1440;
  epoch_ = utc_epoch;
  clock_valid_ = true;
}

void PetEngine::set_activity(Activity a, uint32_t dur_ms) {
  act_ = a;
  act_start_ = now_;
  act_until_ = now_ + dur_ms;
}

void PetEngine::show_toast(const char *msg, uint32_t dur_ms) {
  snprintf(toast_, sizeof(toast_), "%s", msg);
  toast_until_ = now_ + dur_ms;
}

bool PetEngine::needs_attention() const {
  if (s_.dead || s_.stage == STAGE_EGG) return false;
  return s_.food < 20 || s_.happy < 20 || s_.poops >= 3 || s_.sick || (s_.asleep && s_.lights_on) || s_.clean < 15 ||
         s_.health < 30 || s_.tantrum || (needs_math() && math() < 20);
}

const char *PetEngine::stage_name() const { return STAGE_NAMES[s_.stage > 5 ? 5 : s_.stage]; }

const char *PetEngine::mood() const {
  if (s_.dead) return "gone";
  if (s_.stage == STAGE_EGG) return "egg";
  if (s_.asleep) return "asleep";
  if (s_.sick) return "sick";
  if (s_.tantrum) return "naughty";
  if (s_.food < 20) return "hungry";
  if (needs_math() && math() < 20) return "needs math";
  if (s_.happy < 20) return "sad";
  if (s_.poops >= 3 || s_.clean < 15) return "dirty";
  if (s_.energy < 15) return "tired";
  if (s_.happy >= 70) return "happy";
  return "ok";
}

void PetEngine::evolve_to(uint8_t stage) {
  uint8_t prev = s_.stage;
  s_.stage = stage;
  s_.weight = (uint8_t) (s_.weight + base_weight(stage) - base_weight(prev));
  if (stage == STAGE_ADULT) {
    // How it was raised decides what it becomes. First match wins.
    uint8_t f = FORM_PLAIN;
    if (s_.care_mistakes >= 5)
      f = FORM_SCRUFFY;
    else if (s_.care_mistakes <= 1 && s_.discipline >= 70)
      f = FORM_ROYAL;
    else if (s_.smarts_x10 >= 500)  // lots of math practice
      f = FORM_BRAINY;
    else if (s_.discipline < 30)
      f = FORM_WILD;
    else if (s_.weight > base_weight(STAGE_ADULT) * 3 / 2)
      f = FORM_CHUBBY;
    else if (s_.runs >= 5 || s_.best_run >= 25)
      f = FORM_SPORTY;
    s_.form = f;
  }
  update_names();
  char msg[40];
  if (stage == STAGE_BABY) {
    set_activity(ACT_HATCH, 2000);
    snprintf(msg, sizeof(msg), "%s hatched!", name_);
    sound(SND_HATCH, true);
  } else {
    set_activity(ACT_HAPPY, 2000);
    s_.coins = (uint16_t) (s_.coins + 10);  // a present for growing up
    snprintf(msg, sizeof(msg), "%s grew up! +10", name_);
    sound(SND_GROW, true);
  }
  show_toast(msg, 4000);
  dirty_ = true;
}

void PetEngine::die() {
  s_.dead = 1;
  s_.asleep = 0;
  s_.sick = 0;
  s_.lights_on = 1;
  overlay_ = OV_NONE;
  char msg[40];
  snprintf(msg, sizeof(msg), "%s is gone...", name_);
  show_toast(msg, 6000);
  sound(SND_BYE, true);
  dirty_ = true;
}

// Old age: the elder sets off on an adventure and leaves an egg that takes after it.
void PetEngine::retire() {
  char msg[40];
  snprintf(msg, sizeof(msg), "%s left an egg!", name_);
  hatch_new_egg(rnd() ^ now_, true);
  show_toast(msg, 6000);
  sound(SND_BYE, true);
}

void PetEngine::sound(const char *rtttl, bool even_asleep) {
  if (!sound_enabled() || (s_.asleep && !even_asleep)) return;
  sound_ = rtttl;
}

// A tantrum means no; a badly behaved pet sometimes says no anyway.
bool PetEngine::disobeys(const char *what) {
  (void) what;
  bool no = s_.tantrum > 0 || (s_.discipline < 40 && (int) (rnd() % 100) < (40 - s_.discipline));
  if (!no) return false;
  set_activity(ACT_REFUSE, 1200);
  show_toast(s_.tantrum ? "Won't! (scold it)" : "Won't!");
  return true;
}

// Growing up follows time together, not the calendar.
void PetEngine::check_growth() {
  if (s_.dead) return;
  for (int st = STAGE_ELDER; st > s_.stage; st--) {
    if (s_.together_s >= STAGE_TOGETHER_S[st]) {
      evolve_to((uint8_t) st);
      break;
    }
  }
  if (s_.stage == STAGE_ELDER && s_.together_s >= RETIRE_TOGETHER_S) retire();
}

int PetEngine::grows_in_min() const {
  if (s_.dead) return -1;
  uint32_t next = s_.stage >= STAGE_ELDER ? RETIRE_TOGETHER_S : STAGE_TOGETHER_S[s_.stage + 1];
  return next > s_.together_s ? (int) ((next - s_.together_s + 59) / 60) : 0;
}

bool PetEngine::is_active(uint32_t now) const { return last_user_ != 0 && now - last_user_ < ACTIVE_WINDOW_MS; }

bool PetEngine::is_dozing(uint32_t now) const {
  return !is_active(now) && !s_.asleep && !s_.dead && s_.stage != STAGE_EGG;
}

// One pet minute. With someone around (active) these come 15 per real minute; dozing, 0.75.
// Dozing it can't fall sick, throw tantrums or count care mistakes, and neglect hurts it half as fast.
void PetEngine::tick_minute(int mod, bool active) {
  dirty_ = true;
  if (s_.dead || s_.stage == STAGE_EGG) return;
  s_.age_min++;
  auto every = [&](uint32_t n) { return n <= 1 || (s_.age_min % n) == 0; };

  // Sleep: by the clock at night, or a nap when worn out. Lights follow sleep.
  bool night = is_night(mod);
  if (!s_.asleep && (night || s_.energy == 0)) {
    s_.asleep = 1;
    s_.lights_on = 0;
    s_.tantrum = 0;
    overlay_ = OV_NONE;
  } else if (s_.asleep && !night && (s_.energy >= 95 || (mod >= 7 * 60 && mod < 10 * 60 && s_.energy >= 60))) {
    // Mornings it gets up once rested; a daytime nap lasts until the bolt is nearly full.
    s_.asleep = 0;
    s_.lights_on = 1;
    if (owns(IT_NIGHT_LIGHT)) add(s_.happy, 20);  // slept well with its night light on
    show_toast("Good morning!", 3000);
  }

  // In pet minutes, awake: food lasts about 10 h (babies 7 h), fun about 12 h, energy 13 h. With you that is
  // 40, 47 and 53 real minutes. Asleep, food and fun last days.
  bool baby = s_.stage == STAGE_BABY;
  bool teddy = owns(IT_TEDDY);  // no sadness while it sleeps or dozes
  if (!s_.asleep) {
    if (every(baby ? 4 : 6)) add(s_.food, -1);
    if (!(teddy && !active) && every(s_.sick ? 3 : (owns(IT_BALL) ? 9 : 7))) add(s_.happy, -1);
    if (every(8)) {
      add(s_.energy, -1);
      if (s_.energy == 20 && active) show_toast("Sleepy... lights off?", 6000);
    }
    if (every(180) && s_.weight > base_weight(s_.stage)) s_.weight--;
  } else {
    if (every(15)) add(s_.food, -1);
    if (!teddy && every(20)) add(s_.happy, -1);
    // Real-time recharge is in loop(); this covers minutes caught up after a power cut.
    add(s_.energy, s_.lights_on ? 1 : 2);
  }
  if (every(owns(IT_PLANT) ? 40 : 20)) add(s_.clean, -1);
  // Math: awake it lasts about 28 pet hours (about 2 hours of play), asleep 4 days. With half an hour of play a
  // day, two math games a day keep it topped up.
  if (needs_math() && s_.math_need < 100 && every(s_.asleep ? 60 : 17)) {
    s_.math_need++;
    if (s_.math_need == 80 && active) show_toast("Math time? (Play)", 5000);
  }
  if (s_.poops && every(s_.poops >= 3 ? 2 : 8 - 2 * s_.poops)) add(s_.clean, -1);

  if (s_.poop_timer > 0 && !s_.asleep) {
    if (--s_.poop_timer == 0 && s_.poops < 4) {
      s_.poops++;
      add(s_.clean, -5);
    }
  }
  if (s_.age_min % 240 == 0) s_.snacks_hour = 0;

  bool starving = s_.food == 0, miserable = s_.happy == 0, filthy = s_.clean < 20 || s_.poops >= 3;
  bool no_math = needs_math() && math() == 0;
  if (!s_.sick && !s_.asleep && active) {
    int chance = 0;  // per mille per minute
    if (s_.clean < 30) chance += 2;
    if (starving) chance += 2;
    if (s_.poops >= 3) chance += 3;
    if (s_.weight > base_weight(s_.stage) * 2) chance += 2;
    if (chance && (int) (rnd() % 1000) < chance) {
      s_.sick = 1;
      char msg[40];
      snprintf(msg, sizeof(msg), "%s feels sick", name_);
      show_toast(msg, 4000);
    }
  }

  // Tantrums (only with someone there to see): now and then it acts up for no reason. Scold it within
  // 2 real minutes to raise discipline; ignored, discipline drops. The better behaved, the rarer (none at 100).
  if (s_.tantrum) {
    if (--s_.tantrum == 0) add(s_.discipline, -5);
  } else if (active && !s_.asleep && !s_.sick && !needs_attention()) {
    int permille = (100 - s_.discipline) * 4 / 100;  // at discipline 0 about one per 15 minutes together
    if (s_.stage == STAGE_ELDER) permille /= 2;
    if (permille && (int) (rnd() % 1000) < permille) {
      s_.tantrum = 30;
      char msg[40];
      snprintf(msg, sizeof(msg), "%s is acting up!", name_);
      show_toast(msg, 4000);
    }
  }

  // Health only drops from neglect, and slower while it sleeps or dozes: left alone for good it lasts several
  // days.
  int slow = !active ? 4 : (s_.asleep ? 2 : 1);
  if (s_.sick && every(10 * slow)) add(s_.health, -1);
  if (starving && every(20 * slow)) add(s_.health, -1);
  if ((miserable || filthy) && every(30 * slow)) add(s_.health, -1);
  if (no_math && every(20 * slow)) add(s_.health, -1);  // as bad as starving
  if (!s_.sick && !starving && !miserable && !filthy && !no_math && every(10)) add(s_.health, 1);
  // Smarts fade without practice: slowly once Math is low, faster when it is empty.
  if (needs_math() && math() < 25 && s_.smarts_x10 > 0 && every(no_math ? 5 : 15)) s_.smarts_x10--;
  if (s_.health == 0) {
    die();
    return;
  }

  // A care mistake: a need ignored for 120 pet minutes (8 real minutes) while someone is there.
  if (needs_attention()) {
    if (active && s_.attention_min < 255) s_.attention_min++;
    if (s_.attention_min == 120 && s_.care_mistakes < 255) {
      s_.care_mistakes++;
      s_.attention_min++;
    }
  } else {
    s_.attention_min = 0;
  }
}

// Woken from a doze by a touch: say what it needs most.
void PetEngine::greet() {
  const char *msg = "Hi!";
  if (s_.sick)
    msg = "I feel sick...";
  else if (s_.poops)
    msg = "Clean up?";
  else if (s_.food < 30)
    msg = "I'm hungry!";
  else if (needs_math() && math() < 30)
    msg = "Math time!";
  else if (s_.energy < 20)
    msg = "Sleepy...";
  else if (s_.happy < 40)
    msg = "Play with me!";
  else if (s_.clean < 30)
    msg = "Bath time?";
  set_activity(ACT_HAPPY, 1200);
  show_toast(msg, 3000);
  sound(SND_HAPPY);
}

void PetEngine::loop(uint32_t now) {
  now_ = now;
  uint32_t raw = last_loop_ ? now - last_loop_ : 0;  // for the clocks: every millisecond counts
  if (raw > 600000) raw = 600000;
  uint32_t dt = raw > 200 ? 200 : raw;  // for animation and physics
  last_loop_ = now;
  bool active = is_active(now);
  int mod = clock_valid_ ? minute_of_day(epoch_) : -1;

  // After a power cut, make up the time it was off as dozing, once the clock is known (at most 3 days).
  if (clock_valid_ && !caught_up_) {
    caught_up_ = true;
    if (s_.last_epoch != 0 && epoch_ > s_.last_epoch + 60) {
      uint32_t off = epoch_ - s_.last_epoch;
      if (off > MAX_OFFLINE_S) off = MAX_OFFLINE_S;
      uint32_t start = epoch_ - off;
      int ticks = (int) (off / 60.0f * SPEED_DOZE);
      for (int i = 0; i < ticks && !s_.dead; i++)
        tick_minute(minute_of_day(start + (uint32_t) (i * 60 / SPEED_DOZE)), false);
      s_.together_s += (uint32_t) (off * DOZE_GROWTH);
      check_growth();
    }
  }
  if (clock_valid_) s_.last_epoch = epoch_;

  // Pet time runs fast with someone around and slow while it dozes; growth counts time together.
  pet_acc_ms_ += raw * (active ? SPEED_ACTIVE : SPEED_DOZE);
  while (pet_acc_ms_ >= 60000) {
    pet_acc_ms_ -= 60000;
    tick_minute(mod, active);
  }
  together_acc_ms_ += active ? raw : raw * DOZE_GROWTH;
  if (together_acc_ms_ >= 1000) {
    uint32_t s = (uint32_t) (together_acc_ms_ / 1000);
    s_.together_s += s;
    together_acc_ms_ -= s * 1000.0f;
  }
  check_growth();

  if (!active && s_.tantrum) s_.tantrum = 0;  // nobody to throw it for
  // Beep when it starts needing you and every 3 minutes after, while someone is around and not at night.
  bool need = needs_attention();
  if (active && need && !is_night(mod) && (!was_needy_ || now - last_call_ > 180000)) {
    sound(SND_CALL);
    last_call_ = now;
  }
  was_needy_ = need && active;

  // Sleep recharges in real time: in the dark empty to full in 1 minute, light on in 2; a bed halves both.
  if (s_.asleep && !s_.dead) {
    sleep_acc_ms_ += dt;
    const uint32_t per_point = (s_.lights_on ? 1200 : 600) / (owns(IT_BED) ? 2 : 1);
    while (sleep_acc_ms_ >= per_point) {
      sleep_acc_ms_ -= per_point;
      if (s_.energy < 100) {
        add(s_.energy, 1);
        dirty_ = true;
      }
    }
    if (s_.energy >= 100 && !is_night(mod)) {  // a daytime nap ends once rested; at night it sleeps on
      s_.asleep = 0;
      s_.lights_on = 1;
      show_toast("Rested!", 2000);
      dirty_ = true;
    }
  } else {
    sleep_acc_ms_ = 0;
  }

  if (act_ != ACT_IDLE && (int32_t) (now - act_until_) >= 0) act_ = ACT_IDLE;
  if (pressed_ && !long_fired_ && now - press_ms_ > 1200) {
    long_fired_ = true;
    long_press(tx0_, ty0_);
  }
  // Settings page: holding New egg for 2 s replaces the pet.
  if (egg_hold_progress(now) >= 1.0f) {
    long_fired_ = true;
    hatch_new_egg(rnd() ^ now);
    show_toast("A new egg!", 3000);
  }
  if ((overlay_ == OV_FEED || overlay_ == OV_GAMES) && now - overlay_since_ > 10000) overlay_ = OV_NONE;
  if (overlay_ == OV_STAND && now - overlay_since_ > 30000) overlay_ = OV_NONE;
  if (overlay_ == OV_GAME) {
    if (run_phase_ == RUN_READY && now - run_t_ > 15000) {
      overlay_ = OV_NONE;  // never started
    } else if (run_phase_ == RUN_OVER && now - run_t_ > 4000) {
      finish_game();
    } else if (run_phase_ == RUN_GO) {
      if (game_kind_ == GAME_CHANT) {
        chant_update();
      } else if (is_quiz_like(game_kind_) || game_kind_ == GAME_FACTS) {
        math_update();
      } else if (game_kind_ == GAME_FIZZ || game_kind_ == GAME_MUNCH) {
        float left = dt / 1000.0f;
        while (left > 0 && overlay_ == OV_GAME && run_phase_ == RUN_GO) {
          float step = fminf(left, 0.016f);
          if (game_kind_ == GAME_FIZZ)
            fizz_update(step);
          else
            munch_update(step);
          left -= step;
        }
      } else if (game_kind_ == GAME_POP) {
        pop_update();
      } else if (game_kind_ == GAME_COPY) {
        copy_update();
      } else {
        // Small fixed steps keep collisions honest when a frame takes long.
        float left = dt / 1000.0f;
        while (left > 0 && overlay_ == OV_GAME && run_phase_ == RUN_GO) {
          float step = fminf(left, 0.016f);
          if (game_kind_ == GAME_CATCH)
            catch_update(step);
          else
            run_update(step);
          left -= step;
        }
      }
    }
  }

  // Wander about (not while dozing).
  if (active && act_ == ACT_IDLE && overlay_ == OV_NONE && !s_.asleep && !s_.dead && s_.stage != STAGE_EGG) {
    float lo = -60, hi = s_.poops ? -5 : 50;
    if (fabsf(walk_x_ - walk_target_) < 1 && (int32_t) (now - walk_next_) >= 0) {
      walk_target_ = lo + (rnd() % 1000) / 1000.0f * (hi - lo);
      walk_next_ = now + 2500 + rnd() % 5000;
    }
    if (walk_target_ > hi) walk_target_ = hi;
    float step = (s_.sick ? 0.012f : 0.03f) * dt;
    if (walk_x_ < walk_target_ - 1) {
      walk_x_ = fminf(walk_x_ + step, walk_target_);
      facing_ = 1;
    } else if (walk_x_ > walk_target_ + 1) {
      walk_x_ = fmaxf(walk_x_ - step, walk_target_);
      facing_ = -1;
    }
  }
}

int PetEngine::wanted_fps() const {
  if (overlay_ == OV_GAME) return 25;
  if (page_ == PAGE_TABLES && (pressed_ || tables_animating(now_))) return 25;  // smooth while dragging
  if (!s_.lights_on && !pressed_ && (now_ - last_user_ > 20000 || last_user_ == 0)) return 2;
  if (is_dozing(now_) && !pressed_) return 4;
  return 15;
}

float PetEngine::backlight(uint32_t now) const {
  static const float LEVELS[4] = {0.3f, 0.55f, 0.8f, 1.0f};
  float on = LEVELS[brightness_level()];
  bool recent = last_user_ != 0 && now - last_user_ < 20000;
  if (!s_.lights_on) return recent ? fminf(0.6f, on) : 0.1f;  // a touch in the dark lights it up for a while
  if (is_dozing(now)) return fmaxf(0.12f, on * 0.35f);       // dim while it dozes
  return on;
}

void PetEngine::set_sound_enabled(bool on) {
  s_.settings = on ? (uint8_t) (s_.settings & ~1) : (uint8_t) (s_.settings | 1);
  if (!on) sound_ = nullptr;
  dirty_ = true;
}

void PetEngine::set_brightness_level(int level) {
  level = level < 0 ? 0 : (level > 3 ? 3 : level);
  s_.brightness = (uint8_t) (level + 1);
  dirty_ = true;
}

// The New egg button on the settings page.
static const int EGG_BTN_X = 180, EGG_BTN_Y = 268, EGG_BTN_W = 190, EGG_BTN_H = 46;
static const uint32_t EGG_HOLD_MS = 2000;

float PetEngine::egg_hold_progress(uint32_t now) const {
  if (!pressed_ || page_ != 3 || long_fired_) return 0;
  if (abs(tx0_ - EGG_BTN_X) > EGG_BTN_W / 2 || abs(ty0_ - EGG_BTN_Y) > EGG_BTN_H / 2) return 0;
  if (abs(tx_ - tx0_) > 40 || abs(ty_ - ty0_) > 40) return 0;  // slid off: cancelled
  return fminf(1.0f, (float) (now - press_ms_) / EGG_HOLD_MS);
}

// ---------------------------------------------------------------------------------------------
// Actions

bool PetEngine::feed_meal() {
  overlay_ = OV_NONE;
  if (s_.dead || s_.stage == STAGE_EGG) return false;
  if (s_.asleep) {
    show_toast("Zzz...");
    return false;
  }
  if (s_.food >= 95) {
    set_activity(ACT_REFUSE, 1200);
    show_toast("Not hungry!");
    return false;
  }
  if (disobeys("meal")) return false;
  add(s_.food, 25);
  sound(SND_EAT);
  s_.weight++;
  if (s_.poop_timer == 0) s_.poop_timer = 75 + rnd() % 75;  // 5-10 minutes later, with you
  eat_icon_ = art::MEAL;
  set_activity(ACT_EAT, 3000);
  dirty_ = true;
  return true;
}

bool PetEngine::feed_snack() {
  overlay_ = OV_NONE;
  if (s_.dead || s_.stage == STAGE_EGG) return false;
  if (s_.asleep) {
    show_toast("Zzz...");
    return false;
  }
  if (disobeys("snack")) return false;
  add(s_.happy, 15);
  add(s_.food, 5);
  s_.weight += 1;
  sound(SND_EAT);
  if (++s_.snacks_hour > 3) {
    add(s_.health, -5);
    show_toast("Tummy ache!");
  }
  eat_icon_ = art::SNACK;
  set_activity(ACT_EAT, 3000);
  dirty_ = true;
  return true;
}

// The Play button opens the game menu; picking one starts it.
bool PetEngine::open_games() {
  if (s_.dead || s_.stage == STAGE_EGG) return false;
  if (s_.asleep) {
    show_toast("Zzz...");
    return false;
  }
  if (s_.sick || s_.energy < 15) {
    set_activity(ACT_REFUSE, 1200);
    show_toast(s_.sick ? "Too sick to play" : "Too tired...");
    return false;
  }
  if (disobeys("play")) return false;
  overlay_ = OV_GAMES;
  overlay_since_ = now_;
  return true;
}

bool PetEngine::start_game(int kind) {
  if (s_.dead || s_.stage == STAGE_EGG || s_.asleep || kind < 0 || kind >= GAME_COUNT) return false;
  overlay_ = OV_GAME;
  game_kind_ = (uint8_t) kind;
  act_ = ACT_IDLE;
  run_phase_ = RUN_READY;
  run_t_ = now_;
  run_y_ = run_vy_ = 0;
  run_speed_ = kind == GAME_JUMP ? RUN_START_SPEED : 130;  // catch: how fast things fall
  run_scroll_ = 0;
  run_next_ = RUN_START_SPEED * 1.3f;
  run_score_ = 0;
  run_stars_ = 0;
  run_new_best_ = false;
  lives_ = (kind == GAME_FIZZ || kind == GAME_MUNCH) ? 5 : 3;  // the math games are kinder
  catch_x_ = catch_target_ = 180;
  copy_len_ = copy_idx_ = 0;
  lit_pad_ = -1;
  for (auto &o : obs_) o.active = false;
  if (kind == GAME_CHANT) {  // no "tap to start": it opens on picking a table
    run_phase_ = RUN_GO;
    ch_state_ = 0;
    ch_epoch_++;
    ch_ok_ms_ = now_ ? now_ : 1;
    ch_heard_[0] = 0;
    ch_err_until_ = 0;
  }
  return true;
}

int PetEngine::best_score(int kind) const {
  switch (kind) {
    case GAME_CATCH: return s_.best_catch;
    case GAME_POP: return s_.best_pop;
    case GAME_COPY: return s_.best_copy;
    case GAME_QUIZ: return s_.best_quiz;
    case GAME_FACTS: return s_.best_facts;
    case GAME_FIZZ: return s_.best_fizz;
    case GAME_MUNCH: return s_.best_munch;
    case GAME_SHARE: return s_.best_share;
    case GAME_CHANT: return s_.best_chant;
    default: return s_.best_run;
  }
}

void PetEngine::game_over(const char *tune) {
  if (run_phase_ == RUN_OVER) return;
  sound(tune);
  run_phase_ = RUN_OVER;
  run_t_ = now_;
  run_new_best_ = run_score_ > best_score(game_kind_);
}

void PetEngine::finish_game() {
  overlay_ = OV_NONE;
  int sc = run_score_;
  uint16_t *best_slot = game_kind_ == GAME_CATCH ? &s_.best_catch
                        : game_kind_ == GAME_POP ? &s_.best_pop
                        : game_kind_ == GAME_COPY ? &s_.best_copy
                        : game_kind_ == GAME_QUIZ ? &s_.best_quiz
                        : game_kind_ == GAME_FACTS ? &s_.best_facts
                        : game_kind_ == GAME_FIZZ ? &s_.best_fizz
                        : game_kind_ == GAME_MUNCH ? &s_.best_munch
                        : game_kind_ == GAME_SHARE ? &s_.best_share
                        : game_kind_ == GAME_CHANT ? &s_.best_chant
                                                  : &s_.best_run;
  bool best = sc > *best_slot;
  if (best) *best_slot = (uint16_t) sc;
  if (s_.runs < 65535) s_.runs++;
  // Points are worth different amounts per game: a Copy round is harder than a popped bubble.
  int fun = game_kind_ == GAME_COPY   ? sc * 5 + 8
            : (is_quiz_like(game_kind_) || game_kind_ == GAME_FIZZ || game_kind_ == GAME_MUNCH) ? sc * 3 + 8
            : game_kind_ == GAME_POP  ? sc + 8
                                      : sc * 2 + 8;
  add(s_.happy, fun > 35 ? 35 : fun);
  bool math = is_math_game(game_kind_);  // brain work tires it less
  add(s_.energy, math ? -(3 + (sc / 4 > 5 ? 5 : sc / 4)) : -(5 + (sc / 3 > 10 ? 10 : sc / 3)));
  bool active = game_kind_ == GAME_JUMP || game_kind_ == GAME_CATCH;
  if (active && sc >= 5 && s_.weight > base_weight(s_.stage)) s_.weight--;
  // Coins for the shop, scaled like the fun: about 5-15 for a decent game.
  int earned = game_kind_ == GAME_COPY ? sc * 2 : game_kind_ == GAME_POP ? sc / 3 : math ? sc : sc / 2;
  if (sc > 0 && earned == 0) earned = 1;
  s_.coins = (uint16_t) (s_.coins + earned > 9999 ? 9999 : s_.coins + earned);
  if (sc >= 3) {
    set_activity(ACT_HAPPY, 1800);
    sound(SND_HAPPY);
  }
  char msg[40];
  if (best && sc > 0)
    snprintf(msg, sizeof(msg), "New best %d! +%dc", sc, earned);
  else
    snprintf(msg, sizeof(msg), "Score %d  +%d coins", sc, earned);
  show_toast(msg);

  // Prizes only the math games give. A newly won one replaces the score message.
  bool won[PRIZE_COUNT] = {game_kind_ == GAME_QUIZ && sc >= 10,  game_kind_ == GAME_FACTS && sc >= 20,
                           game_kind_ == GAME_FIZZ && sc >= 15,  s_.smarts_x10 >= 600,
                           game_kind_ == GAME_MUNCH && sc >= 15, game_kind_ == GAME_MUNCH && sc >= 25};
  for (int i = 0; i < PRIZE_COUNT; i++) {
    int id = IT_WIZARD_HAT + i;
    if (!won[i] || owns(id)) continue;
    give(id);
    if (is_hat(id)) s_.hat = (uint8_t) id;  // put it straight on
    snprintf(msg, sizeof(msg), "Prize: %s!", SHOP[id].name);
    show_toast(msg, 5000);
    sound(SND_GROW, true);
  }
  // After any math game, however it went, the prize stand opens.
  if (is_math_game(game_kind_)) {
    overlay_ = OV_STAND;
    overlay_since_ = now_;
    shop_sel_ = -1;
    stand_page_ = (uint8_t) ((game_kind_ - GAME_QUIZ) % STAND_PAGES);
    stand_bought_ = false;  // one buy for this game
    stand_got_ = -1;
  }
  dirty_ = true;
}

// --- Jump game: the pet runs right, obstacles scroll in; tap to jump, hold for a higher jump. ---

static void obstacle_box(uint8_t kind, int &w, int &h, int &bottom_above_ground) {
  switch (kind) {
    case 0: w = 36; h = 24; bottom_above_ground = 0; break;    // rock
    case 1: w = 30; h = 30; bottom_above_ground = 0; break;    // mushroom
    case 2: w = 42; h = 30; bottom_above_ground = 0; break;    // bush
    case 3: w = 48; h = 32; bottom_above_ground = 72; break;   // bird (drawn 4x): run under it, don't jump
    default: w = 27; h = 27; bottom_above_ground = 128; break; // star: needs a held jump
  }
}

// Copy game pads: centres, colours and tones.
static const int PAD_OFF = 62, PAD_R = 54;
static const uint32_t PAD_COLORS[4] = {0xF0454B, 0x4AA8FF, 0xFFD23F, 0x5BD46B};
static const char *const PAD_TONES[4] = {"p0:d=8,o=5,b=180:c6", "p1:d=8,o=5,b=180:e6", "p2:d=8,o=5,b=180:g6",
                                         "p3:d=8,o=5,b=180:c7"};
static const char *const SND_POP = "pop:d=32,o=7,b=250:c";
static const char *const SND_CATCH = "catch:d=32,o=6,b=250:e,g";
static const char *const SND_LOSE = "lose:d=16,o=5,b=180:e,c";
static const char *const SND_KEY = "key:d=32,o=6,b=250:g";
static const char *const SND_GOAL = "goal:d=16,o=6,b=200:g,p,g,c7";
static const uint32_t MUNCH_BANNER_MS = 2000;

// Times-tables games: the quiz's four answer circles, and the facts keypad (1-9, then delete, 0, skip).
static const int QUIZ_X[4] = {125, 235, 125, 235}, QUIZ_Y[4] = {176, 176, 268, 268}, QUIZ_R = 42;
static const uint32_t QUIZ_TIME_MS = 20000;  // per question

// The game menu: rows of three, four and three.
static const int MENU_R = 34;
static void menu_pos(int i, int &x, int &y) {
  if (i < 3) {
    y = 104;
    x = 98 + i * 82;
  } else if (i < 7) {
    y = 182;
    x = 60 + (i - 3) * 80;
  } else {
    y = 260;
    x = 98 + (i - 7) * 82;
  }
}
// Facts keypad: twelve round keys round the edge like a clock, digits 1-9 at one to nine o'clock, 0 at ten,
// delete at eleven and skip at twelve. A touch anywhere in the outer ring counts for the key it is nearest by
// angle, so a slightly missed key still counts and the next key is a whole 30 degrees away.
static const int FK_R = 142, FK_KEY_R = 30, FK_INNER = 78;
static int fk_slot(int key) { return key == 0 ? 10 : key == 10 ? 11 : key == 11 ? 12 : key; }
void PetEngine::facts_key_pos(int key, int &x, int &y, int &w) {
  float a = fk_slot(key) * 3.14159265f / 6.0f;
  x = 180 + (int) lroundf(FK_R * sinf(a));
  y = 180 - (int) lroundf(FK_R * cosf(a));
  w = 2 * FK_KEY_R;
}
// Which key a touch is for, or -1 (the middle, where the question is).
static int facts_key_at(int x, int y) {
  int dx = x - 180, dy = y - 180;
  if (dx * dx + dy * dy < FK_INNER * FK_INNER) return -1;
  float a = atan2f((float) dx, (float) -dy);  // 0 at twelve o'clock, clockwise
  if (a < 0) a += 2 * 3.14159265f;
  int slot = (int) lroundf(a / (3.14159265f / 6.0f));
  if (slot == 0) slot = 12;
  return slot == 10 ? 0 : slot == 11 ? 10 : slot == 12 ? 11 : slot;
}
static void pad_center(int i, int &x, int &y) {
  x = 180 + (i % 2 ? PAD_OFF : -PAD_OFF);
  y = 180 + (i / 2 ? PAD_OFF : -PAD_OFF);
}

// A finger came down during a game.
void PetEngine::game_press(int x, int y) {
  if (game_kind_ == GAME_CHANT) {
    chant_press(x, y);
    return;
  }
  if (run_phase_ == RUN_OVER) {
    if (now_ - run_t_ > 700) finish_game();
    return;
  }
  if (run_phase_ == RUN_READY) {
    run_phase_ = RUN_GO;
    run_t_ = now_;
    next_spawn_ms_ = now_ + 300;
    game_end_ms_ = now_ + 30000;  // pop: 30 seconds
    if (game_kind_ == GAME_COPY) {
      copy_add_step();
      copy_state_ = COPY_SHOW;
      copy_idx_ = 0;
      copy_t_ = now_ + 600;
      return;
    }
    if (is_quiz_like(game_kind_) || game_kind_ == GAME_FACTS || game_kind_ == GAME_FIZZ) {
      if (game_kind_ == GAME_FACTS) game_end_ms_ = now_ + 90000;  // facts: 90 seconds
      if (game_kind_ == GAME_FIZZ) run_speed_ = 25;               // fizz: how fast bubbles rise, px/s (~11 s up)
      q_num_ = 0;
      math_new_question();
      if (game_kind_ == GAME_FIZZ) fizz_spawn();
      return;
    }
    if (game_kind_ == GAME_MUNCH) {
      run_speed_ = 30;  // how fast the food falls, px/s
      munch_new_target();
      catch_target_ = (float) x;
      return;
    }
    if (game_kind_ == GAME_POP) return;  // the starting tap doesn't pop anything
  }
  switch (game_kind_) {
    case GAME_JUMP:
      if (run_y_ <= 0) {
        run_vy_ = RUN_JUMP_V;
        run_y_ = 0.01f;
        run_jump_ms_ = now_;
        sound(SND_JUMP);
      }
      break;
    case GAME_CATCH:
    case GAME_MUNCH:
      catch_target_ = (float) x;
      break;
    case GAME_FIZZ:
      if (feedback_ != 0) break;
      for (auto &o : obs_) {
        if (!o.active || o.passed || sq(x - (int) o.x) + sq(y - (int) o.y) > 34 * 34) continue;
        o.passed = true;  // popped
        o.t = now_;
        if (o.value == mans_) {
          math_answered(true, 350);  // a moment for the pop, then the next question
          run_speed_ = fminf(60, run_speed_ + 1.5f);
        } else {
          o.kind = OB_SPIKY;  // shown as a red burst
          sound(SND_LOSE);
          if (--lives_ == 0) game_over(SND_CRASH);
        }
        break;
      }
      break;
    case GAME_POP:
      for (auto &o : obs_) {
        if (!o.active || o.passed) continue;
        if (sq(x - (int) o.x) + sq(y - (int) o.y) > 36 * 36) continue;
        o.passed = true;  // popped
        o.t = now_;
        if (o.kind == OB_SPIKY) {
          run_score_ = run_score_ >= 3 ? run_score_ - 3 : 0;
          sound(SND_LOSE);
        } else {
          run_score_++;
          sound(SND_POP);
        }
        break;
      }
      break;
    case GAME_COPY: {
      if (copy_state_ != COPY_INPUT) break;
      int pad = -1;
      for (int i = 0; i < 4; i++) {
        int px, py;
        pad_center(i, px, py);
        if (sq(x - px) + sq(y - py) < (PAD_R + 8) * (PAD_R + 8)) pad = i;
      }
      if (pad < 0) break;
      lit_pad_ = (int8_t) pad;
      lit_until_ = now_ + 220;
      if (pad != seq_[copy_idx_]) {
        game_over(SND_CRASH);
        break;
      }
      sound(PAD_TONES[pad]);
      copy_t_ = now_ + 6000;  // time allowed for the next tap
      if (++copy_idx_ >= copy_len_) {
        run_score_ = copy_len_;
        copy_state_ = COPY_PAUSE;
        copy_t_ = now_ + 800;
      }
      break;
    }
    case GAME_QUIZ:
    case GAME_SHARE:
      if (feedback_ != 0) break;
      for (int i = 0; i < 4; i++) {
        if (sq(x - QUIZ_X[i]) + sq(y - QUIZ_Y[i]) > (QUIZ_R + 6) * (QUIZ_R + 6)) continue;
        picked_ = (int8_t) i;
        math_answered(choices_[i] == mans_, choices_[i] == mans_ ? 700 : 1600);
        break;
      }
      break;
    case GAME_FACTS:
      if (feedback_ != 0) break;
      for (int k = facts_key_at(x, y); k >= 0;) {
        if (k == 10) {  // delete
          entry_ /= 10;
          if (entry_len_) entry_len_--;
        } else if (k == 11) {  // skip: show the answer, no point
          math_answered(false, 1400);
        } else if (entry_len_ < 3) {
          int digit = k;
          entry_ = (uint16_t) (entry_ * 10 + digit);
          entry_len_++;
          sound(SND_KEY);
          int need = mans_ >= 100 ? 3 : (mans_ >= 10 ? 2 : 1);
          if (entry_len_ == need) math_answered(entry_ == mans_, entry_ == mans_ ? 450 : 1600);
        }
        break;
      }
      break;
  }
}

// --- Times tables: Quiz (pick one of four, 10 questions) and Facts (type it, 60 seconds). ---

// A question from the tables a third grader learns, easy tables first: x2, x5, x10, then x3, x4, then all to x10.
void PetEngine::math_new_question() {
  // Quiz moves up a level every 3 questions; the others by right answers, more slowly.
  bool quiz = is_quiz_like(game_kind_);
  int score = quiz ? q_num_ : run_score_;
  int level = quiz ? (score < 3 ? 0 : score < 6 ? 1 : 2) : (score < 5 ? 0 : score < 12 ? 1 : 2);
  static const uint8_t EASY[3] = {2, 5, 10}, MED[5] = {2, 3, 4, 5, 10};
  int a, b;
  for (int tries = 0; tries < 8; tries++) {
    a = level == 0 ? EASY[rnd() % 3] : level == 1 ? MED[rnd() % 5] : 2 + (int) (rnd() % 9);
    b = level < 2 ? 1 + (int) (rnd() % 10) : 2 + (int) (rnd() % 9);
    if (game_kind_ == GAME_SHARE) {
      // Division: (a x b) / b = a. The divisor b comes from the tables being learned (easy ones first);
      // never divide by 1.
      int t = a;
      a = b < 1 ? 1 : b;
      b = t < 2 ? 2 : t;
    } else if (rnd() % 2) {
      int t = a;
      a = b;
      b = t;
    }
    if (!(a == ma_ && b == mb_) && !(a == mb_ && b == ma_)) break;  // not the same one twice in a row
  }
  ma_ = (uint8_t) a;
  mb_ = (uint8_t) b;
  // Fizz, later on: "? x b = p" sometimes, so the answer is the missing factor a.
  missing_ = game_kind_ == GAME_FIZZ && score >= 15 && rnd() % 2;
  bool factor_answer = missing_ || game_kind_ == GAME_SHARE;  // the answer is a factor, not a product
  mans_ = (uint16_t) (factor_answer ? a : a * b);
  q_t_ = now_;
  feedback_ = 0;
  picked_ = -1;
  entry_ = 0;
  entry_len_ = 0;
  if (!is_quiz_like(game_kind_) && game_kind_ != GAME_FIZZ) return;
  // Wrong answers that look right: a neighbouring row or column, or off by a little.
  int cand[8] = {a * (b + 1), a * (b - 1), (a + 1) * b, (a - 1) * b, mans_ + 1, mans_ - 1, mans_ + 2, mans_ + 10};
  if (factor_answer) {  // other numbers near the right one, and the divisor itself (a common slip)
    int near[8] = {a + 1, a - 1, a + 2, a - 2, b, a + 3, a - 3, 10 - a};
    for (int i = 0; i < 8; i++) cand[i] = near[i];
  }
  int n = 0, picks[3];
  auto usable = [&](int v) {
    bool ok = v > 0 && v != mans_;
    for (int j = 0; j < n; j++) ok = ok && picks[j] != v;
    return ok;
  };
  for (int tries = 0; tries < 40 && n < 3; tries++) {
    int v = cand[rnd() % 8];
    if (usable(v)) picks[n++] = v;
  }
  for (int filler = mans_ + 3; n < 3; filler++)  // (hardly ever needed)
    if (usable(filler)) picks[n++] = filler;
  int right = (int) (rnd() % 4);
  for (int i = 0, j = 0; i < 4; i++) choices_[i] = (uint16_t) (i == right ? mans_ : picks[j++]);
}

void PetEngine::math_answered(bool right, uint32_t show_ms) {
  feedback_ = right ? 1 : 2;
  fb_until_ = now_ + show_ms;
  if (right) {
    run_score_++;
    learn();
  }
  sound(right ? SND_CATCH : SND_LOSE);
}

void PetEngine::math_update() {
  if (game_kind_ == GAME_FACTS && (int32_t) (now_ - game_end_ms_) >= 0) {
    game_over(SND_HAPPY);  // time's up
    return;
  }
  if (feedback_ == 0) {
    if (is_quiz_like(game_kind_) && now_ - q_t_ > QUIZ_TIME_MS) math_answered(false, 1600);  // too slow
    return;
  }
  if ((int32_t) (now_ - fb_until_) < 0) return;
  if (is_quiz_like(game_kind_) && ++q_num_ >= 10) {
    game_over(SND_HAPPY);
    return;
  }
  math_new_question();
}

// --- Catch: drag the pet under falling fruit; rocks and dropped fruit cost a life. ---

void PetEngine::catch_move(float dt) {
  float step = 700 * dt;
  if (catch_x_ < catch_target_) catch_x_ = fminf(catch_x_ + step, catch_target_);
  if (catch_x_ > catch_target_) catch_x_ = fmaxf(catch_x_ - step, catch_target_);
  catch_x_ = fmaxf(70, fminf(290, catch_x_));
}

// A right answer in a math game. Smarts climb fast at first and slower near the top: 1.2 points per answer
// from zero, 0.3 near 100. About 90 right answers reach 60, about 180 reach 100.
void PetEngine::learn() {
  s_.math_need = s_.math_need > 5 ? (uint8_t) (s_.math_need - 5) : 0;  // each right answer feeds the Math need
  int x = s_.smarts_x10 + 2 + (1000 - s_.smarts_x10) / 100;
  s_.smarts_x10 = (uint16_t) (x > 1000 ? 1000 : x);
  dirty_ = true;
}

void PetEngine::math_practice(int right) {
  for (int i = 0; i < right; i++) learn();
}

// --- Fizz: number bubbles float up; pop the one that answers the question. A wrong pop, or letting the
// right one float away, costs one of three lives. ---

void PetEngine::fizz_spawn() {
  for (auto &o : obs_) o.active = false;
  int order[4] = {0, 1, 2, 3};
  for (int i = 3; i > 0; i--) {  // stagger the start heights in a random order
    int j = (int) (rnd() % (i + 1)), t = order[i];
    order[i] = order[j];
    order[j] = t;
  }
  for (int i = 0; i < 4; i++) {
    float x = 70 + i * 73 + (float) ((int) (rnd() % 17) - 8);
    obs_[i] = Obstacle{x, 345.0f + order[i] * 34, OB_BUBBLE, true, false, now_, 0, choices_[i]};
  }
}

void PetEngine::fizz_update(float dt) {
  if (feedback_ && (int32_t) (now_ - fb_until_) >= 0) {  // after a right pop: the next question
    math_new_question();
    fizz_spawn();
    return;
  }
  for (auto &o : obs_) {
    if (!o.active) continue;
    if (o.passed) {
      if (now_ - o.t > 250) o.active = false;  // the burst is over
      continue;
    }
    o.y -= run_speed_ * dt;
    if (o.y < 64 && feedback_ == 0) {
      o.active = false;
      if (o.value == mans_) {  // the answer got away
        sound(SND_LOSE);
        if (--lives_ == 0) {
          game_over(SND_CRASH);
          return;
        }
        feedback_ = 2;
        fb_until_ = now_ + 900;
      }
    }
  }
}

// --- Munch: "Feed me 24!" Food falls with facts on it; drag the pet under the ones that make the target.
// A wrong one gives it a tummy ache (one of three lives). Four right ones and the target changes. ---

void PetEngine::munch_new_target() {
  static const uint8_t EASY[6] = {6, 8, 10, 12, 15, 20}, MED[6] = {12, 16, 18, 20, 24, 30},
                       HARD[9] = {24, 36, 40, 48, 18, 28, 42, 56, 32};
  uint16_t t;
  do {
    t = run_score_ < 8 ? EASY[rnd() % 6] : run_score_ < 16 ? MED[rnd() % 6] : HARD[rnd() % 9];
  } while (t == munch_target_);
  munch_target_ = t;
  munch_got_ = 0;
  // Announce it: a banner across the middle for 2 seconds while the food waits.
  goal_flash_until_ = now_ + MUNCH_BANNER_MS;
  next_spawn_ms_ = goal_flash_until_ + 400;
  sound(SND_GOAL);
}

void PetEngine::munch_update(float dt) {
  catch_move(dt);
  if ((int32_t) (now_ - goal_flash_until_) < 0) return;  // the new-goal banner is up: everything waits
  if ((int32_t) (now_ - next_spawn_ms_) >= 0) {
    for (auto &o : obs_) {
      if (o.active) continue;
      int a = 0, b = 0;
      if (rnd() % 2) {  // a right one: any way to make the target from the tables
        int pairs[10][2], n = 0;
        for (int f = 1; f <= 10; f++)
          if (munch_target_ % f == 0 && munch_target_ / f <= 10) {
            pairs[n][0] = f;
            pairs[n][1] = munch_target_ / f;
            n++;
          }
        int p = (int) (rnd() % n);
        a = pairs[p][0];
        b = pairs[p][1];
      } else {  // a near miss
        for (int tries = 0; tries < 20; tries++) {
          a = 2 + (int) (rnd() % 9);
          b = 2 + (int) (rnd() % 9);
          int d = a * b - munch_target_;
          if (d != 0 && d >= -12 && d <= 12) break;
        }
        if (a * b == munch_target_) b = b < 10 ? b + 1 : b - 1;
      }
      // Keep it clear of the food already falling high up, so the right ones can be reached on their own.
      float x = 80;
      for (int tries = 0; tries < 10; tries++) {
        x = (float) (80 + rnd() % 200);
        bool clear = true;
        for (auto &p : obs_)
          if (&p != &o && p.active && p.y < 150 && fabsf(p.x - x) < 70) clear = false;
        if (clear) break;
      }
      o = Obstacle{x, 40, OB_APPLE, true, false, now_, 0, (uint16_t) (a * 16 + b)};
      break;
    }
    int gap = 2000 - run_score_ * 20;
    next_spawn_ms_ = now_ + (uint32_t) (gap < 1300 ? 1300 : gap);
  }
  float rx, ry;
  body_size(s_.g, s_.stage, s_.form, rx, ry);
  float half = 20, pet_top = GROUND_Y - 4.5f - 2 * ry * 3;  // just its mouth: the food has to be right above it
  for (auto &o : obs_) {
    if (!o.active) continue;
    o.y += run_speed_ * dt;
    float bottom = o.y + 24;  // o.y is the centre; radius 24
    if (bottom >= pet_top + 6 && bottom <= GROUND_Y && fabsf(o.x - catch_x_) < half) {
      o.active = false;
      int a = o.value / 16, b = o.value % 16;
      if (a * b == munch_target_) {
        run_score_++;
        learn();
        run_speed_ = fminf(70, run_speed_ + 1);
        sound(SND_EAT);
        if (++munch_got_ >= 4) munch_new_target();
      } else {
        sound(SND_LOSE);
        if (--lives_ == 0) game_over(SND_CRASH);
      }
      continue;
    }
    if (o.y - 24 > GROUND_Y) o.active = false;  // fell past: no harm
  }
}

void PetEngine::catch_update(float dt) {
  catch_move(dt);

  if ((int32_t) (now_ - next_spawn_ms_) >= 0) {
    for (auto &o : obs_) {
      if (o.active) continue;
      int r = (int) (rnd() % 100);
      ObstacleKind k = (run_score_ >= 2 && r < 20) ? OB_ROCK : (r < 32 ? OB_CAKE : OB_APPLE);
      o = Obstacle{(float) (80 + rnd() % 200), 40, k, true, false, now_, 0, 0};
      break;
    }
    int gap = 950 - run_score_ * 15;
    next_spawn_ms_ = now_ + (uint32_t) (gap < 420 ? 420 : gap);
  }

  float rx, ry;
  body_size(s_.g, s_.stage, s_.form, rx, ry);
  float half = rx * 3 + 12;
  float pet_top = GROUND_Y - 4.5f - 2 * ry * 3;
  for (auto &o : obs_) {
    if (!o.active) continue;
    o.y += run_speed_ * (o.kind == OB_CAKE ? 1.2f : 1.0f) * dt;
    float h = o.kind == OB_ROCK ? 24 : 36, bottom = o.y + h, cx = o.x + 18;
    if (bottom >= pet_top + 6 && bottom <= GROUND_Y && fabsf(cx - catch_x_) < half) {
      o.active = false;
      if (o.kind == OB_ROCK) {
        sound(SND_LOSE);
        if (--lives_ == 0) game_over(SND_CRASH);
      } else {
        run_score_ += o.kind == OB_CAKE ? 2 : 1;
        run_speed_ = fminf(330, run_speed_ + 4);
        sound(SND_CATCH);
      }
      continue;
    }
    if (bottom >= GROUND_Y + 4) {
      o.active = false;
      if (o.kind != OB_ROCK) {
        sound(SND_LOSE);
        if (--lives_ == 0) game_over(SND_CRASH);
      }
    }
  }
}

// --- Pop: tap bubbles before they vanish; the red spiky ones cost 3. 30 seconds. ---

void PetEngine::pop_update() {
  if ((int32_t) (now_ - game_end_ms_) >= 0) {
    game_over(SND_HAPPY);
    return;
  }
  int live = 0;
  for (auto &o : obs_) {
    if (!o.active) continue;
    if (o.passed) {
      if (now_ - o.t > 250) o.active = false;  // the burst is over
    } else if (now_ - o.t > o.life_ms) {
      o.active = false;  // got away
    } else {
      live++;
    }
  }
  if (live < (run_score_ < 10 ? 2 : 3) && (int32_t) (now_ - next_spawn_ms_) >= 0) {
    for (auto &o : obs_) {
      if (o.active) continue;
      float bx = 180, by = 185;
      for (int tries = 0; tries < 8; tries++) {  // somewhere in the circle, clear of other bubbles
        float a = (rnd() % 628) / 100.0f, r = 10 + (float) (rnd() % 105);
        bx = 180 + cosf(a) * r;
        by = 190 + sinf(a) * r * 0.85f;
        bool clear = true;
        for (auto &p : obs_)
          if (&p != &o && p.active && (p.x - bx) * (p.x - bx) + (p.y - by) * (p.y - by) < 70 * 70) clear = false;
        if (clear) break;
      }
      ObstacleKind k = (run_score_ >= 5 && rnd() % 100 < 20) ? OB_SPIKY : OB_BUBBLE;
      int life = 1500 - run_score_ * 25;
      o = Obstacle{bx, by, k, true, false, now_, (uint16_t) (life < 700 ? 700 : life), 0};
      break;
    }
    next_spawn_ms_ = now_ + 250 + rnd() % 400;
  }
}

// --- Copy: watch the pads light up, then tap them in the same order; one more each round. ---

void PetEngine::copy_add_step() {
  if (copy_len_ < sizeof(seq_)) seq_[copy_len_++] = (uint8_t) (rnd() % 4);
}

void PetEngine::copy_update() {
  int on_ms = 520 - copy_len_ * 20;
  if (on_ms < 250) on_ms = 250;
  switch (copy_state_) {
    case COPY_SHOW:
      if ((int32_t) (now_ - copy_t_) < 0) break;
      if (copy_idx_ < copy_len_) {
        lit_pad_ = (int8_t) seq_[copy_idx_];
        lit_until_ = now_ + on_ms;
        sound(PAD_TONES[seq_[copy_idx_]]);
        copy_idx_++;
        copy_t_ = now_ + on_ms + 150;
      } else {
        copy_state_ = COPY_INPUT;
        copy_idx_ = 0;
        copy_t_ = now_ + 8000;
      }
      break;
    case COPY_INPUT:
      if ((int32_t) (now_ - copy_t_) >= 0) game_over(SND_CRASH);  // too slow
      break;
    case COPY_PAUSE:
      if ((int32_t) (now_ - copy_t_) < 0) break;
      copy_add_step();
      copy_state_ = COPY_SHOW;
      copy_idx_ = 0;
      copy_t_ = now_ + 300;
      break;
  }
}

void PetEngine::run_spawn() {
  Obstacle *slot = nullptr;
  for (auto &o : obs_)
    if (!o.active) {
      slot = &o;
      break;
    }
  if (slot == nullptr) {
    run_next_ = 40;
    return;
  }
  int r = (int) (rnd() % 100);
  ObstacleKind kind;
  if (run_score_ >= 8 && r < (run_score_ >= 25 ? 30 : 20))
    kind = OB_BIRD;
  else if (run_score_ >= 2 && r < 36)
    kind = OB_STAR;
  else
    kind = (ObstacleKind) (rnd() % (run_score_ < 4 ? 2 : 3));
  int w, h, b;
  obstacle_box(kind, w, h, b);
  *slot = Obstacle{(float) (SCREEN + 12), 0, kind, true, false, now_, 0, 0};
  float gap_s = kind == OB_STAR ? 0.7f : 0.95f + (rnd() % 100) / 100.0f * 0.8f;
  if (kind != OB_STAR && run_score_ >= 12 && rnd() % 100 < 25) gap_s = 0.72f;  // a quick pair: land and jump again
  run_next_ = run_speed_ * gap_s + w;
}

void PetEngine::run_update(float dt) {
  if (run_phase_ != RUN_GO) return;

  // Jump physics; gravity is lighter while the finger stays down early in the jump.
  if (run_y_ > 0) {
    bool hold = pressed_ && now_ - run_jump_ms_ < 170;
    run_vy_ -= RUN_GRAVITY * (hold ? 0.4f : 1.0f) * dt;
    run_y_ += run_vy_ * dt;
    if (run_y_ <= 0) run_y_ = run_vy_ = 0;
  }

  float dx = run_speed_ * dt;
  run_scroll_ += dx;
  run_next_ -= dx;
  if (run_next_ <= 0) run_spawn();

  float rx, ry;
  body_size(s_.g, s_.stage, s_.form, rx, ry);
  float half = rx * 3 * 0.55f;
  float pl = RUN_PET_X - half, pr = RUN_PET_X + half;
  float pb = GROUND_Y - 4.5f - run_y_, pt = pb - 2 * ry * 3 * 0.85f;

  for (auto &o : obs_) {
    if (!o.active) continue;
    o.x -= dx;
    int w, h, b;
    obstacle_box(o.kind, w, h, b);
    if (o.x + w < -10) {
      o.active = false;
      continue;
    }
    float m = o.kind == OB_STAR ? -4.0f : 7.0f;  // forgiving hit boxes; generous stars
    float ol = o.x + m, orr = o.x + w - m, ob = GROUND_Y - b - m, ot = GROUND_Y - b - h + m;
    if (pr > ol && pl < orr && pb > ot && pt < ob) {
      if (o.kind == OB_STAR) {
        o.active = false;
        run_stars_++;
        run_score_ += 3;
        sound(SND_STAR);
        continue;
      }
      game_over(SND_CRASH);
      return;
    }
    if (!o.passed && o.kind != OB_STAR && o.x + w < pl) {
      o.passed = true;
      run_score_++;
      run_speed_ = fminf(RUN_MAX_SPEED, run_speed_ + 7);
    }
  }
}

float PetEngine::next_obstacle(bool *flying) const {
  float best = -1;
  float rx, ry;
  body_size(s_.g, s_.stage, s_.form, rx, ry);
  float front = RUN_PET_X + rx * 3 * 0.55f;
  for (const auto &o : obs_) {
    if (!o.active || o.kind == OB_STAR || o.passed) continue;
    int w, h, b;
    obstacle_box(o.kind, w, h, b);
    if (o.x + w < RUN_PET_X - rx * 3) continue;
    float d = o.x - front;
    if (best < 0 || d < best) {
      best = d;
      if (flying) *flying = o.kind == OB_BIRD;
    }
  }
  return best;
}

bool PetEngine::clean_up() {
  overlay_ = OV_NONE;
  if (s_.dead || s_.stage == STAGE_EGG) return false;
  clean_poops_ = s_.poops;
  s_.poops = 0;
  s_.clean = 100;
  if (owns(IT_DUCK)) add(s_.happy, 15);  // bath time with the duck
  set_activity(ACT_CLEAN, 2200);
  dirty_ = true;
  return true;
}

bool PetEngine::heal() {
  overlay_ = OV_NONE;
  if (s_.dead || s_.stage == STAGE_EGG) return false;
  if (!s_.sick) {
    if (!s_.asleep) set_activity(ACT_REFUSE, 1200);
    add(s_.happy, -5);
    show_toast("Not sick!");
    dirty_ = true;
    return false;
  }
  s_.sick = 0;
  add(s_.health, 25);
  if (!s_.asleep) set_activity(ACT_HAPPY, 1500);
  show_toast("All better!");
  sound(SND_HAPPY);
  dirty_ = true;
  return true;
}

void PetEngine::toggle_light() {
  if (s_.dead) return;
  s_.lights_on = !s_.lights_on;
  int mod = clock_valid_ ? minute_of_day(epoch_) : -1;
  bool can_sleep = s_.stage != STAGE_EGG;
  if (!s_.lights_on && !s_.asleep && can_sleep) {
    // Lights off puts it to bed: energy (the bolt) refills while it sleeps.
    s_.asleep = 1;
    overlay_ = OV_NONE;
    act_ = ACT_IDLE;
    show_toast("Nap time", 2000);
  } else if (s_.lights_on && s_.asleep && !is_night(mod)) {
    s_.asleep = 0;
    show_toast("Awake!", 1500);
  } else {
    show_toast(s_.lights_on ? "Lights on" : "Lights off", 1500);
  }
  dirty_ = true;
}

bool PetEngine::scold() {
  overlay_ = OV_NONE;
  if (s_.dead || s_.stage == STAGE_EGG) return false;
  if (s_.asleep) {
    show_toast("Zzz...");
    return false;
  }
  sound(SND_SCOLD);
  set_activity(ACT_SULK, 1500);
  dirty_ = true;
  if (s_.tantrum) {
    s_.tantrum = 0;
    add(s_.discipline, 25);
    show_toast("OK... behaving");
    return true;
  }
  add(s_.happy, -10);  // it did nothing wrong
  show_toast("Why?! :(");
  return false;
}

void PetEngine::pat() {
  if (s_.dead || s_.stage == STAGE_EGG || s_.asleep) return;
  if (last_pat_ == 0 || now_ - last_pat_ > 8000) add(s_.happy, 3);
  last_pat_ = now_;
  set_activity(ACT_HAPPY, 1500);
  dirty_ = true;
}

// ---------------------------------------------------------------------------------------------
// Touch

void PetEngine::touch_down(int x, int y, uint32_t now) {
  now_ = now;
  // A touch on the pet page while it dozes just wakes it (and it says what it needs).
  wake_greet_ = page_ == 0 && overlay_ == OV_NONE && is_dozing(now);
  pressed_ = true;
  long_fired_ = false;
  tx0_ = tx_ = x;
  ty0_ = ty_ = y;
  press_ms_ = now;
  last_user_ = now ? now : 1;
  tt_drag_y_ = (int16_t) y;
  tt_lock_ = 0;
  tt_dragged_ = false;
  if (overlay_ == OV_GAME && page_ == 0) {
    game_touch_ = true;
    game_press(x, y);
  }
}

void PetEngine::touch_move(int x, int y) {
  tx_ = x;
  ty_ = y;
  if (overlay_ == OV_GAME && (game_kind_ == GAME_CATCH || game_kind_ == GAME_MUNCH) && pressed_)
    catch_target_ = (float) x;  // drag to move
  if (page_ == PAGE_TABLES && overlay_ == OV_NONE && pressed_ && ty0_ <= HOME_SWIPE_Y) {
    int dx = x - tx0_, dy = y - ty0_;
    if (tt_lock_ == 0 && (abs(dx) > 14 || abs(dy) > 14)) tt_lock_ = abs(dy) > abs(dx) ? 1 : -1;
    if (tt_lock_ == 1) {
      int steps = (tt_drag_y_ - y) / TT_DRAG_PX;  // up: the next tables, down: the ones before
      if (steps) {
        tables_step(steps);
        tt_drag_y_ = (int16_t) (tt_drag_y_ - steps * TT_DRAG_PX);
        tt_dragged_ = true;
      }
    }
  }
}

void PetEngine::touch_up(uint32_t now) {
  now_ = now;
  if (!pressed_) return;
  pressed_ = false;
  last_user_ = now ? now : 1;
  // A swipe up from the bottom edge goes home from anywhere: any page, menu or game.
  if (ty0_ > HOME_SWIPE_Y && ty_ - ty0_ < -90 && abs(ty_ - ty0_) > 2 * abs(tx_ - tx0_)) {
    wake_greet_ = false;
    game_touch_ = false;
    go_home();
    return;
  }
  if (wake_greet_) {
    wake_greet_ = false;
    greet();
    return;
  }
  if (game_touch_) {
    game_touch_ = false;
    return;
  }
  if (page_ == PAGE_TABLES && (tt_dragged_ || tt_lock_ == 1)) return;
  int dx = tx_ - tx0_, dy = ty_ - ty0_;
  if (abs(dx) > 70 && abs(dx) > abs(dy) && overlay_ == OV_NONE) {
    page_ = (uint8_t) ((page_ + (dx < 0 ? 1 : PAGE_COUNT - 1)) % PAGE_COUNT);
    if (page_ == PAGE_TABLES) tables_start_sweep();
    return;
  }
  if (!long_fired_) tap(tx0_, ty0_);
}

// Back to the pet's page with nothing open. A game in progress ends there: its points count (coins, best score),
// but the prize stand stays closed.
void PetEngine::go_home() {
  if (overlay_ == OV_GAME && run_phase_ != RUN_READY && run_score_ > 0) finish_game();
  overlay_ = OV_NONE;
  page_ = 0;
}

void PetEngine::long_press(int x, int y) {
  if (page_ == 0 && s_.dead) {
    hatch_new_egg(rnd() ^ now_);
    show_toast("A new egg!", 3000);
    return;
  }
  // Hold the pet: try on the next hat it owns.
  bool on_pet = abs(x - (180 + (int) walk_x_)) < 60 && y > 100 && y < GROUND_Y + 8;
  if (page_ == 0 && overlay_ == OV_NONE && on_pet && s_.stage != STAGE_EGG) next_hat();
}

// The owned hats in order, then none, then round again.
void PetEngine::next_hat() {
  int hats[IT_COUNT], n = 0, cur = -1;
  for (int id = 0; id < IT_COUNT; id++)
    if (is_hat(id) && owns(id)) {
      if (id == s_.hat) cur = n;
      hats[n++] = id;
    }
  if (n == 0) {
    show_toast("No hats yet", 1500);
    return;
  }
  int next = cur + 1;  // no hat now: the first one; the last one: no hat
  if (next >= n) {
    s_.hat = 0;
    show_toast("No hat", 1500);
  } else {
    s_.hat = (uint8_t) hats[next];
    char msg[32];
    snprintf(msg, sizeof(msg), "%s on!", SHOP[s_.hat].name);
    show_toast(msg, 1500);
  }
  dirty_ = true;
}

void PetEngine::tap(int x, int y) {
  if (page_ == PAGE_TABLES) {
    tables_tap(x, y);
    return;
  }
  if (page_ == 3) {
    settings_tap(x, y);
    return;
  }
  if (page_ == 2) {
    shop_tap(x, y);
    return;
  }
  if (page_ != 0) {
    page_ = 0;
    return;
  }
  if (overlay_ == OV_FEED) {
    if (sq(x - 122) + sq(y - 170) < 52 * 52) {
      feed_meal();
    } else if (sq(x - 238) + sq(y - 170) < 52 * 52) {
      feed_snack();
    } else {
      overlay_ = OV_NONE;
    }
    return;
  }
  if (overlay_ == OV_GAME) return;  // presses go to game_press() on touch-down
  if (overlay_ == OV_STAND) {
    stand_tap(x, y);
    return;
  }
  if (overlay_ == OV_GAMES) {
    for (int i = 0; i < GAME_COUNT; i++) {
      int gx, gy;
      menu_pos(i, gx, gy);
      if (sq(x - gx) + sq(y - gy) < (MENU_R + 4) * (MENU_R + 4)) {
        start_game(i);
        return;
      }
    }
    overlay_ = OV_NONE;  // tapped outside: cancel
    return;
  }
  if (s_.dead) {
    show_toast("Hold for a new egg");
    return;
  }
  for (int i = 0; i < 6; i++) {
    int bx, by;
    button_pos(i, bx, by);
    if (sq(x - bx) + sq(y - by) > (BTN_R + 6) * (BTN_R + 6)) continue;
    switch (i) {
      case 0:
        if (s_.stage == STAGE_EGG) break;
        if (s_.asleep) {
          show_toast("Zzz...");
          break;
        }
        overlay_ = OV_FEED;
        overlay_since_ = now_;
        break;
      case 1: open_games(); break;
      case 2: clean_up(); break;
      case 3: toggle_light(); break;
      case 4: heal(); break;
      case 5: scold(); break;
    }
    return;
  }
  if (y < 80) {
    page_ = 1;
    return;
  }
  if (s_.stage == STAGE_EGG) {
    show_toast("It feels warm...");
    return;
  }
  if (abs(x - (180 + (int) walk_x_)) < 60 && y > 100 && y < GROUND_Y + 8) pat();
}

// ---------------------------------------------------------------------------------------------
// Drawing primitives

static float g_dim = 1.0f;  // scene darkening while the lights are off

void PetEngine::rect(Canvas &c, int x, int y, int w, int h, uint32_t rgb) {
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > SCREEN) w = SCREEN - x;
  if (y + h > SCREEN) h = SCREEN - y;
  if (w <= 0 || h <= 0) return;
  if (g_dim < 1.0f) rgb = mix(0x000000, rgb, g_dim);
  c.fill(x, y, w, h, rgb);
}

void PetEngine::circle(Canvas &c, int cx, int cy, int r, uint32_t rgb) {
  for (int dy = -r; dy <= r; dy++) {
    int half = (int) sqrtf((float) (r * r - dy * dy));
    rect(c, cx - half, cy + dy, 2 * half + 1, 1, rgb);
  }
}

void PetEngine::pix_circle(Canvas &c, int cx, int cy, float r, int scale, uint32_t rgb) {
  int n = (int) ceilf(r);
  for (int gy = -n; gy < n; gy++) {
    int start = 999, end = -999;
    for (int gx = -n; gx < n; gx++) {
      float fx = gx + 0.5f, fy = gy + 0.5f;
      if (fx * fx + fy * fy <= r * r) {
        if (gx < start) start = gx;
        end = gx;
      }
    }
    if (end >= start) rect(c, cx + start * scale, cy + gy * scale, (end - start + 1) * scale, scale, rgb);
  }
}

void PetEngine::icon(Canvas &c, const char *const *rows, int nrows, int x, int y, int scale, int keep_cols,
                     bool grey) {
  for (int r = 0; r < nrows; r++) {
    const char *row = rows[r];
    int len = (int) strlen(row);
    if (len > keep_cols) len = keep_cols;
    int i = 0;
    while (i < len) {
      char ch = row[i];
      int j = i + 1;
      while (j < len && row[j] == ch) j++;
      if (ch != '.') {
        uint32_t col = art::icon_color(ch);
        if (grey) {
          int l = (int) (((col >> 16) & 0xFF) * 0.3f + ((col >> 8) & 0xFF) * 0.59f + (col & 0xFF) * 0.11f);
          l = 90 + l / 3;
          col = (uint32_t) (l << 16 | l << 8 | l);
        }
        rect(c, x + i * scale, y + r * scale, (j - i) * scale, scale, col);
      }
      i = j;
    }
  }
}

void PetEngine::text(Canvas &c, int x, int y, const char *s, int scale, uint32_t rgb, bool center, bool shadow) {
  int len = (int) strlen(s);
  if (center) x -= (len * 6 * scale - scale) / 2;
  if (shadow) text(c, x + scale, y + scale, s, scale, 0x1A1426, false, false);
  for (int i = 0; i < len; i++) {
    unsigned char ch = (unsigned char) s[i];
    if (ch < 32 || ch > 126) ch = '?';
    const uint8_t *col = art::FONT5X7[ch - 32];
    for (int cx = 0; cx < 5; cx++) {
      uint8_t bits = col[cx];
      int r = 0;
      while (r < 7) {
        if (!(bits >> r & 1)) {
          r++;
          continue;
        }
        int r2 = r;
        while (r2 < 7 && (bits >> r2 & 1)) r2++;
        rect(c, x + (i * 6 + cx) * scale, y + r * scale, scale, (r2 - r) * scale, rgb);
        r = r2;
      }
    }
  }
}

void PetEngine::bar(Canvas &c, int x, int y, int w, int h, int v) {
  rect(c, x - 2, y - 2, w + 4, h + 4, 0x2B2233);
  rect(c, x, y, w, h, 0x4A4060);
  uint32_t col = v >= 50 ? 0x5BD46B : (v >= 25 ? 0xFFC93C : 0xF0454B);
  rect(c, x, y, w * v / 100, h, col);
}

static void blit(Canvas &c, PetEngine *e, const Sprite &sp, int ox, int oy, int scale, bool flip,
                 void (PetEngine::*rect_fn)(Canvas &, int, int, int, int, uint32_t)) {
  for (int y = 0; y < G; y++) {
    int x = 0;
    while (x < G) {
      int sx = flip ? G - 1 - x : x;
      uint8_t v = sp.px[y][sx];
      int run = 1;
      while (x + run < G && sp.px[y][flip ? G - 1 - (x + run) : x + run] == v) run++;
      if (v != C_NONE) (e->*rect_fn)(c, ox + x * scale, oy + y * scale, run * scale, scale, sp.pal[v]);
      x += run;
    }
  }
}

// ---------------------------------------------------------------------------------------------
// Screens

void PetEngine::render(Canvas &c, uint32_t now) {
  now_ = now;
  g_dim = 1.0f;
  switch (page_) {
    case 1: draw_stats(c); break;
    case 2: draw_shop(c, now); break;
    case 3: draw_settings(c, now); break;
    case PAGE_TABLES: draw_tables(c, now); break;
    default: draw_scene(c, now); break;
  }
  g_dim = 1.0f;
  draw_page_dots(c);
  if (toast_[0] && (int32_t) (toast_until_ - now) > 0) {
    int w = (int) strlen(toast_) * 12 + 18;
    rect(c, 180 - w / 2, 74, w, 26, 0x2B2233);
    rect(c, 180 - w / 2 + 2, 76, w - 4, 22, 0x3D3350);
    text(c, 180, 80, toast_, 2, 0xFFFFFF, true, false);
  }
}

void PetEngine::draw_page_dots(Canvas &c) {
  for (int i = 0; i < PAGE_COUNT; i++)
    circle(c, 180 - (PAGE_COUNT - 1) * 6 + i * 12, 349, 3, i == page_ ? 0xFFFFFF : 0x6A6480);
}

void PetEngine::draw_sky(Canvas &c, uint32_t now) {
  int mod = clock_valid_ ? minute_of_day(epoch_) : 12 * 60;
  struct Key {
    int m;
    uint32_t top, bot;
  };
  static const Key keys[] = {{0, 0x0B1026, 0x1E2A5A},    {300, 0x0B1026, 0x1E2A5A},  {360, 0x3B4C8C, 0xF7A96B},
                             {450, 0x5EB8FF, 0xBDE8FF},  {1050, 0x5EB8FF, 0xBDE8FF}, {1110, 0x4A3C8C, 0xFF8A65},
                             {1170, 0x1A1F4A, 0x3C3270}, {1260, 0x0B1026, 0x1E2A5A}, {1440, 0x0B1026, 0x1E2A5A}};
  uint32_t top = keys[0].top, bot = keys[0].bot;
  for (size_t i = 0; i + 1 < sizeof(keys) / sizeof(keys[0]); i++) {
    if (mod >= keys[i].m && mod < keys[i + 1].m) {
      float t = (float) (mod - keys[i].m) / (float) (keys[i + 1].m - keys[i].m);
      top = mix(keys[i].top, keys[i + 1].top, t);
      bot = mix(keys[i].bot, keys[i + 1].bot, t);
    }
  }
  for (int y = 0; y < GROUND_Y; y += 6) rect(c, 0, y, SCREEN, 6, mix(top, bot, (float) y / GROUND_Y));

  bool day = mod >= 390 && mod < 1110;
  if (day) {
    const float r = 5.5f;
    int sx = 268, sy = 104;
    bool alt = (now / 600) % 2;
    for (int i = 0; i < 8; i++) {
      float a = i * 3.14159265f / 4 + (alt ? 0.39f : 0);
      int d = (int) (r * 4 + 10);
      rect(c, sx + (int) (cosf(a) * d) - 3, sy + (int) (sinf(a) * d) - 3, 6, 6, 0xFFE88A);
    }
    pix_circle(c, sx, sy, r, 4, 0xFFD23F);
    pix_circle(c, sx - 4, sy - 4, r * 0.45f, 4, 0xFFF0A0);
    for (int i = 0; i < 2; i++) {
      int x = (int) ((now / (90 + i * 30) + i * 170) % 480) - 60;
      int y = 120 + i * 34 - (i == 2 ? 60 : 0);
      pix_circle(c, x, y, 3.2f, 4, 0xFFFFFF);
      pix_circle(c, x + 16, y - 6, 3.8f, 4, 0xFFFFFF);
      pix_circle(c, x + 32, y, 3.0f, 4, 0xFFFFFF);
    }
  } else {
    for (int i = 0; i < 16; i++) {
      uint32_t hsh = (uint32_t) i * 2654435761u;
      int x = 40 + (int) (hsh % 280), y = 30 + (int) ((hsh >> 9) % 170);
      int s = ((now / 700 + i) % 4 == 0) ? 3 : 2;
      rect(c, x, y, s, s, 0xFFF6C8);
    }
    pix_circle(c, 92, 104, 5, 4, 0xFFF3C4);
    pix_circle(c, 104, 96, 4.5f, 4, mix(top, bot, 96.0f / GROUND_Y));
  }
}

void PetEngine::draw_scene(Canvas &c, uint32_t now) {
  g_dim = s_.lights_on ? 1.0f : 0.32f;
  draw_sky(c, now);

  // Ground
  int mod = clock_valid_ ? minute_of_day(epoch_) : 12 * 60;
  bool day = mod >= 390 && mod < 1110;
  uint32_t grass = day ? 0x79C267 : 0x3F6E48, edge = day ? 0x5BA64E : 0x2F5638;
  rect(c, 0, GROUND_Y, SCREEN, SCREEN - GROUND_Y, grass);
  rect(c, 0, GROUND_Y, SCREEN, 5, edge);
  int scroll = overlay_ == OV_GAME ? (int) run_scroll_ % SCREEN : 0;
  for (int i = 0; i < 9; i++) {
    int x = ((22 + i * 40 + (i % 2) * 9 - scroll) % SCREEN + SCREEN) % SCREEN - 10;
    int y = GROUND_Y + 14 + (i % 3) * 7;
    rect(c, x, y, 3, 6, edge);
    rect(c, x + 5, y - 3, 3, 9, edge);
    rect(c, x + 10, y, 3, 6, edge);
  }

  if (overlay_ == OV_STAND) {
    g_dim = 1.0f;
    draw_stand(c, now);
    return;
  }
  if (overlay_ == OV_GAME) {
    g_dim = 1.0f;
    switch (game_kind_) {
      case GAME_CATCH: draw_catch(c, now); break;
      case GAME_POP: draw_pop(c, now); break;
      case GAME_COPY: draw_copy(c, now); break;
      case GAME_QUIZ:
      case GAME_SHARE: draw_quiz(c, now); break;
      case GAME_FACTS: draw_facts(c, now); break;
      case GAME_FIZZ: draw_fizz(c, now); break;
      case GAME_MUNCH: draw_munch(c, now); break;
      case GAME_CHANT: draw_chant(c, now); break;
      default: draw_runner(c, now); break;
    }
    return;
  }

  // Poops (during cleaning, the ones the wave has not reached yet)
  static const int POOP_X[4] = {246, 282, 316, 212};
  int show_poops = s_.poops;
  int wave_x = -100;
  if (act_ == ACT_CLEAN) {
    float t = (float) (now - act_start_) / (float) (act_until_ - act_start_);
    wave_x = (int) (-40 + t * 440);
    show_poops = clean_poops_;
  }
  for (int i = 0; i < show_poops && i < 4; i++) {
    if (POOP_X[i] < wave_x) continue;
    int px = POOP_X[i], py = GROUND_Y - 22;
    icon(c, art::POOP, 10, px, py, 3);
    int wig = (now / 300 + i) % 2 ? 2 : 0;
    rect(c, px + 6 + wig, py - 14, 2, 6, 0x7A6A50);
    rect(c, px + 16 - wig, py - 18, 2, 6, 0x7A6A50);
  }

  if (owns(IT_PLANT)) icon(c, art::PLANT, 12, 78, GROUND_Y - 34, 3);  // behind the pet
  // Math prizes sit on a shelf on the left.
  if (owns(IT_TROPHY) || owns(IT_ROCKET) || owns(IT_OWL) || owns(IT_GOLD_APPLE)) {
    rect(c, 14, 130, 110, 6, 0x8A5A35);
    rect(c, 22, 136, 6, 8, 0x5C3A22);
    rect(c, 110, 136, 6, 8, 0x5C3A22);
    if (owns(IT_TROPHY)) icon(c, art::TROPHY, 12, 18, 106, 2);
    if (owns(IT_ROCKET)) icon(c, art::ROCKET, 12, 44, 106, 2);
    if (owns(IT_OWL)) icon(c, art::OWL, 12, 70, 106, 2);
    if (owns(IT_GOLD_APPLE)) icon(c, art::GOLD_APPLE, 12, 96, 106, 2);
  }
  // Prize-stand things sit on a study shelf on the right, three at a time, taking turns every 6 seconds.
  {
    int decor[IT_COUNT], n = 0;
    for (int id = IT_PROP_CAP; id < IT_COUNT; id++)
      if (is_decor(id) && owns(id)) decor[n++] = id;
    if (n) {
      rect(c, 258, 176, 88, 6, 0x8A5A35);
      rect(c, 266, 182, 6, 8, 0x5C3A22);
      rect(c, 332, 182, 6, 8, 0x5C3A22);
      int first = n > 3 ? (int) ((now / 6000) * 3 % n) : 0;
      for (int k = 0; k < 3 && k < n; k++) {
        const ShopItem &it = SHOP[decor[(first + k) % n]];
        icon(c, it.icon, it.rows, 262 + k * 26, 152, 2);
      }
    }
  }

  // Pet
  int pcx = 180 + (int) walk_x_;
  draw_pet(c, now, pcx, GROUND_Y);

  // Cleaning wave
  if (act_ == ACT_CLEAN) {
    for (int i = 0; i < 4; i++) {
      icon(c, art::CLEAN, 12, wave_x - 12 + (i % 2) * 14, GROUND_Y - 30 - i * 26, 2);
      circle(c, wave_x + 18, GROUND_Y - 10 - i * 22, 5, 0xDFF3FF);
    }
  }

  // HUD and buttons
  g_dim = s_.lights_on ? 1.0f : 0.55f;
  if (overlay_ == OV_NONE) draw_hud(c);
  if (overlay_ == OV_NONE && !s_.dead) draw_buttons(c, s_.stage != STAGE_EGG);
  g_dim = 1.0f;
  if (overlay_ == OV_FEED) draw_feed_menu(c);
  if (overlay_ == OV_GAMES) draw_games_menu(c);
  if (s_.dead) {
    text(c, 180, 268, "Hold to hatch", 2, 0xFFFFFF);
    text(c, 180, 288, "a new egg", 2, 0xFFFFFF);
  }
}

void PetEngine::draw_pet(Canvas &c, uint32_t now, int pcx, int ground) {
  static Sprite sp;
  const int sc = SPRITE_SCALE;
  int ox = pcx - G * sc / 2;
  int oy = ground - 30 * sc;  // sprite row 30 sits on the ground

  if (s_.stage == STAGE_EGG && !s_.dead) {
    int crack = s_.together_s >= STAGE_TOGETHER_S[STAGE_BABY] / 2 ? ((now / 1000) % 4 == 0 ? 2 : 1) : 0;
    build_egg(s_.g, crack, sp);
    int wob = ((now / 180) % 10) < 2 ? ((now / 90) % 2 ? sc : -sc) : 0;
    blit(c, this, sp, 180 - G * sc / 2 + wob, oy, sc, false, &PetEngine::rect);
    return;
  }

  if (s_.dead) {
    int fy = (int) (sinf(now / 600.0f) * 8);
    icon(c, art::TOMB, 12, 228, ground - 44, 4);
    build_pet(s_.g, s_.stage, s_.form, EX_DEAD, 0, false, true, sp);
    int gx = 150 - G * sc / 2, gy = oy - 30 + fy;
    // halo
    rect(c, gx + 12 * sc, gy + 2 * sc - 4, 8 * sc, 4, 0xFFC93C);
    rect(c, gx + 11 * sc, gy + 2 * sc, 2 * sc, 4, 0xFFC93C);
    rect(c, gx + 19 * sc, gy + 2 * sc, 2 * sc, 4, 0xFFC93C);
    blit(c, this, sp, gx, gy, sc, false, &PetEngine::rect);
    return;
  }

  Expr ex = EX_NORMAL;
  int look = 0, jump = 0, shake = 0;
  bool squash = (now / 600) % 2;
  bool dozing = is_dozing(now) && act_ == ACT_IDLE;
  bool walking = fabsf(walk_x_ - walk_target_) >= 1 && act_ == ACT_IDLE && !s_.asleep && !dozing;
  bool flip = facing_ < 0;
  bool blink = (now % 3700) < 140;
  // With a ball it now and then plays: bounces it for 5 seconds out of every 20.
  bool ball = owns(IT_BALL) && !s_.asleep && !dozing && act_ == ACT_IDLE && overlay_ == OV_NONE && !s_.sick &&
              !s_.tantrum && (now / 1000) % 20 < 5;

  if (owns(IT_BED) && s_.asleep) {  // the bed goes under it
    rect(c, pcx - 84, ground - 52, 10, 52, 0x8A5A35);
    rect(c, pcx - 78, ground - 16, 156, 16, 0x8A5A35);
    rect(c, pcx - 74, ground - 22, 148, 8, 0xFFFFFF);
    rect(c, pcx - 74, ground - 32, 30, 10, 0xFFFFFF);
  }

  if (s_.asleep || dozing) {
    ex = EX_SLEEP;
    squash = (now / 1500) % 2;
  } else if (ball) {
    ex = EX_HAPPY;
    look = facing_;
  } else {
    switch (act_) {
      case ACT_EAT: ex = (now / 300) % 2 ? EX_EAT_OPEN : EX_EAT_SHUT; break;
      case ACT_HAPPY:
      case ACT_HATCH: {
        ex = EX_HAPPY;
        float t = (float) (now - act_start_) / 450.0f;
        jump = (int) fabsf(sinf(t * 3.14159f) * 16);
        break;
      }
      case ACT_REFUSE:
        ex = EX_ANGRY;
        shake = ((now / 90) % 2) ? 5 : -5;
        break;
      case ACT_CLEAN: ex = EX_HAPPY; break;
      case ACT_SULK: ex = EX_SAD; break;
      default:
        if (s_.tantrum) {
          ex = EX_ANGRY;  // stomping about
          shake = (now / 150) % 4 == 0 ? 4 : ((now / 150) % 4 == 2 ? -4 : 0);
          jump = (now / 150) % 4 == 1 ? 6 : 0;
        } else if (s_.sick) ex = EX_SICK;
        else if (s_.food < 20 || s_.happy < 20 || s_.health < 30) ex = blink ? EX_BLINK : EX_SAD;
        else if (blink) ex = EX_BLINK;
        else if (s_.happy >= 80) ex = EX_HAPPY;
        if (walking) look = facing_;
        break;
    }
  }
  if (act_ == ACT_EAT) flip = true;  // face the food on the left

  if (act_ == ACT_HATCH && (now / 120) % 2) circle(c, pcx, ground - 70, 70, 0xFFFFFF);
  build_pet(s_.g, s_.stage, s_.form, ex, flip ? -look : look, squash, false, sp, s_.hat);
  int bob = (walking && (now / 200) % 2) ? -sc : 0;
  blit(c, this, sp, ox + shake, oy - jump + bob, sc, flip, &PetEngine::rect);
  draw_overlay_hat(c, ox + shake, oy - jump + bob, sc);

  int head_y = oy + 6 * sc;
  if (ball) {
    float t = (float) (now % 700) / 700.0f;
    int bx = pcx + facing_ * 64, by = ground - 14 - (int) (fabsf(sinf(t * 3.14159f)) * 56);
    icon(c, art::PLAY, 12, bx - 12, by - 12, 2);
  }
  if (owns(IT_TEDDY) && (s_.asleep || dozing)) icon(c, art::TEDDY, 12, pcx + 40, ground - 38, 3);
  if (act_ == ACT_EAT) {
    int bites = (int) ((now - act_start_) / 1000);
    if (bites < 3)
      icon(c, eat_icon_ ? eat_icon_ : art::MEAL, 12, pcx - 112, ground - 70, 4, 12 - bites * 4);
  }
  if (act_ == ACT_HAPPY || act_ == ACT_HATCH) {
    float t = (float) (now - act_start_) / 1500.0f;
    for (int i = 0; i < 3; i++)
      icon(c, art::MINI_HEART, 7, pcx - 60 + i * 50, head_y + 30 - (int) (t * 60) - (i % 2) * 14, 3);
  }
  if (s_.asleep) {
    int phase = (now / 700) % 3;
    for (int i = 0; i <= phase; i++)
      text(c, pcx + 50 + i * 18, head_y + 10 - i * 22, i == 2 ? "Z" : "z", 2 + i, 0xFFFFFF);
  } else if (dozing) {
    if ((now / 1200) % 2) text(c, pcx + 50, head_y + 4, "z", 2, 0xFFFFFF);  // just dozing
  } else if (s_.sick) {
    circle(c, pcx + 62, head_y - 4, 20, 0x2B2233);
    circle(c, pcx + 62, head_y - 4, 18, 0xFFFFFF);
    icon(c, art::SKULL, 7, pcx + 62 - 10, head_y - 4 - 10, 3);
  } else if (needs_attention() && act_ == ACT_IDLE && overlay_ == OV_NONE && (now / 500) % 2) {
    // A tantrum gets a red bubble: that one wants the Scold button, not care.
    circle(c, pcx - 62, head_y - 4, 18, 0x2B2233);
    circle(c, pcx - 62, head_y - 4, 16, s_.tantrum ? 0xF0454B : 0xFFF6E0);
    text(c, pcx - 62, head_y - 4 - 10, "!", 3, s_.tantrum ? 0xFFFFFF : 0xF0454B, true, false);
  }
}

void PetEngine::draw_hud(Canvas &c) {
  static const char *const *ICONS[5] = {art::MINI_FOOD, art::MINI_HEART, art::MINI_BOLT, art::MINI_DROP,
                                        art::MINI_MATH};
  int vals[5] = {s_.food, s_.happy, s_.energy, s_.clean, math()};
  if (s_.stage == STAGE_EGG || s_.dead) return;
  int n = needs_math() ? 5 : 4;
  for (int i = 0; i < n; i++) {
    int cx = n == 5 ? 88 + i * 46 : 99 + i * 54, y = 44;
    icon(c, ICONS[i], 7, cx - 24, y - 7, 2);
    bar(c, cx - 8, y - 4, n == 5 ? 28 : 32, 8, vals[i]);
  }
}

void PetEngine::draw_buttons(Canvas &c, bool enabled) {
  static const char *const *const ICONS[6] = {art::FOOD, art::PLAY, art::CLEAN, art::BULB, art::HEAL, art::SCOLD};
  for (int i = 0; i < 6; i++) {
    int x, y;
    button_pos(i, x, y);
    bool on = enabled && !(s_.asleep && (i == 0 || i == 1 || i == 5));
    if (i == 3 && !s_.dead) on = true;
    circle(c, x, y, BTN_R + 2, 0x2B2233);
    // Scold glows red during a tantrum
    uint32_t face = !on ? 0xA8A2B0 : (i == 5 && s_.tantrum && (now_ / 400) % 2) ? 0xFF9A9A : 0xFFF6E0;
    circle(c, x, y, BTN_R - 1, face);
    const char *const *ic = i == 3 ? (s_.lights_on ? art::BULB : art::MOON) : ICONS[i];
    icon(c, ic, 12, x - 18, y - 18, 3, 99, !on);
  }
}

void PetEngine::draw_feed_menu(Canvas &c) {
  circle(c, 180, 175, 128, 0x2B2233);
  circle(c, 180, 175, 124, 0xFFF6E0);
  text(c, 180, 78, "FEED", 3, 0x2B2233, true, false);
  const int xs[2] = {122, 238};
  for (int i = 0; i < 2; i++) {
    circle(c, xs[i], 170, 48, 0x2B2233);
    circle(c, xs[i], 170, 45, i == 0 ? 0xFFE3B0 : 0xFFD0E4);
    icon(c, i == 0 ? art::MEAL : art::SNACK, 12, xs[i] - 30, 140, 5);
    text(c, xs[i], 228, i == 0 ? "MEAL" : "SNACK", 2, 0x2B2233, true, false);
  }
  text(c, 180, 262, "tap outside", 2, 0x8A8098, true, false);
  text(c, 180, 280, "to cancel", 2, 0x8A8098, true, false);
}

void PetEngine::draw_runner(Canvas &c, uint32_t now) {
  static Sprite sp;
  const int sc = 3;

  for (const auto &o : obs_) {
    if (!o.active) continue;
    int w, h, b;
    obstacle_box(o.kind, w, h, b);
    int x = (int) o.x, y = GROUND_Y - b - h;
    switch (o.kind) {
      case OB_ROCK: icon(c, art::ROCK, 8, x, y + 2, sc); break;
      case OB_MUSHROOM: icon(c, art::MUSHROOM, 10, x, y + 2, sc); break;
      case OB_BUSH: icon(c, art::BUSH, 10, x, y + 2, sc); break;
      case OB_BIRD: icon(c, (now / 140) % 2 ? art::BIRD_UP : art::BIRD_DOWN, 8, x, y, 4); break;
      case OB_STAR: icon(c, art::STAR, 9, x, y + (int) (sinf(now / 200.0f) * 3), sc); break;
      default: break;  // other games' things
    }
  }

  // The pet, facing right. Legs pump while it runs; a crash shows x-eyes.
  Expr ex = EX_NORMAL;
  bool on_ground = run_y_ <= 0;
  bool squash = false;
  if (run_phase_ == RUN_READY) {
    ex = (now % 3000) < 140 ? EX_BLINK : EX_HAPPY;
    squash = (now / 600) % 2;
  } else if (run_phase_ == RUN_OVER) {
    ex = EX_DEAD;
  } else {
    ex = on_ground ? EX_NORMAL : EX_HAPPY;
    squash = on_ground && (now / 110) % 2;
  }
  build_pet(s_.g, s_.stage, s_.form, ex, 1, squash, false, sp, s_.hat);
  int ox = (int) RUN_PET_X - G * sc / 2, oy = GROUND_Y - 30 * sc - (int) run_y_;
  int shake = run_phase_ == RUN_OVER && now - run_t_ < 400 ? ((now / 60) % 2 ? 3 : -3) : 0;
  blit(c, this, sp, ox + shake, oy, sc, false, &PetEngine::rect);
  draw_overlay_hat(c, ox + shake, oy, sc);
  if (run_phase_ == RUN_GO && on_ground && (now / 110) % 2) {
    rect(c, ox + 6, GROUND_Y - 8, 6, 6, 0xE8F4D8);  // dust puffs
    rect(c, ox - 4, GROUND_Y - 14, 4, 4, 0xE8F4D8);
  }

  draw_game_hud(c, now, "Tap to jump!", "hold = higher", "birds: stay down");
}

// Score and best at the top; how to play before the first tap; the result after.
void PetEngine::draw_game_hud(Canvas &c, uint32_t now, const char *title, const char *hint1, const char *hint2) {
  char buf[24];
  snprintf(buf, sizeof(buf), "%d", run_score_);
  text(c, 180, 26, buf, 3, 0xFFFFFF);
  snprintf(buf, sizeof(buf), "BEST %d", best_score(game_kind_));
  text(c, 180, 54, buf, 2, 0xFFF6C8);
  if (run_phase_ == RUN_READY) {
    text(c, 180, 90, title, 2, 0xFFFFFF);
    text(c, 180, 112, hint1, 2, 0xFFF6C8);
    text(c, 180, 280, hint2, 2, 0xFFFFFF);
  } else if (run_phase_ == RUN_OVER) {
    const char *word = game_kind_ == GAME_POP ? "Time!" : game_kind_ == GAME_COPY ? "Oops!" : "Ouch!";
    text(c, 180, 90, word, 4, game_kind_ == GAME_POP ? 0xFFF6C8 : 0xFF8A8A);
    if (run_new_best_) text(c, 180, 130, "New best!", 2, 0xFFC93C);
    if (now - run_t_ > 700) text(c, 180, 280, "tap to finish", 2, 0xFFFFFF);
  }
}

void PetEngine::draw_games_menu(Canvas &c) {
  circle(c, 180, 180, 168, 0x2B2233);
  circle(c, 180, 180, 164, 0xFFF6E0);
  text(c, 180, 36, "PLAY", 3, 0x2B2233, true, false);
  static const char *const NAMES[GAME_COUNT] = {"JUMP", "CATCH", "POP",   "COPY",  "QUIZ",
                                                "FACTS", "FIZZ", "MUNCH", "SHARE", "CHANT"};
  static const uint32_t BG[GAME_COUNT] = {0xD8F0C8, 0xFFE3B0, 0xCFE8FF, 0xF3D6FF, 0xC8F0E0,
                                          0xFFF0B0, 0xC8F0E0, 0xFFF0B0, 0xC8E0FF, 0xFFD8E8};
  for (int i = 0; i < GAME_COUNT; i++) {
    int gx, gy;
    menu_pos(i, gx, gy);
    circle(c, gx, gy, MENU_R + 3, 0x2B2233);
    circle(c, gx, gy, MENU_R, BG[i]);
    switch (i) {
      case GAME_JUMP: icon(c, art::BUSH, 10, gx - 14, gy - 30, 2); break;
      case GAME_CATCH: icon(c, art::FOOD, 12, gx - 12, gy - 30, 2); break;
      case GAME_POP: icon(c, art::BUBBLE, 12, gx - 12, gy - 30, 2); break;
      case GAME_COPY: icon(c, art::PADS, 12, gx - 12, gy - 30, 2); break;
      case GAME_QUIZ: icon(c, art::TIMES, 12, gx - 12, gy - 30, 2); break;
      case GAME_FACTS: icon(c, art::KEYPAD, 12, gx - 12, gy - 30, 2); break;
      case GAME_FIZZ: icon(c, art::FIZZ_ICON, 12, gx - 12, gy - 30, 2); break;
      case GAME_MUNCH: icon(c, art::MUNCH_ICON, 12, gx - 12, gy - 30, 2); break;
      case GAME_CHANT: icon(c, art::MIC, 12, gx - 12, gy - 30, 2); break;
      default: icon(c, art::DIVIDE, 12, gx - 12, gy - 30, 2); break;
    }
    text(c, gx, gy - 2, NAMES[i], 2, 0x2B2233, true, false);
    char buf[16];
    snprintf(buf, sizeof(buf), "best %d", best_score(i));
    text(c, gx, gy + 18, buf, 1, 0x6A6480, true, false);
  }
}

void PetEngine::draw_catch(Canvas &c, uint32_t now) {
  static Sprite sp;
  const int sc = 3;
  for (const auto &o : obs_) {
    if (!o.active) continue;
    int x = (int) o.x, y = (int) o.y;
    if (o.kind == OB_ROCK)
      icon(c, art::ROCK, 8, x, y, sc);
    else
      icon(c, o.kind == OB_CAKE ? art::SNACK : art::FOOD, 12, x, y, sc);
  }
  Expr ex = run_phase_ == RUN_OVER ? EX_DEAD : (run_phase_ == RUN_READY ? EX_HAPPY : EX_EAT_OPEN);
  int look = catch_target_ > catch_x_ + 4 ? 1 : (catch_target_ < catch_x_ - 4 ? -1 : 0);
  bool moving = fabsf(catch_target_ - catch_x_) > 4;
  build_pet(s_.g, s_.stage, s_.form, ex, look, moving && (now / 110) % 2, false, sp, s_.hat);
  blit(c, this, sp, (int) catch_x_ - G * sc / 2, GROUND_Y - 30 * sc, sc, false, &PetEngine::rect);
  draw_overlay_hat(c, (int) catch_x_ - G * sc / 2, GROUND_Y - 30 * sc, sc);
  for (int i = 0; i < 3; i++)  // lives
    icon(c, art::MINI_HEART, 7, 148 + i * 24, 74, 3, 99, i >= lives_);
  draw_game_hud(c, now, "Catch the fruit!", "drag to move", "rocks hurt!");
}

void PetEngine::draw_pop(Canvas &c, uint32_t now) {
  static Sprite sp;
  for (const auto &o : obs_) {
    if (!o.active) continue;
    int x = (int) o.x, y = (int) o.y;
    if (o.passed) {  // burst
      int r = 12 + (int) ((now - o.t) / 10);
      for (int k = 0; k < 8; k++) {
        float a = k * 3.14159f / 4;
        rect(c, x + (int) (cosf(a) * r) - 3, y + (int) (sinf(a) * r) - 3, 6, 6,
             o.kind == OB_SPIKY ? 0xF0454B : 0xFFFFFF);
      }
      continue;
    }
    uint32_t age = now - o.t;
    int r = age < 150 ? 10 + (int) (age * 20 / 150) : 30;
    if (age + 300 > o.life_ms && (now / 80) % 2) r -= 3;  // about to go: wobble
    bool spiky = o.kind == OB_SPIKY;
    if (spiky)
      for (int k = 0; k < 8; k++) {
        float a = k * 3.14159f / 4;
        rect(c, x + (int) (cosf(a) * (r + 5)) - 3, y + (int) (sinf(a) * (r + 5)) - 3, 7, 7, 0x8E2A3A);
      }
    circle(c, x, y, r + 2, 0x2B2233);
    circle(c, x, y, r, spiky ? 0xF0454B : 0x9AD8FF);
    circle(c, x - r / 3, y - r / 3, r / 4 + 1, 0xFFFFFF);
  }
  build_pet(s_.g, s_.stage, s_.form, run_phase_ == RUN_OVER ? EX_HAPPY : EX_NORMAL, 0, (now / 300) % 2, false, sp, s_.hat);
  blit(c, this, sp, 180 - G * 2 / 2, GROUND_Y - 30 * 2, 2, false, &PetEngine::rect);
  draw_overlay_hat(c, 180 - G * 2 / 2, GROUND_Y - 30 * 2, 2);
  if (run_phase_ == RUN_GO) {
    char buf[12];
    int left = (int) ((int32_t) (game_end_ms_ - now) / 1000) + 1;
    snprintf(buf, sizeof(buf), "%ds", left < 0 ? 0 : left);
    text(c, 180, 76, buf, 2, left <= 5 ? 0xFF8A8A : 0xFFFFFF);
  }
  draw_game_hud(c, now, "Pop the bubbles!", "30 seconds", "not the red ones!");
}

void PetEngine::draw_copy(Canvas &c, uint32_t now) {
  rect(c, 0, 0, SCREEN, SCREEN, 0x1B1530);
  bool lit_now = lit_pad_ >= 0 && (int32_t) (lit_until_ - now) > 0;
  for (int i = 0; i < 4; i++) {
    int px, py;
    pad_center(i, px, py);
    bool lit = lit_now && lit_pad_ == i;
    circle(c, px, py, PAD_R + 3, lit ? 0xFFFFFF : 0x2B2233);
    circle(c, px, py, PAD_R, lit ? PAD_COLORS[i] : mix(0x1B1530, PAD_COLORS[i], 0.45f));
  }
  circle(c, 180, 180, 28, 0x2B2233);
  char buf[24];
  snprintf(buf, sizeof(buf), "%d", run_score_);
  text(c, 180, 170, buf, 3, 0xFFFFFF);
  snprintf(buf, sizeof(buf), "BEST %d", best_score(GAME_COPY));
  text(c, 180, 306, buf, 2, 0xFFF6C8);
  const char *top = "";
  if (run_phase_ == RUN_READY) top = "Copy me! Tap";
  else if (run_phase_ == RUN_OVER) top = run_new_best_ ? "Oops! New best!" : "Oops!";
  else if (copy_state_ == COPY_INPUT) top = "Your turn";
  else if (copy_state_ == COPY_SHOW) top = "Watch...";
  else top = "Yes!";
  text(c, 180, 30, top, 2, run_phase_ == RUN_OVER ? 0xFF8A8A : 0xFFFFFF);
  if (run_phase_ == RUN_OVER && now - run_t_ > 700) text(c, 180, 326, "tap to finish", 1, 0xC9C2E8);
}

// Before the first tap and after the last answer, the math games show this instead of the board.
static void math_intro_lines(int kind, const char *&title, const char *&l1, const char *&l2) {
  if (kind == GAME_QUIZ) {
    title = "Times quiz!";
    l1 = "pick the answer";
    l2 = "10 questions";
  } else if (kind == GAME_SHARE) {
    title = "Share it!";
    l1 = "divide: pick one";
    l2 = "10 questions";
  } else {
    title = "Times facts!";
    l1 = "type the answer";
    l2 = "90 seconds";
  }
}

// The top of the Fizz and Munch screens while playing: score, the question on a band, lives.
static void math_top(PetEngine *e, Canvas &c, void (PetEngine::*text_fn)(Canvas &, int, int, const char *, int,
                                                                         uint32_t, bool, bool),
                     void (PetEngine::*rect_fn)(Canvas &, int, int, int, int, uint32_t), int score,
                     const char *question) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", score);
  (e->*text_fn)(c, 180, 12, buf, 2, 0xFFFFFF, true, true);
  (e->*rect_fn)(c, 30, 34, 300, 40, 0x2B2233);
  (e->*text_fn)(c, 180, 43, question, 3, 0xFFF6C8, true, false);
}

void PetEngine::draw_fizz(Canvas &c, uint32_t now) {
  if (run_phase_ != RUN_GO) {
    draw_game_hud(c, now, "Math bubbles!", "pop the answer", "5 lives");
    return;
  }
  char buf[24];
  for (const auto &o : obs_) {
    if (!o.active) continue;
    int x = (int) o.x, y = (int) o.y;
    if (o.passed) {  // burst: green when right, red when wrong
      int r = 14 + (int) ((now - o.t) / 8);
      for (int k = 0; k < 8; k++) {
        float a = k * 3.14159f / 4;
        rect(c, x + (int) (cosf(a) * r) - 4, y + (int) (sinf(a) * r) - 4, 8, 8,
             o.kind == OB_SPIKY ? 0xF0454B : 0x7CFF8A);
      }
      continue;
    }
    circle(c, x, y, 30, 0x2B2233);
    circle(c, x, y, 28, 0x9AD8FF);
    circle(c, x - 10, y - 11, 5, 0xFFFFFF);
    snprintf(buf, sizeof(buf), "%d", o.value);
    text(c, x, y - (o.value >= 100 ? 7 : 10), buf, o.value >= 100 ? 2 : 3, 0x2B2233, true, false);
  }
  if (missing_)
    snprintf(buf, sizeof(buf), "? x %d = %d", mb_, ma_ * mb_);
  else
    snprintf(buf, sizeof(buf), "%d x %d = ?", ma_, mb_);
  math_top(this, c, &PetEngine::text, &PetEngine::rect, run_score_, buf);
  for (int i = 0; i < 5; i++) icon(c, art::MINI_HEART, 7, 120 + i * 24, 80, 3, 99, i >= lives_);
}

void PetEngine::draw_munch(Canvas &c, uint32_t now) {
  static Sprite sp;
  if (run_phase_ != RUN_GO) {
    draw_game_hud(c, now, "Feed me facts!", "eat the target", "drag to move");
    return;
  }
  char buf[24];
  for (const auto &o : obs_) {
    if (!o.active) continue;
    int x = (int) o.x, y = (int) o.y;
    circle(c, x, y, 26, 0x2B2233);
    circle(c, x, y, 24, 0xFFB37A);
    snprintf(buf, sizeof(buf), "%dx%d", o.value / 16, o.value % 16);
    text(c, x, y - 7, buf, 2, 0x2B2233, true, false);
  }
  bool moving = fabsf(catch_target_ - catch_x_) > 4;
  int look = catch_target_ > catch_x_ + 4 ? 1 : (catch_target_ < catch_x_ - 4 ? -1 : 0);
  build_pet(s_.g, s_.stage, s_.form, EX_EAT_OPEN, look, moving && (now / 110) % 2, false, sp, s_.hat);
  blit(c, this, sp, (int) catch_x_ - G * 3 / 2, GROUND_Y - 30 * 3, 3, false, &PetEngine::rect);
  draw_overlay_hat(c, (int) catch_x_ - G * 3 / 2, GROUND_Y - 30 * 3, 3);
  snprintf(buf, sizeof(buf), "Feed me %d!", munch_target_);
  math_top(this, c, &PetEngine::text, &PetEngine::rect, run_score_, buf);
  for (int i = 0; i < 5; i++) icon(c, art::MINI_HEART, 7, 120 + i * 24, 80, 3, 99, i >= lives_);

  // New goal: a flashing banner slides in across the middle and stays for a moment.
  int32_t left = (int32_t) (goal_flash_until_ - now);
  if (left > 0) {
    uint32_t shown = MUNCH_BANNER_MS - (uint32_t) left;
    int slide = shown < 250 ? (int) ((250 - shown) * 360 / 250) : 0;  // in from the right
    bool flash = (now / 180) % 2;
    rect(c, slide, 132, SCREEN, 84, 0x2B2233);
    rect(c, slide, 136, SCREEN, 76, flash ? 0xFFD23F : 0xFF9A3C);
    text(c, 180 + slide, 142, run_score_ == 0 ? "Your goal:" : "New goal!", 2, 0x2B2233, true, false);
    text(c, 180 + slide, 166, buf, 4, 0x2B2233, true, false);
  }
}

void PetEngine::draw_quiz(Canvas &c, uint32_t now) {
  bool share = game_kind_ == GAME_SHARE;  // division: blue board
  uint32_t board = share ? 0x23364F : 0x234536, dark = share ? 0x16233A : 0x16301F;
  rect(c, 0, 0, SCREEN, SCREEN, board);
  char buf[32];
  if (run_phase_ != RUN_GO) {
    const char *title, *l1, *l2;
    math_intro_lines(game_kind_, title, l1, l2);
    if (run_phase_ == RUN_READY) {
      text(c, 180, 96, title, 3, 0xFFFFFF);
      text(c, 180, 150, l1, 2, 0xFFF6C8);
      text(c, 180, 174, l2, 2, 0xFFF6C8);
      text(c, 180, 240, "tap to start", 2, 0xFFFFFF);
    } else {
      snprintf(buf, sizeof(buf), "%d/10", run_score_);
      text(c, 180, 90, "Done!", 4, 0xFFFFFF);
      text(c, 180, 140, buf, 4, run_score_ >= 8 ? 0x7CFF8A : 0xFFD23F);
      if (run_new_best_) text(c, 180, 196, "New best!", 2, 0xFFC93C);
      if (now - run_t_ > 700) text(c, 180, 260, "tap to finish", 2, 0xFFFFFF);
    }
    snprintf(buf, sizeof(buf), "BEST %d", best_score(game_kind_));
    text(c, 180, 300, buf, 2, 0xC9C2E8);
    return;
  }
  snprintf(buf, sizeof(buf), "%d/10   %d right", q_num_ + 1, run_score_);
  text(c, 180, 22, buf, 2, 0xC9C2E8);
  if (share)
    snprintf(buf, sizeof(buf), "%d ` %d = ?", ma_ * mb_, mb_);  // ` is the division sign in our font
  else
    snprintf(buf, sizeof(buf), "%d x %d = ?", ma_, mb_);
  text(c, 180, 54, buf, 4, 0xFFFFFF);
  // time left for this question
  float left = feedback_ ? 0 : 1.0f - (float) (now - q_t_) / QUIZ_TIME_MS;
  if (left < 0) left = 0;
  rect(c, 80, 106, 200, 8, dark);
  rect(c, 80, 106, (int) (200 * left), 8, left > 0.3f ? 0x7CFF8A : 0xFF8A8A);
  for (int i = 0; i < 4; i++) {
    uint32_t fill = 0xFFF6E0;
    if (feedback_ && choices_[i] == mans_) fill = 0x7CFF8A;                // the right one
    else if (feedback_ == 2 && picked_ == i) fill = 0xFF8A8A;              // a wrong pick
    circle(c, QUIZ_X[i], QUIZ_Y[i], QUIZ_R + 3, dark);
    circle(c, QUIZ_X[i], QUIZ_Y[i], QUIZ_R, fill);
    snprintf(buf, sizeof(buf), "%d", choices_[i]);
    text(c, QUIZ_X[i], QUIZ_Y[i] - 14, buf, 4, 0x2B2233, true, false);
  }
  if (share && feedback_) {  // division links back to the times tables
    snprintf(buf, sizeof(buf), feedback_ == 1 ? "Yes! %d x %d = %d" : "since %d x %d = %d", mb_, ma_, ma_ * mb_);
    text(c, 180, 316, buf, 2, feedback_ == 1 ? 0x7CFF8A : 0xFFF6C8);
  } else if (feedback_ == 2) {
    snprintf(buf, sizeof(buf), "%d x %d = %d", ma_, mb_, mans_);
    text(c, 180, 316, buf, 2, 0xFFF6C8);
  } else if (feedback_ == 1) {
    text(c, 180, 316, "Yes!", 2, 0x7CFF8A);
  }
}

void PetEngine::draw_facts(Canvas &c, uint32_t now) {
  rect(c, 0, 0, SCREEN, SCREEN, 0x1B1530);
  char buf[32];
  if (run_phase_ != RUN_GO) {
    const char *title, *l1, *l2;
    math_intro_lines(GAME_FACTS, title, l1, l2);
    if (run_phase_ == RUN_READY) {
      text(c, 180, 96, title, 3, 0xFFFFFF);
      text(c, 180, 150, l1, 2, 0xFFF6C8);
      text(c, 180, 174, l2, 2, 0xFFF6C8);
      text(c, 180, 240, "tap to start", 2, 0xFFFFFF);
    } else {
      snprintf(buf, sizeof(buf), "%d right", run_score_);
      text(c, 180, 90, "Time!", 4, 0xFFFFFF);
      text(c, 180, 140, buf, 3, 0xFFD23F);
      if (run_new_best_) text(c, 180, 186, "New best!", 2, 0xFFC93C);
      if (now - run_t_ > 700) text(c, 180, 260, "tap to finish", 2, 0xFFFFFF);
    }
    snprintf(buf, sizeof(buf), "BEST %d", best_score(GAME_FACTS));
    text(c, 180, 300, buf, 2, 0xC9C2E8);
    return;
  }
  int secs = (int) ((int32_t) (game_end_ms_ - now) / 1000) + 1;
  // In the middle: the question, the answer box, then the score and the time left. The keys ring the edge.
  snprintf(buf, sizeof(buf), "%d x %d =", ma_, mb_);
  if (strlen(buf) > 7)
    text(c, 180, 110, buf, 3, 0xFFFFFF);  // "10 x 10 =" at the big size would touch the 0 and 2 keys
  else
    text(c, 180, 104, buf, 4, 0xFFFFFF);
  uint32_t box = feedback_ == 1 ? 0x7CFF8A : feedback_ == 2 ? 0xFF8A8A : 0xFFF6E0;
  rect(c, 118, 146, 124, 40, 0x2B2233);
  rect(c, 121, 149, 118, 34, box);
  if (feedback_ == 2)
    snprintf(buf, sizeof(buf), "%d", mans_);
  else if (entry_len_)
    snprintf(buf, sizeof(buf), "%d%s", entry_, (now / 400) % 2 ? "_" : " ");
  else
    snprintf(buf, sizeof(buf), "%s", (now / 400) % 2 ? "_" : " ");
  text(c, 180, 152, buf, 4, 0x2B2233, true, false);
  snprintf(buf, sizeof(buf), "%d right", run_score_);
  text(c, 180, 200, buf, 2, 0x7CFF8A);
  snprintf(buf, sizeof(buf), "%ds left", secs < 0 ? 0 : secs);
  text(c, 180, 222, buf, 2, secs <= 10 ? 0xFF8A8A : 0xC9C2E8);
  for (int k = 0; k < 12; k++) {
    int kx, ky, kw;
    facts_key_pos(k, kx, ky, kw);
    uint32_t face = k == 10 ? 0xFFD0E4 : k == 11 ? 0xFFE3B0 : 0xFFF6E0;
    circle(c, kx, ky, FK_KEY_R + 2, 0x2B2233);
    circle(c, kx, ky, FK_KEY_R - 1, face);
    if (k < 10) {
      char d[2] = {(char) ('0' + k), 0};
      text(c, kx, ky - 13, d, 4, 0x2B2233, true, false);
    } else {
      text(c, kx, ky - 7, k == 10 ? "del" : "skip", 2, 0x2B2233, true, false);
    }
  }
}

// --- Shop: food is eaten at once; toys and hats are kept, and pass down the family. ---

// What eating a food item does: bought in the shop or at the stand, or found in a Chant gift.
void PetEngine::food_effect(int id) {
  switch (id) {
    case IT_CAKE:
      add(s_.happy, 30);
      add(s_.food, 10);
      s_.weight = (uint8_t) (s_.weight + 2);
      break;
    case IT_STEAK:
      add(s_.food, 50);
      s_.weight++;
      break;
    case IT_JUICE:
      add(s_.energy, 40);
      add(s_.happy, 5);
      break;
    case IT_BRAIN_SNACK:  // blueberries: good for the brain
      s_.smarts_x10 = (uint16_t) (s_.smarts_x10 + 20 > 1000 ? 1000 : s_.smarts_x10 + 20);
      add(s_.happy, 10);
      add(s_.food, 5);
      break;
    case IT_STAR_COOKIE:
      s_.smarts_x10 = (uint16_t) (s_.smarts_x10 + 10 > 1000 ? 1000 : s_.smarts_x10 + 10);
      add(s_.happy, 10);
      break;
    case IT_ENERGY_BAR: add(s_.energy, 30); break;
    case IT_MILK:
      add(s_.health, 10);
      add(s_.food, 10);
      break;
    case IT_SODA: add(s_.happy, 15); break;
    case IT_SUSHI: add(s_.food, 30); break;
    case IT_DONUT:
      add(s_.happy, 20);
      s_.weight = (uint8_t) (s_.weight + 2);
      break;
    case IT_PIZZA:
      add(s_.food, 40);
      add(s_.happy, 10);
      s_.weight++;
      break;
    case IT_ASTRO:
      add(s_.happy, 20);
      add(s_.energy, 10);
      break;
    case IT_ICE_CREAM:
      add(s_.happy, 25);
      add(s_.food, 5);
      break;
    case IT_BURGER:
      add(s_.food, 45);
      s_.weight = (uint8_t) (s_.weight + 2);
      break;
    default:  // candy
      add(s_.happy, 10);
      break;
  }
}

bool PetEngine::buy(int id) {
  if (id < 0 || id >= IT_COUNT) return false;
  const ShopItem &it = SHOP[id];
  bool food = is_food(id), hat = is_hat(id);
  char msg[40];
  if (is_prize(id) && !owns(id)) {
    snprintf(msg, sizeof(msg), "Win it: %s", PRIZE_HOW[id - IT_WIZARD_HAT]);
    show_toast(msg, 2500);
    return false;
  }
  if (hat && owns(id)) {  // put it on or take it off
    s_.hat = s_.hat == id ? 0 : (uint8_t) id;
    show_toast(s_.hat ? "Looking good!" : "Hat off", 1500);
    dirty_ = true;
    return true;
  }
  if (!food && owns(id)) {
    show_toast(is_prize(id) ? "On the shelf" : "Already yours", 1500);
    return false;
  }
  if (s_.dead || s_.stage == STAGE_EGG) {
    show_toast("Not yet!", 1500);
    return false;
  }
  if (food && s_.asleep) {
    show_toast("Zzz...", 1500);
    return false;
  }
  if (s_.coins < it.price) {
    snprintf(msg, sizeof(msg), "Need %d more coins", it.price - s_.coins);
    show_toast(msg, 2000);
    return false;
  }
  s_.coins = (uint16_t) (s_.coins - it.price);
  if (food) {
    food_effect(id);
    sound(SND_EAT);
    if (overlay_ == OV_STAND) {  // keep the stand open
      snprintf(msg, sizeof(msg), id == IT_BRAIN_SNACK ? "Smarts +2!" : id == IT_STAR_COOKIE ? "Smarts +1!" : "Yum!");
    } else {
      eat_icon_ = it.icon;
      page_ = 0;  // go and watch it eat
      overlay_ = OV_NONE;
      set_activity(ACT_EAT, 3000);
      snprintf(msg, sizeof(msg), "Yum, %s!", it.name);
    }
  } else {
    give(id);
    if (hat) s_.hat = (uint8_t) id;
    sound(SND_HAPPY);
    snprintf(msg, sizeof(msg), "Got the %s!", it.name);
  }
  show_toast(msg, 2000);
  dirty_ = true;
  return true;
}

static const int SHOP_TAB_Y = 80, SHOP_ROW1 = 152, SHOP_ROW2 = 254, SHOP_R = 46;
static int shop_tab_x(int t) { return 60 + t * 60; }
// Food, toys and hats: four big circles. Prizes: six smaller ones, three across.
static int shop_count(int) { return 6; }
static int shop_item(int tab, int i) { return SHOP_TAB_ITEMS[tab][i]; }
static void shop_slot(int, int i, int &x, int &y, int &r) {  // every tab: three across, two rows
  x = 96 + (i % 3) * 84;
  y = i / 3 ? 246 : 150;
  r = 39;
}

void PetEngine::shop_tap(int x, int y) {
  for (int t = 0; t < SHOP_TAB_COUNT; t++)
    if (abs(x - shop_tab_x(t)) < 30 && abs(y - SHOP_TAB_Y) < 18) {
      shop_tab_ = (uint8_t) t;
      shop_sel_ = -1;
      return;
    }
  for (int i = 0; i < shop_count(shop_tab_); i++) {
    int sx, sy, sr;
    shop_slot(shop_tab_, i, sx, sy, sr);
    if (sq(x - sx) + sq(y - sy) > (sr + 4) * (sr + 4)) continue;
    int id = shop_item(shop_tab_, i);
    bool kept = !is_food(id);
    if (kept && owns(id)) {  // hats: wear / take off; toys: already there
      buy(id);
    } else if (is_prize(id)) {  // locked: show how to win it
      shop_sel_ = (int8_t) id;
      shop_sel_t_ = now_;
    } else if (shop_sel_ == id && now_ - shop_sel_t_ < 4000) {
      buy(id);  // second tap: buy
      shop_sel_ = -1;
    } else {
      shop_sel_ = (int8_t) id;  // first tap: pick it
      shop_sel_t_ = now_;
    }
    return;
  }
  shop_sel_ = -1;
}

void PetEngine::draw_shop(Canvas &c, uint32_t now) {
  rect(c, 0, 0, SCREEN, SCREEN, 0x1B1530);
  char buf[32];
  // coins
  snprintf(buf, sizeof(buf), "%u", (unsigned) s_.coins);
  int w = (int) strlen(buf) * 18 + 24;
  icon(c, art::COIN, 7, 180 - w / 2, 22, 3);
  text(c, 180 - w / 2 + 26, 22, buf, 3, 0xFFD23F, false);
  // tabs
  for (int t = 0; t < SHOP_TAB_COUNT; t++) {
    int tx = shop_tab_x(t);
    bool on = t == shop_tab_;
    rect(c, tx - 29, SHOP_TAB_Y - 14, 58, 28, on ? 0xFFD23F : (t == SHOP_PRIZE_TAB ? 0x4A3D20 : 0x3D3350));
    text(c, tx, SHOP_TAB_Y - 7, SHOP_TABS[t], 2, on ? 0x2B2233 : 0xC9C2E8, true, false);
  }
  // items
  bool sel_live = shop_sel_ >= 0 && now - shop_sel_t_ < 4000;
  for (int i = 0; i < shop_count(shop_tab_); i++) {
    int id = shop_item(shop_tab_, i), sx, sy, sr;
    shop_slot(shop_tab_, i, sx, sy, sr);
    const ShopItem &it = SHOP[id];
    bool picked = sel_live && shop_sel_ == id, have = !is_food(id) && owns(id), worn = s_.hat == id && have;
    circle(c, sx, sy, sr + 3, picked ? 0xFFD23F : 0x2B2233);
    circle(c, sx, sy, sr, worn ? 0xD8F0C8 : 0xFFF6E0);
    bool locked = is_prize(id) && !have;
    bool small = sr < SHOP_R;
    if (small)
      icon(c, it.icon, it.rows, sx - 12, sy - 34, 2, 99, locked);
    else
      icon(c, it.icon, it.rows, sx - 18, sy - 40, 3, 99, locked);
    text(c, sx, small ? sy - 6 : sy, it.name, 2, 0x2B2233, true, false);
    if (worn)
      snprintf(buf, sizeof(buf), "ON");
    else if (have)
      snprintf(buf, sizeof(buf), is_hat(id) ? "OWN" : "HAVE");
    else if (locked)
      snprintf(buf, sizeof(buf), "LOCKED");
    else
      snprintf(buf, sizeof(buf), "%uc", (unsigned) it.price);
    bool afford = have || (!locked && s_.coins >= it.price);
    if (small)
      text(c, sx, sy + 14, buf, 1, afford ? 0x3A8A3A : 0xB02A36, true, false);
    else
      text(c, sx, sy + 18, buf, 2, afford ? 0x3A8A3A : 0xB02A36, true, false);
  }
  // hint
  const char *hint = shop_tab_ == 0   ? "food is eaten now"
                     : shop_tab_ == 1 ? "toys help it out"
                     : shop_tab_ == 2 ? "tap owned: wear"
                     : shop_tab_ == 3 ? "for its shelf"
                                      : "win them in math";
  if (sel_live && is_prize(shop_sel_)) {
    snprintf(buf, sizeof(buf), "Win: %s", PRIZE_HOW[shop_sel_ - IT_WIZARD_HAT]);
    hint = buf;
  } else if (sel_live) {
    snprintf(buf, sizeof(buf), "Tap again: %uc", (unsigned) SHOP[shop_sel_].price);
    hint = buf;
  }
  text(c, 180, 310, hint, 2, sel_live ? 0xFFD23F : 0x9A90B0, true, false);
}

// Hats from the prize stand pages are drawn over the pet from their shop art, sitting on its head by their lowest
// row. (ox, oy) is where the pet's 32x32 sprite was drawn at scale sc.
void PetEngine::draw_overlay_hat(Canvas &c, int ox, int oy, int sc) {
  if (!is_overlay_hat(s_.hat) || s_.dead || s_.stage == STAGE_EGG) return;
  const ShopItem &it = SHOP[s_.hat];
  int last = 0;
  for (int r = 0; r < it.rows; r++)
    for (const char *p = it.icon[r]; *p; p++)
      if (*p != '.') {
        last = r;
        break;
      }
  float rx, ry;
  body_size(s_.g, s_.stage, s_.form, rx, ry);
  int top = (int) floorf(28.5f - 2 * ry);
  icon(c, it.icon, it.rows, ox + 10 * sc, oy + (top + 1 - last) * sc, sc);
}

// --- Prize stand: opens after every math game; sells things the shop doesn't. ---

static const int STAND_DONE_Y = 324;

void PetEngine::stand_tap(int x, int y) {
  overlay_since_ = now_;  // still here: don't close
  if (abs(x - 180) < 64 && abs(y - STAND_DONE_Y) < 20) {
    overlay_ = OV_NONE;
    shop_sel_ = -1;
    return;
  }
  for (int i = 0; i < STAND_PAGE; i++) {
    int sx, sy, sr;
    shop_slot(3, i, sx, sy, sr);  // the same 3x2 grid as the prize tab
    if (sq(x - sx) + sq(y - sy) > (sr + 4) * (sr + 4)) continue;
    int id = IT_PROP_CAP + stand_page_ * STAND_PAGE + i;
    if (!is_food(id) && owns(id)) {
      buy(id);  // a hat: on or off; the others: already on the shelf
    } else if (stand_bought_) {
      show_toast("1 per game: play again!", 2000);
    } else if (shop_sel_ == id && now_ - shop_sel_t_ < 4000) {
      if (buy(id)) {  // the one buy for this game
        stand_bought_ = true;
        stand_got_ = (int8_t) id;
      }
      shop_sel_ = -1;
    } else {
      shop_sel_ = (int8_t) id;
      shop_sel_t_ = now_;
    }
    return;
  }
  shop_sel_ = -1;
}

void PetEngine::draw_stand(Canvas &c, uint32_t now) {
  uint32_t board = STAND_BOARD[stand_page_], dark = mix(board, 0x000000, 0.4f);
  rect(c, 0, 0, SCREEN, SCREEN, board);
  char buf[32];
  text(c, 180, 26, STAND_TITLE[stand_page_], 2, 0xFFF6C8);
  snprintf(buf, sizeof(buf), "%u", (unsigned) s_.coins);
  int w = (int) strlen(buf) * 18 + 24;
  icon(c, art::COIN, 7, 180 - w / 2, 50, 3);
  text(c, 180 - w / 2 + 26, 50, buf, 3, 0xFFD23F, false);
  bool sel_live = shop_sel_ >= 0 && now - shop_sel_t_ < 4000;
  for (int i = 0; i < STAND_PAGE; i++) {
    int id = IT_PROP_CAP + stand_page_ * STAND_PAGE + i, sx, sy, sr;
    shop_slot(3, i, sx, sy, sr);
    const ShopItem &it = SHOP[id];
    bool have = !is_food(id) && owns(id), worn = have && s_.hat == id, picked = sel_live && shop_sel_ == id;
    bool got = stand_got_ == id, closed = stand_bought_ && !have && !got;  // the one buy is used up
    circle(c, sx, sy, sr + 3, picked || got ? 0xFFD23F : dark);
    circle(c, sx, sy, sr, worn || got ? 0xD8F0C8 : (closed ? 0xC8C4CC : 0xFFF6E0));
    icon(c, it.icon, it.rows, sx - 12, sy - 34, 2, 99, closed);
    text(c, sx, sy - 6, it.name, 2, 0x2B2233, true, false);
    if (got)
      snprintf(buf, sizeof(buf), "GOT IT");
    else if (worn)
      snprintf(buf, sizeof(buf), "ON");
    else if (have)
      snprintf(buf, sizeof(buf), is_hat(id) ? "OWN" : "HAVE");
    else
      snprintf(buf, sizeof(buf), "%uc", (unsigned) it.price);
    bool ok = got || have || (!closed && s_.coins >= it.price);
    text(c, sx, sy + 14, buf, 1, ok ? 0x3A8A3A : 0xB02A36, true, false);
  }
  const char *hint = stand_bought_ ? "1 per game!" : "pick 1 prize!";
  if (sel_live && !stand_bought_) {
    snprintf(buf, sizeof(buf), "Tap again: %uc", (unsigned) SHOP[shop_sel_].price);
    hint = buf;
  }
  text(c, 180, 292, hint, 2, sel_live ? 0xFFD23F : 0xC9C2E8, true, false);
  rect(c, 180 - 60, STAND_DONE_Y - 15, 120, 30, dark);
  rect(c, 180 - 57, STAND_DONE_Y - 12, 114, 24, 0xFFF6E0);
  text(c, 180, STAND_DONE_Y - 7, "Done", 2, 0x2B2233, true, false);
}

// --- Times-table page ---
// The table laid out like a clock: x1 at one o'clock round to x12 at twelve, so the whole table is on the screen
// at once. Opening the page (or a new table) sweeps a light round the ring and counts up as each answer pops in.
// Tapping an answer shows the fact in the middle with a picture of it: that many rows of dots.

static const int TT_R = 142, TT_BR = 23;  // ring radius and answer-bubble radius
static const int TT_STEP_MS = 160;         // sweep time per answer
static const int TT_INNER = 112;           // middle area radius
static const int TT_UP_Y = 82, TT_DOWN_Y = 278;
static const uint32_t TT_COLORS[12] = {0xF0454B, 0xFF9A3C, 0xFFC93C, 0x8BD346, 0x4CC25A, 0x2EC4B6,
                                       0x4AA8FF, 0x6C7BFF, 0xA66CFF, 0xE36CD9, 0xFF7EB0, 0xC9A26B};

static void tt_pos(float k, int &x, int &y) {
  float a = k * 3.14159265f / 6.0f;  // k = 1..12, clockwise from twelve o'clock
  x = 180 + (int) lroundf(TT_R * sinf(a));
  y = 180 - (int) lroundf(TT_R * cosf(a));
}

void PetEngine::tables_start_sweep() {
  tt_sweep_ms_ = now_ ? now_ : 1;
  tt_sel_ = 0;
}

// Instant, with no animation, so a drag can run through the tables quickly. A picked fact stays picked
// (7 x 8 becomes 8 x 8).
void PetEngine::tables_step(int steps) {
  tt_table_ = (uint8_t) ((tt_table_ - 1 + steps % 12 + 12) % 12 + 1);
  tt_sweep_ms_ = 0;
  tt_sel_ms_ = 0;
}

bool PetEngine::tables_animating(uint32_t now) const {
  if (tt_sweep_ms_ && now - tt_sweep_ms_ < (uint32_t) (12 * TT_STEP_MS + 600)) return true;
  return tt_sel_ && tt_sel_ms_ && now - tt_sel_ms_ < 1200;
}

void PetEngine::tables_tap(int x, int y) {
  for (int k = 1; k <= 12; k++) {
    int bx, by;
    tt_pos((float) k, bx, by);
    if (sq(x - bx) + sq(y - by) < (TT_BR + 8) * (TT_BR + 8)) {
      tt_sweep_ms_ = 0;  // a tap mid-sweep shows everything
      tt_sel_ = tt_sel_ == k ? 0 : (uint8_t) k;
      tt_sel_ms_ = now_ ? now_ : 1;
      return;
    }
  }
  if (abs(x - 180) < 50 && abs(y - TT_UP_Y) < 26) {
    tables_step(1);
    return;
  }
  if (abs(x - 180) < 50 && abs(y - TT_DOWN_Y) < 26) {
    tables_step(-1);
    return;
  }
  if (sq(x - 180) + sq(y - 180) < TT_INNER * TT_INNER) {
    if (tt_sel_)
      tt_sel_ = 0;  // back to the table
    else
      tables_start_sweep();  // count it up again
  }
}

void PetEngine::draw_tables(Canvas &c, uint32_t now) {
  const int n = tt_table_;
  const uint32_t col = TT_COLORS[(n - 1) % 12];
  rect(c, 0, 0, SCREEN, SCREEN, 0x16122A);
  circle(c, 180, 180, TT_R + TT_BR + 6, 0x221B3A);  // the ring the answers sit on
  circle(c, 180, 180, TT_INNER + 2, 0x16122A);

  // Sweep: how many answers are out, and where the light is.
  int32_t el = tt_sweep_ms_ ? (int32_t) (now - tt_sweep_ms_) : (1 << 30);
  bool sweeping = el < 12 * TT_STEP_MS + 600;
  int shown = sweeping ? el / TT_STEP_MS + 1 : 12;
  if (shown > 12) shown = 12;
  float head = (float) el / TT_STEP_MS + 1.0f;  // the light's position, in answers

  // The light's trail, under the bubbles.
  if (sweeping && head <= 12.6f) {
    for (int i = 6; i >= 1; i--) {
      float k = head - i * 0.09f;
      if (k < 0.5f) continue;
      int tx, ty;
      tt_pos(k, tx, ty);
      circle(c, tx, ty, 3 + (6 - i), mix(0x221B3A, col, 0.25f + 0.1f * (6 - i)));
    }
  }

  char buf[24];
  for (int k = 1; k <= 12; k++) {
    int bx, by;
    tt_pos((float) k, bx, by);
    snprintf(buf, sizeof(buf), "x%d", k);
    if (k > shown) {  // not reached yet
      circle(c, bx, by, TT_BR, 0x2E2650);
      text(c, bx, by - 3, buf, 1, 0x6A6480, true, false);
      continue;
    }
    int r = TT_BR;
    int32_t t = sweeping ? el - (k - 1) * TT_STEP_MS : 1000;
    if (t >= 0 && t < 260) r = (int) lroundf(TT_BR * (1.0f + 0.35f * sinf(3.14159265f * t / 260.0f)));
    bool sel = k == tt_sel_;
    if (sel) circle(c, bx, by, r + 5, 0xFFFFFF);
    circle(c, bx, by, r + 2, 0x1A1426);
    circle(c, bx, by, r, sel ? mix(col, 0xFFFFFF, 0.35f) : col);
    snprintf(buf, sizeof(buf), "%d", n * k);
    text(c, bx, by - 12, buf, 2, 0xFFFFFF, true, true);
    snprintf(buf, sizeof(buf), "x%d", k);
    text(c, bx, by + 6, buf, 1, 0x2B2233, true, false);
  }

  // The light itself, on top.
  if (sweeping && head <= 12.6f) {
    int lx, ly;
    tt_pos(head, lx, ly);
    circle(c, lx, ly, 10, mix(col, 0xFFFFFF, 0.5f));
    circle(c, lx, ly, 6, 0xFFFFFF);
  }

  // Up and down arrows for the next and previous tables, with their numbers.
  auto arrow = [&](int cy, bool up) {
    for (int i = 0; i < 9; i++) {
      int w = 2 + i * 2;
      rect(c, 180 - w / 2, up ? cy - 6 + i : cy + 6 - i, w, 1, 0x8A80B0);
    }
  };
  arrow(TT_UP_Y - 8, true);
  arrow(TT_DOWN_Y + 8, false);
  snprintf(buf, sizeof(buf), "%d", n % 12 + 1);
  text(c, 180, TT_UP_Y + 5, buf, 1, 0x8A80B0, true, false);
  snprintf(buf, sizeof(buf), "%d", (n + 10) % 12 + 1);
  text(c, 180, TT_DOWN_Y - 12, buf, 1, 0x8A80B0, true, false);

  // The middle.
  if (tt_sel_) {
    // A picture of n x k: k rows of n dots, the rows appearing one by one.
    int k = tt_sel_, big = n > k ? n : k;
    int p = 86 / big;
    if (p > 9) p = 9;
    int dot = p - 2 < 2 ? 2 : p - 2;
    int gw = n * p + (n - 1) / 5 * 2, gh = k * p + (k - 1) / 5 * 2;  // a small gap every five, for counting
    int ox = 180 - gw / 2, oy = 148 - gh / 2;
    int rows = tt_sel_ms_ ? (int) ((now - tt_sel_ms_) / 70) + 1 : k;  // row by row just after a tap
    for (int r = 0; r < k && r < rows; r++)
      for (int q = 0; q < n; q++)
        rect(c, ox + q * p + q / 5 * 2, oy + r * p + r / 5 * 2, dot, dot, r % 2 ? mix(col, 0xFFFFFF, 0.3f) : col);
    snprintf(buf, sizeof(buf), "%d x %d = %d", n, k, n * k);
    text(c, 180, 202, buf, 3, 0xFFFFFF);
    snprintf(buf, sizeof(buf), "%d row%s of %d", k, k == 1 ? "" : "s", n);
    text(c, 180, 234, buf, 2, 0xC9C2E8, true, false);
  } else if (sweeping) {
    // Counting up: the fact the light just reached, big.
    snprintf(buf, sizeof(buf), "%d x %d", n, shown);
    text(c, 180, 118, buf, 3, 0xC9C2E8);
    snprintf(buf, sizeof(buf), "%d", n * shown);
    text(c, 180, 152, buf, 7, col);
    text(c, 180, 220, "counting up...", 2, 0x8A80B0, true, false);
  } else {
    text(c, 180, 116, "TIMES TABLE", 2, 0xC9C2E8, true, false);
    snprintf(buf, sizeof(buf), "%d", n);
    text(c, 180, 140, buf, 8, col);
    text(c, 180, 208, "tap an answer", 2, 0x8A80B0, true, false);
    text(c, 180, 228, "drag up/down", 2, 0x8A80B0, true, false);
  }
}

// --- Chant: the times tables by heart ---
// Pick a table, then its twelve answers sit round a circle: tap them in order (7, 14, 21...), or say them out
// loud. The first time they go round in order like a clock; once the table's medal is won they are shuffled, so
// you have to know what comes next. Nothing is lost for a wrong tap, and if you get stuck the next one glows.
// Every table done brings a gift: its medal the first time, a treat after that.

static const char *const CH_NOTES[12] = {
    "n1:d=16,o=6,b=200:c",  "n2:d=16,o=6,b=200:d",  "n3:d=16,o=6,b=200:e",  "n4:d=16,o=6,b=200:f",
    "n5:d=16,o=6,b=200:g",  "n6:d=16,o=6,b=200:a",  "n7:d=16,o=6,b=200:b",  "n8:d=16,o=7,b=200:c",
    "n9:d=16,o=7,b=200:d",  "n10:d=16,o=7,b=200:e", "n11:d=16,o=7,b=200:f", "n12:d=16,o=7,b=200:g"};
static const char *const SND_GIFT = "gift:d=16,o=6,b=180:c,e,g,c7,p,g,c7";
static const int CH_HINT_MS = 4000;      // stuck this long: the next answer glows
static const int CH_DROP_MS = 600, CH_OPEN_MS = 1300, CH_BUTTONS_MS = 2200;  // the gift's timeline
static const int CH_BTN_Y = 282, CH_BTN_W = 70, CH_BTN_H = 36;
static const int CH_BTN_X[3] = {103, 180, 257};  // again, next, done
static const int CH_MIC_Y = 214;
static const int CH_QUIT_Y = 272, CH_HARD_Y = 150;  // the exit / back pill, and the hard switch on the picker
// Treats a gift can hold (a medal comes first, once per table).
static const uint8_t CH_TREATS[] = {IT_CAKE, IT_JUICE, IT_CANDY, IT_DONUT, IT_SODA, IT_ASTRO, IT_STAR_COOKIE, IT_PIZZA};

static uint32_t max3(uint32_t a, uint32_t b, uint32_t c) { return a > b ? (a > c ? a : c) : (b > c ? b : c); }

int PetEngine::chant_medals() const {
  int n = 0;
  for (int i = 0; i < 12; i++) n += (s_.chant_medals >> i) & 1;
  return n;
}

void PetEngine::set_voice_enabled(bool on) {
  if (on)
    s_.settings &= (uint8_t) ~2;
  else
    s_.settings |= 2;
  dirty_ = true;
}

bool PetEngine::wants_listening() const {
  if (overlay_ != OV_GAME || game_kind_ != GAME_CHANT || !voice_enabled() || !voice_link_) return false;
  if ((int32_t) (ch_err_until_ - now_) > 0) return false;  // a voice error: wait a bit before trying again
  return now_ - max3(ch_ok_ms_, ch_heard_ms_, last_user_) < 90000;  // stop listening when nobody's there
}

void PetEngine::voice_error(const char *code) {
  if (code == nullptr) return;
  // Silence and "didn't catch that" are normal while someone thinks; only real failures pause the mic.
  // "not-connected" is a start that did not get through this time: the next one is tried straight away.
  if (strstr(code, "no-text") || strstr(code, "timeout") || strstr(code, "no_text") || strstr(code, "cancel") ||
      strstr(code, "not-connected"))
    return;
  ch_err_until_ = now_ + 15000;
  snprintf(ch_heard_, sizeof(ch_heard_), "voice: %.34s", code);
  ch_heard_ms_ = now_ ? now_ : 1;
}

// Number words to numbers. "seven fourteen twenty one" is 7, 14, 21: a new number starts whenever the next word
// can't add to the one being built (a unit after a unit, a teen or a ten after anything but a hundred).
int PetEngine::parse_numbers(const char *text, int *out, int max) {
  static const char *const UNITS[] = {"zero", "one", "two",   "three", "four", "five",   "six",
                                      "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen",
                                      "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
  static const char *const TENS[] = {"twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
  struct Alias {
    const char *word;
    int value;
  };
  // Words speech-to-text writes for numbers it heard ("for" four, "ate" eight...).
  static const Alias ALIASES[] = {{"for", 4}, {"fore", 4}, {"to", 2}, {"too", 2}, {"ate", 8},
                                  {"won", 1}, {"fourty", 40}, {"twelfth", 12}, {"oh", 0}};
  int n = 0, cur = -1;  // cur: the number being built, -1 none
  auto emit = [&]() {
    if (cur >= 0 && n < max) out[n++] = cur;
    cur = -1;
  };
  const char *p = text ? text : "";
  while (*p) {
    if (isdigit((unsigned char) *p)) {
      emit();
      long v = 0;
      while (isdigit((unsigned char) *p) || (*p == ',' && isdigit((unsigned char) p[1]) && isdigit((unsigned char) p[2]) &&
                                             isdigit((unsigned char) p[3]) && !isdigit((unsigned char) p[4]))) {
        if (*p != ',') v = v * 10 + (*p - '0');
        if (v > 100000) v = 100000;
        p++;
      }
      if (n < max) out[n++] = (int) v;
      continue;
    }
    if (!isalpha((unsigned char) *p)) {
      p++;
      continue;
    }
    char w[16];
    int len = 0;
    while (isalpha((unsigned char) *p)) {
      if (len < 15) w[len++] = (char) tolower((unsigned char) *p);
      p++;
    }
    w[len] = 0;
    int val = -1;
    for (int i = 0; i < 20 && val < 0; i++)
      if (!strcmp(w, UNITS[i])) val = i;
    for (int i = 0; i < 8 && val < 0; i++)
      if (!strcmp(w, TENS[i])) val = 20 + i * 10;
    for (const auto &a : ALIASES)
      if (val < 0 && !strcmp(w, a.word)) val = a.value;
    if (!strcmp(w, "hundred")) {
      if (cur < 0)
        cur = 100;  // "a hundred", "hundred"
      else if (cur >= 1 && cur <= 9)
        cur *= 100;
      else {
        emit();
        cur = 100;
      }
      continue;
    }
    if (val < 0) {
      // "and" or "a" inside "a hundred and eight" keeps the number going; any other word ends it.
      if (!strcmp(w, "and") || !strcmp(w, "a")) continue;
      emit();
      continue;
    }
    if (cur < 0) {
      cur = val;
    } else if (val < 10 && cur >= 20 && cur % 10 == 0 && cur % 100 != 10) {
      cur += val;  // twenty one, a hundred and six
    } else if (val < 100 && cur >= 100 && cur % 100 == 0) {
      cur += val;  // a hundred and twelve, one hundred twenty
    } else if (val < 10 && cur >= 100 && cur % 10 == 0 && cur % 100 >= 20) {
      cur += val;  // one hundred twenty one
    } else {
      emit();
      cur = val;
    }
  }
  emit();
  return n;
}

void PetEngine::voice_heard(const char *text) {
  if (text == nullptr) return;
  size_t tlen = strlen(text);  // keep the end: with continuous listening the newest words are last
  snprintf(ch_heard_, sizeof(ch_heard_), "%s", tlen >= sizeof(ch_heard_) ? text + tlen - (sizeof(ch_heard_) - 1) : text);
  ch_heard_ms_ = now_ ? now_ : 1;
  if (overlay_ != OV_GAME || game_kind_ != GAME_CHANT) return;
  last_user_ = ch_heard_ms_;  // someone is talking to it: it is not alone
  char low[64];
  snprintf(low, sizeof(low), "%s", text);
  for (char *q = low; *q; q++) *q = (char) tolower((unsigned char) *q);
  int nums[24];
  int count = parse_numbers(text, nums, 24);
  if (ch_state_ == 0) {  // "seven" (or "the seven times table") picks a table
    for (int i = count - 1; i >= 0; i--)  // the last one said
      if (nums[i] >= 1 && nums[i] <= 12) {
        chant_begin(nums[i]);
        return;
      }
    return;
  }
  if (ch_state_ == 2) {
    if (now_ - ch_gift_ms_ < (uint32_t) CH_OPEN_MS) return;
    if (strstr(low, "again"))
      chant_begin(ch_table_);
    else if (strstr(low, "next"))
      chant_begin(ch_table_ % 12 + 1);
    else if (strstr(low, "done") || strstr(low, "stop"))
      finish_game();
    return;
  }
  // Chanting: every number that is the next answer counts; anything else ("seven times three is...") is ignored.
  for (int i = 0; i < count && ch_state_ == 1; i++)
    if (ch_next_ < 12 && nums[i] == ch_table_ * (ch_next_ + 1)) chant_hit(ch_next_ + 1);
}

void PetEngine::chant_begin(int table) {
  ch_epoch_++;
  ch_table_ = (uint8_t) table;
  ch_next_ = 0;
  ch_state_ = 1;
  ch_ok_ms_ = now_ ? now_ : 1;
  ch_wrong_ = -1;
  for (int k = 0; k < 12; k++) ch_slot_[k] = (uint8_t) (k + 1);
  if (ch_hard_) {  // hard: shuffled, so the next one has to be remembered
    for (int k = 11; k > 0; k--) {
      int j = (int) (rnd() % (uint32_t) (k + 1));
      uint8_t t = ch_slot_[k];
      ch_slot_[k] = ch_slot_[j];
      ch_slot_[j] = t;
    }
  }
}

void PetEngine::chant_hit(int k) {
  if (s_.math_need >= 2) s_.math_need -= 2;
  ch_next_ = (uint8_t) k;
  ch_ok_ms_ = now_ ? now_ : 1;
  ch_wrong_ = -1;
  sound(CH_NOTES[k - 1]);
  if (k == 12) chant_complete();
}

void PetEngine::chant_complete() {
  ch_epoch_++;
  ch_state_ = 2;
  ch_gift_ms_ = now_ ? now_ : 1;
  if (run_score_ < 65535) run_score_++;
  uint16_t bit = (uint16_t) (1u << (ch_table_ - 1));
  bool medal = !(s_.chant_medals & bit);
  ch_champion_ = false;
  int coins;
  if (medal) {
    s_.chant_medals |= bit;
    ch_gift_item_ = -1;
    coins = 10;
    for (int i = 0; i < 3; i++) learn();
    if ((s_.chant_medals & 0xFFF) == 0xFFF) {  // that was the last one
      ch_champion_ = true;
      coins += 50;
    }
  } else {
    ch_gift_item_ = (int8_t) CH_TREATS[rnd() % (sizeof(CH_TREATS) / sizeof(CH_TREATS[0]))];
    food_effect(ch_gift_item_);
    coins = 3;
    learn();
  }
  ch_gift_coins_ = (uint8_t) coins;
  s_.coins = (uint16_t) (s_.coins + coins > 9999 ? 9999 : s_.coins + coins);
  add(s_.happy, 10);
  sound(SND_GIFT, true);
  dirty_ = true;
}

void PetEngine::chant_update() {
  uint32_t idle = now_ - max3(ch_ok_ms_, ch_heard_ms_, last_user_);
  if (ch_state_ == 0 && idle > 60000) overlay_ = OV_NONE;  // never picked a table
  else if (idle > 300000) finish_game();                    // left alone for five minutes
}

void PetEngine::chant_press(int x, int y) {
  if (ch_state_ != 2 && abs(x - 180) < 70 && abs(y - (CH_MIC_Y + 14)) < 22) {
    set_voice_enabled(!voice_enabled());
    show_toast(voice_enabled() ? "Mic on" : "Mic off", 1200);
    return;
  }
  if (ch_state_ != 2 && abs(x - 180) < 48 && abs(y - CH_QUIT_Y) < 20) {
    if (ch_state_ == 1) {  // back to the table picker
      ch_state_ = 0;
      ch_epoch_++;
    } else if (run_score_ > 0) {
      finish_game();  // tables were done: coins and the prize stand
    } else {
      overlay_ = OV_NONE;
    }
    return;
  }
  if (ch_state_ == 0) {
    if (abs(x - 180) < 70 && abs(y - CH_HARD_Y) < 17) {
      ch_hard_ = !ch_hard_;
      return;
    }
    for (int k = 1; k <= 12; k++) {
      int bx, by;
      tt_pos((float) k, bx, by);
      if (sq(x - bx) + sq(y - by) < (TT_BR + 8) * (TT_BR + 8)) {
        chant_begin(k);
        return;
      }
    }
    return;
  }
  if (ch_state_ == 1) {
    for (int k = 1; k <= 12; k++) {
      int bx, by;
      tt_pos((float) ch_slot_[k - 1], bx, by);
      if (sq(x - bx) + sq(y - by) >= (TT_BR + 8) * (TT_BR + 8)) continue;
      if (k > ch_next_) {  // answers have to be said out loud: a tap only wiggles the bubble
        ch_wrong_ = (int8_t) k;
        ch_wrong_ms_ = now_ ? now_ : 1;
        sound(SND_KEY);
        show_toast("Say it out loud!", 1500);
      }
      return;
    }
    return;
  }
  // The gift: again, next table, or done.
  if (now_ - ch_gift_ms_ < (uint32_t) CH_BUTTONS_MS) return;
  for (int b = 0; b < 3; b++) {
    if (abs(x - CH_BTN_X[b]) > CH_BTN_W / 2 + 4 || abs(y - CH_BTN_Y) > CH_BTN_H / 2 + 6) continue;
    if (b == 0)
      chant_begin(ch_table_);
    else if (b == 1)
      chant_begin(ch_table_ % 12 + 1);
    else
      finish_game();
    return;
  }
}

// The mic and what it heard, under the middle of the Chant screens.
void PetEngine::draw_mic_status(Canvas &c, uint32_t now, int y) {
  bool on = voice_enabled(), want = wants_listening();
  bool err = (int32_t) (ch_err_until_ - now) > 0;
  const char *label;
  uint32_t col = 0x8A80B0;
  if (!on) {
    label = "mic off";
  } else if (!voice_link_) {
    label = "tap only";  // no Home Assistant connection: no speech-to-text
  } else if (err) {
    label = "mic paused";
    col = 0xFF9A3C;
  } else if (want && thinking_) {
    label = "thinking...";  // Home Assistant is turning the speech into text
    col = 0xFFD23F;
  } else if (want && listening_) {
    label = "listening";
    col = 0x7CFF8A;
  } else if (want) {
    label = "mic starting";
  } else {
    label = "tap mic";
  }
  int w = 24 + 6 + (int) strlen(label) * 12;
  int x0 = 180 - w / 2;
  if (on && voice_link_ && want && listening_ && !thinking_) {
    int r = 15 + (int) (2 * sinf(now / 160.0f));
    circle(c, x0 + 12, y + 12, r, 0x1F4A2E);
  }
  icon(c, art::MIC, 12, x0, y, 2, 99, !(on && voice_link_));
  text(c, x0 + 30, y + 6, label, 2, col, false, false);
  if (ch_heard_[0] && now - ch_heard_ms_ < 6000) {
    char buf[30];
    size_t hl = strlen(ch_heard_);
    snprintf(buf, sizeof(buf), "\"%.24s\"", hl > 24 ? ch_heard_ + hl - 24 : ch_heard_);
    text(c, 180, y + 32, buf, 1, 0xC9C2E8, true, false);
  }
}

void PetEngine::draw_chant(Canvas &c, uint32_t now) {
  rect(c, 0, 0, SCREEN, SCREEN, 0x16122A);
  circle(c, 180, 180, TT_R + TT_BR + 6, 0x221B3A);
  circle(c, 180, 180, TT_INNER + 2, 0x16122A);
  char buf[48];

  if (ch_state_ == 0) {  // pick a table; tables with a medal are gold
    for (int k = 1; k <= 12; k++) {
      int bx, by;
      tt_pos((float) k, bx, by);
      bool medal = s_.chant_medals & (1u << (k - 1));
      circle(c, bx, by, TT_BR + 2, 0x1A1426);
      circle(c, bx, by, TT_BR, medal ? 0xFFD23F : TT_COLORS[k - 1]);
      snprintf(buf, sizeof(buf), "%d", k);
      text(c, bx, by - 10, buf, 3, medal ? 0x2B2233 : 0xFFFFFF, true, !medal);
      if (medal) {  // a little rosette
        circle(c, bx + 15, by - 15, 5, 0xFFFFFF);
        circle(c, bx + 15, by - 15, 3, 0xF0454B);
      }
    }
    text(c, 180, 84, ch_hard_ ? "CHANT HARD" : "CHANT", 3, 0xFFFFFF);
    text(c, 180, 114, "pick a table", 2, 0xC9C2E8, true, false);
    // The hard switch: answers in order (off) or shuffled round the circle (on).
    rect(c, 180 - 62, CH_HARD_Y - 13, 124, 26, 0x1A1426);
    rect(c, 180 - 60, CH_HARD_Y - 11, 120, 22, ch_hard_ ? 0xF0454B : 0x3D3350);
    text(c, 180, CH_HARD_Y - 7, ch_hard_ ? "HARD: ON" : "HARD: OFF", 2, 0xFFFFFF, true, false);
    snprintf(buf, sizeof(buf), "medals %d/12", chant_medals());
    text(c, 180, 180, buf, 2, 0xFFD23F, true, false);
    draw_mic_status(c, now, CH_MIC_Y);
    rect(c, 180 - 40, CH_QUIT_Y - 13, 80, 26, 0x1A1426);
    rect(c, 180 - 38, CH_QUIT_Y - 11, 76, 22, 0x8A80B0);
    text(c, 180, CH_QUIT_Y - 7, "EXIT", 2, 0xFFFFFF, true, false);
    return;
  }

  const int n = ch_table_;
  const uint32_t col = TT_COLORS[(n - 1) % 12];
  bool gift = ch_state_ == 2;
  if (gift) {  // a party ring of sparkles turning slowly round the gift, instead of the answers
    static const uint32_t SPARK_RING[3] = {0xFFD23F, 0xFFFFFF, 0xFF9A3C};
    for (int i = 0; i < 18; i++) {
      float a = i * 0.349f + now / 2400.0f;
      int r = 4 + (int) (2 * (1 + sinf(now / 200.0f + i)));
      circle(c, 180 + (int) (155 * cosf(a)), 180 + (int) (155 * sinf(a)), r, i % 3 == 2 ? col : SPARK_RING[i % 3]);
    }
  }
  for (int k = 1; k <= 12 && !gift; k++) {
    int bx, by;
    tt_pos((float) ch_slot_[k - 1], bx, by);
    bool done = k <= ch_next_;
    int r = TT_BR;
    if (done && k == ch_next_ && now - ch_ok_ms_ < 250) r = (int) lroundf(TT_BR * (1.0f + 0.3f * sinf(3.14159265f * (now - ch_ok_ms_) / 250.0f)));
    if (k == ch_wrong_ && now - ch_wrong_ms_ < 320) bx += (int) (5 * sinf((now - ch_wrong_ms_) / 25.0f));
    bool hint = !gift && k == ch_next_ + 1 && now - ch_ok_ms_ > (uint32_t) CH_HINT_MS;
    if (hint) circle(c, bx, by, r + 5 + (int) (3 * (1 + sinf(now / 150.0f))), mix(0x221B3A, col, 0.8f));
    circle(c, bx, by, r + 2, 0x1A1426);
    circle(c, bx, by, r, done ? col : 0xFFF6E0);
    snprintf(buf, sizeof(buf), "%d", n * k);
    text(c, bx, by - 7, buf, 2, done ? 0xFFFFFF : 0x2B2233, true, done);
  }

  if (!gift) {
    snprintf(buf, sizeof(buf), "TIMES %d", n);
    text(c, 180, 92, buf, 2, 0xC9C2E8, true, false);
    snprintf(buf, sizeof(buf), "%d x %d = ?", n, ch_next_ + 1);
    text(c, 180, 116, buf, 3, 0xFFFFFF);
    // The chant so far: the last few answers, then the one to come.
    int first = ch_next_ > 4 ? ch_next_ - 3 : 1;
    int len = 0;
    buf[0] = 0;
    for (int k = first; k <= ch_next_; k++) len += snprintf(buf + len, sizeof(buf) - len, "%d ", n * k);
    snprintf(buf + len, sizeof(buf) - len, "?");
    text(c, 180, 152, buf, 2, col, true, false);
    snprintf(buf, sizeof(buf), "%d of 12", ch_next_);
    text(c, 180, 180, buf, 2, 0x8A80B0, true, false);
    draw_mic_status(c, now, CH_MIC_Y);
    rect(c, 180 - 40, CH_QUIT_Y - 13, 80, 26, 0x1A1426);
    rect(c, 180 - 38, CH_QUIT_Y - 11, 76, 22, 0x8A80B0);
    text(c, 180, CH_QUIT_Y - 7, "BACK", 2, 0xFFFFFF, true, false);
    return;
  }

  // The gift: a box drops in, shakes, pops open, and out comes the medal or a treat.
  uint32_t t = now - ch_gift_ms_;
  circle(c, 180, 180, TT_INNER + 2, 0x16122A);
  int bx = 180, by = 150;
  if (t < (uint32_t) CH_DROP_MS) {
    float f = 1.0f - t / (float) CH_DROP_MS;
    by -= (int) (f * f * 130);
  } else if (t < (uint32_t) CH_OPEN_MS) {
    bx += (int) (4 * sinf(t / 30.0f));
  }
  bool open = t >= (uint32_t) CH_OPEN_MS;
  // box
  rect(c, bx - 34, by - 10, 68, 48, 0x2B2233);
  rect(c, bx - 31, by - 7, 62, 42, 0xF0454B);
  rect(c, bx - 31, by + 22, 62, 13, 0xC2333A);
  rect(c, bx - 6, by - 7, 12, 42, 0xFFD23F);
  if (!open) {  // lid and bow
    rect(c, bx - 38, by - 24, 76, 16, 0x2B2233);
    rect(c, bx - 35, by - 21, 70, 10, 0xFF6B70);
    rect(c, bx - 6, by - 21, 12, 10, 0xFFD23F);
    circle(c, bx - 10, by - 30, 8, 0xFFD23F);
    circle(c, bx + 10, by - 30, 8, 0xFFD23F);
    circle(c, bx, by - 27, 4, 0xE0A800);
  } else {
    uint32_t to = t - CH_OPEN_MS;
    if (to < 500) {  // the lid flies off
      int ly = by - 24 - (int) (to * 0.25f), lx = bx + (int) (to * 0.12f);
      rect(c, lx - 38, ly, 76, 16, 0x2B2233);
      rect(c, lx - 35, ly + 3, 70, 10, 0xFF6B70);
    }
    if (to < 700) {  // sparkles
      for (int i = 0; i < 10; i++) {
        float a = i * 0.628f, d = 10 + to * 0.12f;
        static const uint32_t SPARK[3] = {0xFFD23F, 0xFFFFFF, 0x7CC8FF};
        circle(c, bx + (int) (cosf(a) * d), by - 20 + (int) (sinf(a) * d * 0.8f), 3, SPARK[i % 3]);
      }
    }
    int rise = to < 300 ? (int) (to * 0.1f) : 30;
    int iy = by - 24 - rise;
    if (ch_gift_item_ < 0) {  // the medal: ribbon, gold disc, the table's number
      rect(c, bx - 14, iy - 30, 10, 22, 0x4AA8FF);
      rect(c, bx + 4, iy - 30, 10, 22, 0xF0454B);
      circle(c, bx, iy, 24, 0x2B2233);
      circle(c, bx, iy, 22, 0xFFD23F);
      circle(c, bx, iy, 17, 0xFFE680);
      snprintf(buf, sizeof(buf), "%d", n);
      text(c, bx, iy - 10, buf, 3, 0x2B2233, true, false);
    } else {
      const ShopItem &it = SHOP[ch_gift_item_];
      icon(c, it.icon, it.rows, bx - 24, iy - 24, 4);
    }
  }
  if (t >= (uint32_t) CH_OPEN_MS + 200) {
    if (ch_gift_item_ < 0)
      snprintf(buf, sizeof(buf), "Medal for the %ds!", n);
    else
      snprintf(buf, sizeof(buf), "Gift: %s!", SHOP[ch_gift_item_].name);
    text(c, 180, 202, buf, 2, 0xFFFFFF);
    snprintf(buf, sizeof(buf), "+%d coins", ch_gift_coins_);
    text(c, 180, 224, buf, 2, 0xFFD23F);
    if (ch_champion_) text(c, 180, 248, "ALL 12: TIMES TABLE CHAMPION!", 1, 0x7CFF8A, true, false);
  }
  if (t >= (uint32_t) CH_BUTTONS_MS) {
    static const char *const LABELS[3] = {"AGAIN", "NEXT", "DONE"};
    static const uint32_t BTN_COL[3] = {0x4AA8FF, 0x5BD46B, 0x8A80B0};
    for (int b = 0; b < 3; b++) {
      int x = CH_BTN_X[b];
      rect(c, x - CH_BTN_W / 2 - 2, CH_BTN_Y - CH_BTN_H / 2 - 2, CH_BTN_W + 4, CH_BTN_H + 4, 0x1A1426);
      rect(c, x - CH_BTN_W / 2, CH_BTN_Y - CH_BTN_H / 2, CH_BTN_W, CH_BTN_H, BTN_COL[b]);
      text(c, x, CH_BTN_Y - 7, LABELS[b], 2, 0xFFFFFF, true, true);
    }
  }
}

// --- Settings page: sound, brightness, and a held New egg button. ---

static const int SND_X = 180, SND_Y = 112, MINUS_X = 96, PLUS_X = 264, BRIGHT_Y = 186;

void PetEngine::settings_tap(int x, int y) {
  if (abs(x - SND_X) < 62 && abs(y - SND_Y) < 26) {
    set_sound_enabled(!sound_enabled());
    if (sound_enabled()) sound(SND_HAPPY, true);  // a sample
    return;
  }
  if (sq(x - MINUS_X) + sq(y - BRIGHT_Y) < 32 * 32) {
    set_brightness_level(brightness_level() - 1);
    return;
  }
  if (sq(x - PLUS_X) + sq(y - BRIGHT_Y) < 32 * 32) {
    set_brightness_level(brightness_level() + 1);
    return;
  }
  if (abs(x - EGG_BTN_X) < EGG_BTN_W / 2 && abs(y - EGG_BTN_Y) < EGG_BTN_H / 2) show_toast("Hold to confirm", 1500);
}

void PetEngine::draw_settings(Canvas &c, uint32_t now) {
  rect(c, 0, 0, SCREEN, SCREEN, 0x1B1530);
  auto pill = [&](int cx, int cy, int w, int h, uint32_t col) {
    rect(c, cx - w / 2 + h / 2, cy - h / 2, w - h, h, col);
    circle(c, cx - w / 2 + h / 2, cy, h / 2, col);
    circle(c, cx + w / 2 - h / 2, cy, h / 2, col);
  };
  text(c, 180, 30, "SETTINGS", 3, 0xFFFFFF);

  // Sound toggle
  text(c, 180, 72, "Sound", 2, 0xC9C2E8);
  bool on = sound_enabled();
  pill(SND_X, SND_Y, 112, 38, 0x2B2233);
  pill(SND_X, SND_Y, 106, 32, on ? 0x5BD46B : 0x6A6480);
  circle(c, on ? SND_X + 37 : SND_X - 37, SND_Y, 13, 0xFFFFFF);
  text(c, on ? SND_X - 14 : SND_X + 14, SND_Y - 7, on ? "ON" : "OFF", 2, 0xFFFFFF, true, false);

  // Brightness: - [####] +
  text(c, 180, 148, "Brightness", 2, 0xC9C2E8);
  for (int side = 0; side < 2; side++) {
    int bx = side ? PLUS_X : MINUS_X;
    circle(c, bx, BRIGHT_Y, 23, 0x2B2233);
    circle(c, bx, BRIGHT_Y, 20, 0xFFF6E0);
    text(c, bx, BRIGHT_Y - 10, side ? "+" : "-", 3, 0x2B2233, true, false);
  }
  int level = brightness_level();
  for (int i = 0; i < 4; i++) rect(c, 128 + i * 27, BRIGHT_Y - 13, 22, 26, i <= level ? 0xFFD23F : 0x3D3350);

  // New egg: hold for 2 s; the bar fills while held.
  float p = egg_hold_progress(now);
  pill(EGG_BTN_X, EGG_BTN_Y, EGG_BTN_W + 6, EGG_BTN_H + 6, 0x2B2233);
  pill(EGG_BTN_X, EGG_BTN_Y, EGG_BTN_W, EGG_BTN_H, 0x8E2A3A);
  if (p > 0) {
    int fill = (int) (EGG_BTN_W * p);
    rect(c, EGG_BTN_X - EGG_BTN_W / 2, EGG_BTN_Y - EGG_BTN_H / 2, fill, EGG_BTN_H, 0xF0454B);
  }
  text(c, EGG_BTN_X, EGG_BTN_Y - 7, p > 0 ? "Keep holding" : "Hold: new egg", 2, 0xFFFFFF, true, false);
  char buf[40];
  if (s_.dead)
    snprintf(buf, sizeof(buf), "a fresh start");
  else
    snprintf(buf, sizeof(buf), "replaces %s", name_);
  text(c, 180, 304, buf, 2, 0x9A90B0, true, false);
}

void PetEngine::draw_stats(Canvas &c) {
  rect(c, 0, 0, SCREEN, SCREEN, 0x1B1530);
  char buf[40];
  text(c, 180, 26, name_, 4, BODY_COLORS[s_.g.color % 12]);
  text(c, 180, 60, species_, 2, 0xC9C2E8);
  // Stage and how much more time together until the next one.
  static const char *const NEXT[6] = {"hatches", "child", "teen", "adult", "elder", "egg"};
  int mins = grows_in_min();
  char when[16];
  if (mins < 0)
    when[0] = 0;
  else if (mins < 60)
    snprintf(when, sizeof(when), "%dm", mins);
  else
    snprintf(when, sizeof(when), "%dh%02dm", mins / 60, mins % 60);
  if (s_.dead)
    snprintf(buf, sizeof(buf), "%s", stage_name());
  else if (s_.stage == STAGE_EGG)
    snprintf(buf, sizeof(buf), "Egg, hatches in %s", when);
  else
    snprintf(buf, sizeof(buf), "%s, %s in %s", stage_name(), NEXT[s_.stage > 5 ? 5 : s_.stage], when);
  text(c, 180, 80, buf, 2, 0xFFFFFF);
  // food, happiness, energy, cleanliness, math, health, discipline, smarts
  static const char *const *ICONS[8] = {art::MINI_FOOD, art::MINI_HEART, art::MINI_BOLT, art::MINI_DROP,
                                       art::MINI_MATH, art::MINI_CROSS, art::MINI_DISC, art::MINI_BOOK};
  int vals[8] = {s_.food, s_.happy, s_.energy, s_.clean, math(), s_.health, s_.discipline, smarts()};
  for (int i = 0; i < 8; i++) {
    int y = 101 + i * 19;
    icon(c, ICONS[i], 7, 84, y - 2, 3);
    bar(c, 116, y + 2, 160, 12, vals[i]);
  }
  snprintf(buf, sizeof(buf), "%ug  Gen %u  Oops %u", (unsigned) s_.weight, (unsigned) s_.generation,
           (unsigned) s_.care_mistakes);
  text(c, 180, 252, buf, 2, 0xC9C2E8);
  snprintf(buf, sizeof(buf), "Feeling %s", mood());
  text(c, 180, 272, buf, 2, 0xFFF6C8);
  snprintf(buf, sizeof(buf), "Coins %u", (unsigned) s_.coins);
  text(c, 180, 292, buf, 2, 0xFFD23F);
  if (parent_[0]) {
    snprintf(buf, sizeof(buf), "Child of %s", parent_);
    text(c, 180, 312, buf, 2, 0xFFB3D1);
  }
}

}  // namespace petcore
