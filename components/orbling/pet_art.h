// Pixel art and the 5x7 font for Orbling. Icons are rows of characters, one per pixel:
// '.' clear, K outline, W white, R red, G green, B brown, Y yellow, O orange, P pink,
// L blue, S grey, D dark brown, C cream.
#pragma once

#include <cstdint>

namespace petcore {
namespace art {

inline uint32_t icon_color(char ch) {
  switch (ch) {
    case 'K': return 0x2B2233;
    case 'W': return 0xFFFFFF;
    case 'R': return 0xF0454B;
    case 'G': return 0x4CC25A;
    case 'B': return 0x8A5A35;
    case 'D': return 0x5C3A22;
    case 'Y': return 0xFFD23F;
    case 'O': return 0xFF9A3C;
    case 'P': return 0xFF7EB0;
    case 'L': return 0x4AA8FF;
    case 'S': return 0xA0A4B8;
    case 'C': return 0xFFF1D0;
    default: return 0xFF00FF;
  }
}

// --- 12x12 button icons ---
static const char *const FOOD[12] = {
    "......K.....", ".....KGG....", "..KKKKKKKK..", ".KRRRRRRWRK.", "KRRRRRRRRWRK", "KRRRRRRRRRRK",
    "KRRRRRRRRRRK", "KRRRRRRRRRRK", ".KRRRRRRRRK.", ".KRRRRRRRRK.", "..KRRKKRRK..", "...KK..KK...",
};
static const char *const PLAY[12] = {
    "....KKKK....", "..KKYYYYKK..", ".KYYYYYYYYK.", ".KRRRRRRRRK.", "KRRRRRRRRRRK", "KWWWWWWWWWWK",
    "KWWWWWWWWWWK", "KLLLLLLLLLLK", ".KLLLLLLLLK.", ".KYYYYYYYYK.", "..KKYYYYKK..", "....KKKK....",
};
static const char *const CLEAN[12] = {
    ".....KK.....", "....KLLK....", "....KLLK....", "...KLLLLK...", "..KLWLLLLK..", "..KLWLLLLK..",
    ".KLWLLLLLLK.", ".KLLLLLLLLK.", ".KLLLLLLLLK.", "..KLLLLLLK..", "...KKKKKK...", "............",
};
static const char *const BULB[12] = {
    "....KKKK....", "..KKYYYYKK..", ".KYYWWYYYYK.", ".KYWYYYYYYK.", ".KYYYYYYYYK.", ".KYYYYYYYYK.",
    "..KYYYYYYK..", "...KYYYYK...", "....KSSK....", "....KSSK....", "....KSSK....", ".....KK.....",
};
static const char *const MOON[12] = {
    "....KKKK....", "..KKYYYK....", ".KYYYK......", ".KYYK.......", "KYYK........", "KYYK........",
    "KYYK........", "KYYYK.......", ".KYYYK....K.", ".KYYYYKKKYK.", "..KKYYYYYK..", "....KKKKK...",
};
static const char *const HEAL[12] = {
    "..KKKKKKKK..", ".KWWWWWWWWK.", "KWWWWRRWWWWK", "KWWWWRRWWWWK", "KWWWWRRWWWWK", "KWRRRRRRRRWK",
    "KWRRRRRRRRWK", "KWWWWRRWWWWK", "KWWWWRRWWWWK", "KWWWWRRWWWWK", ".KWWWWWWWWK.", "..KKKKKKKK..",
};

static const char *const SCOLD[12] = {
    ".....KK.....", "....KYYK....", "....KYYK....", "...KYKKYK...", "...KYKKYK...", "..KYYKKYYK..",
    "..KYYKKYYK..", ".KYYYKKYYYK.", ".KYYYYYYYYK.", "KYYYYKKYYYYK", "KYYYYYYYYYYK", "KKKKKKKKKKKK",
};

// --- food the pet eats ---
static const char *const MEAL[12] = {
    ".....KK.....", "....KWWK....", "...KWWWWK...", "..KWWWWWWK..", "..KWWWWWWK..", ".KWWWWWWWWK.",
    ".KWWKKKKWWK.", "KWWKGGGGKWWK", "KWWKGGGGKWWK", "KWWKGGGGKWWK", ".KKKKKKKKKK.", "............",
};
static const char *const SNACK[12] = {
    "....KKKK....", "...KPPWPK...", "..KPWPPPWK..", "..KPPPWPPK..", "..KWPPPPPK..", "...KPPWPK...",
    "....KKKK....", ".....KK.....", ".....KC.....", ".....KC.....", ".....KC.....", "......K.....",
};

// --- 7x7 gauge icons ---
static const char *const MINI_FOOD[7] = {"...KG..", ".KKKKK.", "KRRRWRK", "KRRRRRK", "KRRRRRK", ".KRRRK.", "..K.K.."};
static const char *const MINI_HEART[7] = {".KK.KK.", "KPPKPPK", "KPPPPPK", "KPPPPPK", ".KPPPK.", "..KPK..", "...K..."};
static const char *const MINI_BOLT[7] = {"...KKK.", "..KYYK.", ".KYYK..", "KYYYYYK", "..KYYK.", "..KYK..", "..KK..."};
static const char *const MINI_DROP[7] = {"...K...", "..KLK..", ".KLLLK.", "KLLLLLK", "KLWLLLK", ".KLLLK.", "..KKK.."};
static const char *const MINI_DISC[7] = {"...K...", "..KYK..", ".KYRYK.", ".KYRYK.", "KYYYYYK", "KYYRYYK", "KKKKKKK"};
static const char *const MINI_CROSS[7] = {"..KKK..", "..KRK..", "KKKRKKK", "KRRRRRK", "KKKRKKK", "..KRK..", "..KKK.."};

// --- scene bits ---
static const char *const POOP[10] = {
    "....KK....", "...KBBK...", "...KBDK...", "..KBBBBK..", ".KBBDBBBK.", ".KBBBBBBK.",
    "KBBBBBDBBK", "KBDBBBBBBK", ".KKKKKKKK.", "..........",
};
static const char *const SKULL[7] = {".KKKKK.", "KWWWWWK", "KWKWKWK", "KWWWWWK", ".KWWWK.", ".KWKWK.", "..KKK.."};
static const char *const TOMB[12] = {
    "...KKKKKK...", "..KSSSSSSK..", ".KSSSSSSSSK.", ".KSSSKSSSSK.", ".KSSKKKSSSK.", ".KSSSKSSSSK.",
    ".KSSSKSSSSK.", ".KSSSSSSSSK.", ".KSSSSSSSSK.", ".KSSSSSSSSK.", "KKKKKKKKKKKK", "KGGGGGGGGGGK",
};

// --- jump game ---
static const char *const ROCK[8] = {
    "....KKKK....", "..KKSSSSKK..", ".KSSWSSSSSK.", ".KSWSSSSSSK.",
    "KSSSSSSSSSSK", "KSSSSSSSKSSK", "KSSSSSSSSSSK", "KKKKKKKKKKKK",
};
static const char *const MUSHROOM[10] = {
    "..KKKKKK..", ".KRRWRRRK.", "KRWWRRRWRK", "KRRRRRWWRK", "KKKKKKKKKK",
    "...KCCK...", "...KCCK...", "...KCCK...", "..KCCCCK..", "..KKKKKK..",
};
static const char *const BUSH[10] = {
    "...KK....KK...", "..KGGK..KGGK..", ".KGGGGKKGGGGK.", "KGGGGGGGGGGGGK", "KGGWGGGGGGWGGK",
    "KGGGGGGGGGGGGK", "KGGGGGRGGGGGGK", ".KGGGGGGGGGGK.", ".KGGGGGGGGGGK.", "..KKKKKKKKKK..",
};
static const char *const BIRD_UP[8] = {
    "..K.....K...", ".KWK...KWK..", "KWWWK.KWWWK.", "..KWWKWWK...",
    "...KLLLLKOO.", "...KLLLLLK..", "....KKKKK...", "............",
};
static const char *const BIRD_DOWN[8] = {
    "............", "............", "...KKKKK....", "..KLLLLLKOO.",
    ".KWWLLLLK...", "KWWWKKKK....", ".KWK........", "..K.........",
};
static const char *const STAR[9] = {
    "....K....", "...KYK...", "KKKKYKKKK", "KYYYYYYYK", ".KYYYYYK.",
    "..KYYYK..", ".KYYKYYK.", ".KYK.KYK.", ".KK...KK.",
};

// --- game menu ---
static const char *const BUBBLE[12] = {
    "...KKKKKK...", "..KLLLLLLK..", ".KLWWLLLLLK.", "KLWWLLLLLLLK", "KLWLLLLLLLLK", "KLLLLLLLLLLK",
    "KLLLLLLLLLLK", "KLLLLLLLLLLK", "KLLLLLLLLLLK", ".KLLLLLLLLK.", "..KLLLLLLK..", "...KKKKKK...",
};
static const char *const PADS[12] = {
    "KKKKKKKKKKKK", "KRRRRKKLLLLK", "KRRRRKKLLLLK", "KRRRRKKLLLLK", "KRRRRKKLLLLK", "KKKKKKKKKKKK",
    "KKKKKKKKKKKK", "KYYYYKKGGGGK", "KYYYYKKGGGGK", "KYYYYKKGGGGK", "KYYYYKKGGGGK", "KKKKKKKKKKKK",
};

static const char *const TIMES[12] = {
    "KKKKKKKKKKKK", "KGGGGGGGGGGK", "KGWGGGGGGWGK", "KGGWGGGGWGGK", "KGGGWGGWGGGK", "KGGGGWWGGGGK",
    "KGGGGWWGGGGK", "KGGGWGGWGGGK", "KGGWGGGGWGGK", "KGWGGGGGGWGK", "KGGGGGGGGGGK", "KKKKKKKKKKKK",
};
static const char *const KEYPAD[12] = {
    "KKKKKKKKKKKK", "KLLLLLLLLLLK", "KKKKKKKKKKKK", "KWWWKWWWKWWK", "KWWWKWWWKWWK", "KKKKKKKKKKKK",
    "KWWWKWWWKWWK", "KWWWKWWWKWWK", "KKKKKKKKKKKK", "KWWWKWWWKGGK", "KWWWKWWWKGGK", "KKKKKKKKKKKK",
};
static const char *const MIC[12] = {
    "....KKKK....", "...KLLLLK...", "...KLWLLK...", "...KLLLLK...", "...KLLLLK...", "...KLLLLK...",
    ".K.KLLLLK.K.", ".K..KKKK..K.", "..K......K..", "...KKKKKK...", ".....KK.....", "...KKKKKK...",
};

// --- math prizes and smarts ---
static const char *const MINI_MATH[7] = {"KKKKKKK", "KLLLLLK", "KLWLWLK", "KLLWLLK", "KLWLWLK", "KLLLLLK", "KKKKKKK"};
static const char *const MINI_BOOK[7] = {"KKKKKKK", "KLLKWWK", "KLLKWWK", "KLLKWWK", "KLLKWWK", "KKKKKKK", "......."};
static const char *const WIZARD_HAT[12] = {
    ".....KK.....", "....KLLK....", "....KLYK....", "...KLLLLK...", "...KLLYLK...", "..KLLLLLLK..",
    "..KYLLLLLK..", ".KLLLLLYLLK.", ".KLLLLLLLLK.", "KKKKKKKKKKKK", "KLLLLLLLLLLK", "KKKKKKKKKKKK",
};
static const char *const TROPHY[12] = {
    "..KKKKKKKK..", "KKYYYYYYYYKK", "KYKYWYYYYKYK", "KYKYWYYYYKYK", ".KKYYYYYYKK.", "...KYYYYK...",
    "....KYYK....", ".....KK.....", "....KYYK....", "...KKKKKK...", "..KDDDDDDK..", "..KKKKKKKK..",
};
static const char *const ROCKET[12] = {
    ".....KK.....", "....KRRK....", "...KWWWWK...", "...KWLLWK...", "...KWLLWK...", "...KWWWWK...",
    "...KWWWWK...", "..KRKWWKRK..", ".KRRKWWKRRK.", ".KRKKKKKKRK.", "....KOOK....", ".....YY.....",
};
static const char *const OWL[12] = {
    "..K......K..", ".KBK....KBK.", ".KBBKKKKBBK.", "KBBBBBBBBBBK", "KBWKWBBWKWBK", "KBWWWBBWWWBK",
    "KBBBBOOBBBBK", "KBBCCBBCCBBK", ".KBCCCCCCBK.", "..KBBBBBBK..", "...KOKKOK...", "............",
};
static const char *const CHEF_HAT[12] = {
    "............", "..KKK.KKK...", ".KWWWKWWWK..", "KWWWWWWWWWK.", "KWWWWWWWWWK.", ".KWWWWWWWK..",
    "..KWWWWWK...", "..KWWWWWK...", "..KSSSSSK...", "..KKKKKKK...", "............", "............",
};
static const char *const GOLD_APPLE[12] = {
    "......K.....", ".....KGG....", "..KKKKKKKK..", ".KYYYYYYWYK.", "KYYYYYYYYWYK", "KYYYYYYYYYYK",
    "KYYYYYYYYYYK", "KYYYYYYYYYYK", ".KYYYYYYYYK.", ".KYYYYYYYYK.", "..KYYKKYYK..", "...KK..KK...",
};
// --- prize stand (sold only after a math game) ---
static const char *const PROP_CAP[12] = {
    "............", "...YYYYYY...", "......K.....", "......K.....", "...KKKKKK...", "..KRRLLRRK..",
    ".KRRRLLRRRK.", ".KRRRLLRRRK.", "KKKKKKKKKKKK", "............", "............", "............",
};
static const char *const STAR_HAT[12] = {
    ".....KK.....", "....KYYK....", "....KYYK....", "KKKKKYYKKKKK", "KYYYYYYYYYYK", ".KYYYYWYYYK.",
    "..KYYYYYYK..", "..KYYKKYYK..", ".KYYK..KYYK.", ".KKK....KKK.", "............", "............",
};
static const char *const ABACUS[12] = {
    "............", "KKKKKKKKKKKK", "KBRRSSSSSSBK", "KBSSLLSSSSBK", "KBSSSSYYSSBK", "KBGGSSSSSSBK",
    "KBSSSRRSSSBK", "KKKKKKKKKKKK", "K..........K", "K..........K", "............", "............",
};
static const char *const GLOBE[12] = {
    "....KKKK....", "..KKLLLLKK..", ".KLLGGLLLLK.", ".KLGGGGLLLK.", "KLLLGGLLGGLK", "KLLLLLLLGGLK",
    ".KLLLGLLLLK.", ".KLLGGLLLLK.", "..KKLLLLKK..", "....KKKK....", ".....KK.....", "...KKKKKK...",
};
static const char *const BOOKS[12] = {
    "............", "..KKKKKKKKK.", "..KRRRRRRRK.", "..KKKKKKKKK.", ".KKKKKKKKKK.", ".KLLLLLLLLK.",
    ".KKKKKKKKKK.", "KKKKKKKKKK..", "KGGGGGGGGK..", "KCCCCCCCCK..", "KKKKKKKKKK..", "............",
};
static const char *const BRAIN_SNACK[12] = {
    "............", "............", "...LL.LL....", "..LLLLLLL...", ".LLWLLLLLL..", "KKKKKKKKKKK.",
    "KWWWWWWWWWK.", ".KWWWWWWWK..", "..KKKKKKK...", "............", "............", "............",
};
// --- prize stand pages 2-5 (Facts: gadgets, Fizz: ocean, Munch: bakery, Share: space) ---
// Hats drawn over the pet sit on its head by their lowest row.
static const char *const HEADPHONES[12] = {
    "............", "...KKKKKK...", "..K......K..", ".K........K.", ".K........K.", "KK........KK",
    "KLK......KLK", "KLK......KLK", "KLK......KLK", "KKK......KKK", "............", "............",
};
static const char *const GOGGLES[12] = {
    "............", "............", "............", "KKKKKKKKKKKK", "KLLLKKKKLLLK", "KLWLKKKKLWLK",
    "KLLLK..KLLLK", "KKKKK..KKKKK", "............", "............", "............", "............",
};
static const char *const ROBOT[12] = {
    ".....KK.....", ".....YY.....", "..KKKKKKKK..", "..KSSSSSSK..", "..KSLSSLSK..", "..KSSSSSSK..",
    "..KSRRRRSK..", "..KKKKKKKK..", ".KKSSSSSSKK.", ".KSKSSSSKSK.", "...KSKKSK...", "...KK..KK...",
};
static const char *const CLOCK[12] = {
    "...KKKKKK...", "..KWWWWWWK..", ".KWWWWKWWWK.", ".KWWWWKWWWK.", ".KWWWWKWWWK.", ".KWWWWKKKWK.",
    ".KWWWWWWWWK.", ".KWWWWWWWWK.", "..KWWWWWWK..", "...KKKKKK...", "..KK....KK..", "............",
};
static const char *const ENERGY_BAR[12] = {
    "............", "............", "............", "KKKKKKKKKKKK", "KOOOOOOOOOOK", "KOYYOOOYYOOK",
    "KOOOOOOOOOOK", "KKKKKKKKKKKK", "............", "............", "............", "............",
};
static const char *const MILK[12] = {
    "....KKKK....", "....KWWK....", "...KWWWWK...", "..KWWWWWWK..", "..KLLLLLLK..", "..KLWWWWLK..",
    "..KLWLLWLK..", "..KLWWWWLK..", "..KLLLLLLK..", "..KWWWWWWK..", "..KKKKKKKK..", "............",
};
static const char *const SNORKEL[12] = {
    "..........K.", "..........LK", "..........LK", "KKKKKKKKKKLK", "KLLLLLLLLKLK", "KLWLLLLLLKKK",
    "KLLLLLLLLK..", "KKKKKKKKKK..", "............", "............", "............", "............",
};
static const char *const SAILOR_HAT[12] = {
    "............", "............", "....KKKK....", "..KKWWWWKK..", ".KWWWWWWWWK.", "KWWWWWWWWWWK",
    "KLLLLLLLLLLK", "KKKKKKKKKKKK", "............", "............", "............", "............",
};
static const char *const FISHBOWL[12] = {
    "............", "...KKKKKK...", "..KLLLLLLK..", ".KLLLLLLLLK.", "KLLLOOLLLLLK", "KLLOOOOLLLLK",
    "KLLLOOLLLLLK", "KLLLLLLLGLLK", ".KLLLLLLGLK.", "..KKKKKKKK..", "............", "............",
};
static const char *const SHELL[12] = {
    "............", "....KKKK....", "...KPCPCK...", "..KCPCPCPK..", ".KPCPCPCPCK.", ".KCPCPCPCPK.",
    "KKPCPCPCPCKK", "KCKKKKKKKKCK", ".KK......KK.", "............", "............", "............",
};
static const char *const SODA[12] = {
    ".....K......", "....K.......", "...KK.......", "..KKKKKK....", "..KRRRRK....", "..KRWRRK....",
    "..KRRRRK....", "..KWWWWK....", "..KRRRRK....", "..KRRRRK....", "..KKKKKK....", "............",
};
static const char *const SUSHI[12] = {
    "............", "............", "....KKKK....", "..KKOOOOKK..", ".KOOWOOOOOK.", "KWWWWWWWWWWK",
    "KWWWWWWWWWWK", "KWWWWWWWWWWK", ".KKKKKKKKKK.", "............", "............", "............",
};
static const char *const BERET[12] = {
    "............", "............", "......K.....", "...KKKKKK...", "..KRRRRRRK..", ".KRRRRRRRRK.",
    "KRRRRRRRRRRK", "KKKKKKKKKKKK", "............", "............", "............", "............",
};
static const char *const FRUIT_HAT[12] = {
    "...G...G....", "..KRK.KYK...", "..KRKKKYK...", ".KKKKKKKKK..", ".KOOOOOOOK..", "KOOOOOOOOOK.",
    "KKKKKKKKKKK.", "............", "............", "............", "............", "............",
};
static const char *const COOKIE_JAR[12] = {
    "....KKKK....", "...KSSSSK...", "..KKKKKKKK..", "..KWWWWWWK..", "..KWBWWBWK..", "..KWWBWWWK..",
    "..KWBWWWBK..", "..KWWWWWWK..", "..KKKKKKKK..", "............", "............", "............",
};
static const char *const TEAPOT[12] = {
    ".....KK.....", "....KWWK....", "..KKKKKKKK..", ".KWWWWWWWWK.", "KKWWPWWPWWKK", "K.KWWWWWWK.K",
    "K.KWWWWWWKK.", ".KKWWWWWWK..", "...KKKKKK...", "............", "............", "............",
};
static const char *const DONUT[12] = {
    "............", "...KKKKKK...", "..KPPPPPPK..", ".KPPYPPRPPK.", ".KPPKKKKPPK.", ".KPRK..KPPK.",
    ".KBBKKKKBBK.", "..KBBBBBBK..", "...KKKKKK...", "............", "............", "............",
};
static const char *const PIZZA[12] = {
    "KKKKKKKKKKKK", "KOOOOOOOOOOK", ".KYYRYYYYRK.", ".KYYYYRYYK..", "..KYRYYYK...", "..KYYYYK....",
    "...KYRK.....", "...KYK......", "....K.......", "............", "............", "............",
};
static const char *const SPACE_HELMET[12] = {
    "....KKKK....", "..KKWWWWKK..", ".KWLLLLLLWK.", "KWLLWLLLLLWK", "KWLLLLLLLLWK", "KWLLLLLLLLWK",
    "KSSSSSSSSSSK", "KKKKKKKKKKKK", "............", "............", "............", "............",
};
static const char *const ALIEN_BAND[12] = {
    "..G......G..", ".GGG....GGG.", "..K......K..", "...K....K...", "....K..K....", "KKKKKKKKKKKK",
    "KGGGGGGGGGGK", "KKKKKKKKKKKK", "............", "............", "............", "............",
};
static const char *const TELESCOPE[12] = {
    "............", "..........KK", "........KKLK", "......KKLLK.", "....KKLLLK..", "..KKLLLKK...",
    "KKLLLKK.....", "KKKKK.......", "....KK......", "...K..K.....", "..K....K....", ".K......K...",
};
static const char *const PLANET[12] = {
    "............", "............", "....KKKK....", "..KKOOOOKK..", ".KOOYOOOOOK.", "YYYYYYYYYYYY",
    ".KOOOOOOOOK.", "..KKOOOOKK..", "....KKKK....", "............", "............", "............",
};
static const char *const STAR_COOKIE[12] = {
    ".....KK.....", "....KBBK....", "KKKKKBBKKKKK", "KBBBBBBYBBBK", ".KBBYBBBBBK.", "..KBBBBBBK..",
    "..KBBKKBBK..", ".KBBK..KBBK.", ".KKK....KKK.", "............", "............", "............",
};
static const char *const ASTRO[12] = {
    "............", "..KKKKKKKK..", "..KSSSSSSK..", "..KSPPWWSK..", "..KSPPWWSK..", "..KSWWBBSK..",
    "..KSWWBBSK..", "..KSSSSSSK..", "..KKKKKKKK..", "............", "............", "............",
};

static const char *const DIVIDE[12] = {
    "KKKKKKKKKKKK", "KLLLLLLLLLLK", "KLLLLWWLLLLK", "KLLLLWWLLLLK", "KLLLLLLLLLLK", "KLWWWWWWWWLK",
    "KLWWWWWWWWLK", "KLLLLLLLLLLK", "KLLLLWWLLLLK", "KLLLLWWLLLLK", "KLLLLLLLLLLK", "KKKKKKKKKKKK",
};
// game menu: Fizz (a numbered bubble) and Munch (food with a fact on it)
static const char *const FIZZ_ICON[12] = {
    "...KKKKKK...", "..KLLLLLLK..", ".KLWLLLLLLK.", "KLWLLKKLLLLK", "KLLLKLLKLLLK", "KLLLLLKLLLLK",
    "KLLLLKLLLLLK", "KLLLKKKKLLLK", "KLLLLLLLLLLK", ".KLLLLLLLLK.", "..KLLLLLLK..", "...KKKKKK...",
};
static const char *const MUNCH_ICON[12] = {
    "............", "..KKKKKKKK..", ".KOOOOOOOOK.", "KOOKOOOKOOOK", "KOOOKOKOOOOK", "KOOOOKOOOOOK",
    "KOOOKOKOOOOK", "KOOKOOOKOOOK", ".KOOOOOOOOK.", "..KKKKKKKK..", "............", "............",
};

// --- shop ---
static const char *const COIN[7] = {"..KKK..", ".KYYYK.", "KYYWYYK", "KYWYYYK", "KYYYYYK", ".KYYYK.", "..KKK.."};
static const char *const CAKE[12] = {
    "....R..R....", "....K..K....", "..KKKKKKKK..", ".KWWWWWWWWK.", ".KPPWPPWPPK.", "KPPPPPPPPPPK",
    "KWWWWWWWWWWK", "KPPPPPPPPPPK", "KPPPPPPPPPPK", "KWWWWWWWWWWK", ".KKKKKKKKKK.", "............",
};
static const char *const STEAK[12] = {
    "........KK..", ".......KWWK.", "..KKKKKKWWK.", ".KRRRRRKKK..", "KRRWRRRRK...", "KRRRRRRRK...",
    "KRRRRRWRK...", "KRRRRRRRK...", ".KRRRRRK....", "..KKKKKK....", "............", "............",
};
static const char *const JUICE[12] = {
    ".......KK...", "......KK....", "..KKKKKKKK..", "..KOOOOOOK..", "..KOWOOOOK..", "..KOWOOOOK..",
    "..KOOOOOOK..", "..KOOOOOOK..", "...KOOOOK...", "...KOOOOK...", "...KKKKKK...", "............",
};
static const char *const TEDDY[12] = {
    ".KK......KK.", "KBBK....KBBK", "KBBBKKKKBBBK", ".KBBBBBBBBK.", ".KBKBBBBKBK.", ".KBBBBBBBBK.",
    ".KBBBDDBBBK.", "..KBBBBBBK..", ".KBBKKKKBBK.", "KBBBBBBBBBBK", "KBBK....KBBK", ".KK......KK.",
};
static const char *const PLANT[12] = {
    "....GG......", "...GGGG.GG..", "..GGGGGGGGG.", ".GGGGGGGGG..", "..GGGGGGG...", "....GGG.....",
    ".....G......", "..KKKKKKKK..", "..KOOOOOOK..", "...KOOOOK...", "...KOOOOK...", "...KKKKKK...",
};
static const char *const BED[12] = {
    "............", "............", "............", "K...........", "KWWK........", "KWWKLLLLLLLK",
    "KKKKKKKKKKKK", "KBBBBBBBBBBK", "KBBBBBBBBBBK", "K..........K", "K..........K", "............",
};
static const char *const BOW[12] = {
    "............", "............", ".KK.....KK..", "KRRK...KRRK.", "KRRRK.KRRRK.", "KRRRRKRRRRK.",
    "KRRRKKKRRRK.", "KRRRK.KRRRK.", "KRRK...KRRK.", ".KK.....KK..", "............", "............",
};
static const char *const CAP[12] = {
    "............", "............", "....KKKK....", "..KKLLLLKK..", ".KLLLLLLLLK.", ".KLWLLLLLLK.",
    ".KLLLLLLLLK.", "KKKKKKKKKKKK", "......KLLLLK", "......KKKKKK", "............", "............",
};
static const char *const GLASSES[12] = {
    "............", "............", "............", "KKKKK..KKKKK", "KLLLK..KLLLK", "KLWLKKKKLWLK",
    "KLLLK..KLLLK", "KKKKK..KKKKK", "............", "............", "............", "............",
};
static const char *const PARTY_HAT[12] = {
    ".....Y......", "....YKY.....", ".....K......", "....KRK.....", "....KRK.....", "...KYYYK....",
    "...KRRRK....", "..KYYYYYK...", "..KRRRRRK...", ".KYYYYYYYK..", ".KRRRRRRRK..", "KKKKKKKKKKK.",
};

// Classic 5x7 font, columns LSB at top, ASCII 32..126. The backtick ` is a division sign instead.
static const uint8_t FONT5X7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00}, {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7F, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40}, {0x08, 0x08, 0x2A, 0x08, 0x08} /* ` is drawn as a division sign */, {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x0C, 0x52, 0x52, 0x52, 0x3E},
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3D, 0x00},
    {0x7F, 0x10, 0x28, 0x44, 0x00}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0x7C, 0x14, 0x14, 0x14, 0x08},
    {0x08, 0x14, 0x14, 0x18, 0x7C}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x7F, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00}, {0x10, 0x08, 0x08, 0x10, 0x08},
};

}  // namespace art
}  // namespace petcore
