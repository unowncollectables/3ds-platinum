/* celebrations=1 (default off; settings app Battle tab "Special date effects"): birthday and holiday particle effects.
 * NEW, port-side and purely visual: the original game has no birthday or holiday events. Particles are drawn over the top
 * screen picture (like dw_debris / the shiny flash), never touch gameplay or the save, and are a pure function of a frame
 * counter, so both 3D eyes agree and each particle gets a per-layer parallax shift.
 *
 * Birthdays (list in the player's own sdmc:/3ds/platinum3ds/config.txt only; never in the repo, docs or code):
 *     birthdays=10-27,03-14,07-02        MM-DD (or MM/DD), up to 32 entries
 *   Always the shiny-style star sparkle, always in the same place: Sprout Tower (violet city, all 3 floors), with New Bark
 *   Town as the fallback place. A birthday beats any holiday that day. The dates are never written to log.txt (count only).
 * Holidays (public dates, in code), place per holiday:
 *   Jan 1 fireworks, Goldenrod City   Feb 14 pink hearts, Cherrygrove City   Mar 17 green sparkle rain, Ilex Forest
 *   Jul 1 red and white sparkles, Olivine City   Oct 31 purple wisps, Lavender Town   Dec 25 snowflakes, Mahogany Town
 *   (Easter / Thanksgiving: not included; the dates move.)
 * Test without waiting for the date: config.txt celeb_test=N forces effect N (1 birthday, 2 fireworks, 3 hearts, 4 green
 * rain, 5 red/white, 6 wisps, 7 snow) on every map in the overworld. Log: [CELEB]. Map ids = pokeheartgold maps.h. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "celebrate.h"
#include "achievements.h"

extern void PSPNativeMemLog(const char *, ...);
extern int P3DS_CfgInt(const char *, int);
extern int P3DS_CfgStr(const char *, char *, int);
extern void P3DS_CfgWrite(const char *, int);
extern int P3DS_InField, P3DS_StereoShiftNow(void);
extern volatile int P3DS_MmMap;

enum { E_NONE, E_BDAY, E_FIREWORKS, E_HEARTS, E_GREEN, E_CANADA, E_WISPS, E_SNOW };
static const char *const kEffName[] = { "none", "birthday sparkle", "fireworks", "hearts", "green sparkle rain", "red/white sparkles", "ghost wisps", "snow" };

static unsigned T; /* frame counter, advanced on the left-eye call */
static int onV = -1;
int P3DS_CelebOn(void) { if (onV < 0) { onV = P3DS_CfgInt("celebrations", 0) != 0; PSPNativeMemLog("[CELEB] celebrations=%d (celeb_test=%d)", onV, P3DS_CfgInt("celeb_test", 0)); } return onV; }
void P3DS_CelebSetOn(int v) { onV = v != 0; P3DS_CfgWrite("celebrations", onV); PSPNativeMemLog("[CELEB] celebrations=%d from settings", onV); }

/* ---- which effect is active here and now ---- */
static int bdayN = -1; static short bdayMD[32]; /* month * 100 + day */
static void bdayLoad(void) {
  char b[512]; bdayN = 0; if (!P3DS_CfgStr("birthdays", b, sizeof b)) return;
  for (const char *p = b; *p && bdayN < 32;) { int m, d, used = 0;
    if (sscanf(p, "%d%*[-/]%d%n", &m, &d, &used) >= 2 && used > 0 && m >= 1 && m <= 12 && d >= 1 && d <= 31) { bdayMD[bdayN++] = (short)(m * 100 + d); p += used; }
    else { while (*p && *p != ',' && *p != ' ') p++; }
    while (*p == ',' || *p == ' ') p++; }
  PSPNativeMemLog("[CELEB] %d birthday entr%s loaded from config.txt", bdayN, bdayN == 1 ? "y" : "ies"); }

/* achievements: the holiday or birthday effect that fires unlocks its achievement (the real effect only, never celeb_test) */
static int achFor(int e) {
  switch (e) { case E_BDAY: return ACH_BDAY; case E_FIREWORKS: return ACH_NEWYEAR; case E_HEARTS: return ACH_VALENTINE; case E_GREEN: return ACH_STPAT;
    case E_CANADA: return ACH_CANADA; case E_WISPS: return ACH_HALLOWEEN; case E_SNOW: return ACH_XMAS; }
  return -1; }

static int effectNow(int *placeOnly) {
  static int cached = E_NONE, cachedMap = -2, cachedDay = -1, lastLogged = -1, testV = -1;
  if (testV < 0) testV = P3DS_CfgInt("celeb_test", 0);
  const int test = testV;
  if (test >= 1 && test <= 7) { *placeOnly = 0; if (lastLogged != test) { lastLogged = test; PSPNativeMemLog("[CELEB] celeb_test=%d forces '%s' on every map", test, kEffName[test]); } return test; }
  if (bdayN < 0) bdayLoad();
  time_t t = time(NULL); struct tm *lt = localtime(&t); if (!lt) return E_NONE;
  const int md = (lt->tm_mon + 1) * 100 + lt->tm_mday, map = P3DS_MmMap;
  if (md == cachedDay && map == cachedMap) return cached;
  cachedDay = md; cachedMap = map; int e = E_NONE;
  for (int i = 0; i < bdayN; i++) if (bdayMD[i] == md) { e = E_BDAY; break; }
  if (e == E_BDAY) { if (!(map == 110 || map == 155 || map == 156 || map == 60)) e = E_NONE; } /* Sprout Tower 1F-3F, fallback New Bark Town */
  else switch (md) { /* birthday on this date -> handled above (personal beats generic) */
    case 101: if (map == 76) e = E_FIREWORKS; break;  case 214: if (map == 67) e = E_HEARTS; break;
    case 317: if (map == 117) e = E_GREEN; break;     case 701: if (map == 77) e = E_CANADA; break;
    case 1031: if (map == 53) e = E_WISPS; break;     case 1225: if (map == 87) e = E_SNOW; break; }
  *placeOnly = 0; cached = e;
  if (e != lastLogged) { lastLogged = e; if (e != E_NONE) { PSPNativeMemLog("[CELEB] effect '%s' active (map %d)", kEffName[e], map); if (achFor(e) >= 0) P3DS_AchUnlock(achFor(e)); } }
  return e; }

/* ---- drawing helpers: images are 256x192, 0x00BBGGRR ---- */
static unsigned hsh(unsigned a, unsigned b) { unsigned x = a * 2654435761u ^ (b + 0x9E3779B9u) * 2246822519u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; x *= 3266489917u; x ^= x >> 16; return x; }
static float frand(unsigned a, unsigned b) { return (hsh(a, b) & 0xFFFF) / 65536.0f; }
static void px(unsigned *img, int x, int y, unsigned col, int a) { /* alpha a/16 */
  if (x < 0 || x > 255 || y < 0 || y > 191 || a <= 0) return; unsigned d = img[y * 256 + x]; if (a > 16) a = 16;
  unsigned r = ((d & 255) * (16 - a) + (col & 255) * a) >> 4, g = (((d >> 8) & 255) * (16 - a) + ((col >> 8) & 255) * a) >> 4, b = (((d >> 16) & 255) * (16 - a) + ((col >> 16) & 255) * a) >> 4;
  img[y * 256 + x] = (d & 0xFF000000u) | r | (g << 8) | (b << 16); }
static void star(unsigned *img, int cx, int cy, int s, unsigned col, int a) { /* 4-point sparkle of radius s */
  px(img, cx, cy, 0x00FFFFFF, a);
  for (int k = 1; k <= s; k++) { int al = a - (k * a) / (s + 1); px(img, cx + k, cy, col, al); px(img, cx - k, cy, col, al); px(img, cx, cy + k, col, al); px(img, cx, cy - k, col, al); }
  if (s >= 2) { px(img, cx + 1, cy + 1, col, a / 2); px(img, cx - 1, cy + 1, col, a / 2); px(img, cx + 1, cy - 1, col, a / 2); px(img, cx - 1, cy - 1, col, a / 2); } }


static unsigned vT0; static int vOn, wasField;   /* rare variant particle: one per visit */
static const unsigned char kVar[6] = { 0x1C, 0x3E, 0x7F, 0x7F, 0x7F, 0x55 }; /* 7 wide, 6 high */
static void drawVariant(unsigned *img, int par) {
  if (!vOn) return; const float y = -8.0f + (T - vT0) * 0.5f; if (y > 200.0f) { vOn = 0; return; }
  const int x0 = 40 + (int)(hsh(vT0, 77) % 170) + par, y0 = (int)y;
  for (int r = 0; r < 6; r++) for (int c = 0; c < 7; c++) if (kVar[r] & (0x40 >> c)) px(img, x0 + c, y0 + r, 0x00FFFFFF, 14);
  px(img, x0 + 2, y0 + 2, 0x00282020, 16); px(img, x0 + 4, y0 + 2, 0x00282020, 16); }

static void drawBirthday(unsigned *img, int par) {
  static const unsigned pal[4] = { 0x0070E0FF, 0x00FFFFFF, 0x0090E8FF, 0x00E8F8FF }; /* white-gold */
  for (int i = 0; i < 40; i++) { const int L = i % 3, len = 80 + (int)(hsh(i, 1) % 70); const unsigned k = (T + hsh(i, 2) % len) / len, age = (T + hsh(i, 2) % len) % len;
    const float x = frand(i, k * 7 + 3) * 256.0f, y = frand(i, k * 7 + 4) * 170.0f + age * 0.18f;
    const int tri = age < (unsigned)len / 2 ? (int)age : len - (int)age, s = tri * 4 / (len / 2 + 1); /* 0..3..0 */
    star(img, (int)x + par * (L - 1), (int)y, s, pal[hsh(i, k) & 3], 14); } }

static void drawFireworks(unsigned *img, int par) {
  static const unsigned pal[5] = { 0x0040D0FF, 0x0060E0FF, 0x0030C0F0, 0x0080E8FF, 0x0050D8FF }; /* gold bursts */
  for (int b = 0; b < 3; b++) { const unsigned per = 150, t0 = T + b * 50, k = t0 / per, age = t0 % per; if (age > 80) continue;
    const float cx = 48 + frand(b, k) * 160, cy = 28 + frand(b, k + 99) * 70; const unsigned col = pal[hsh(b, k) % 5]; const int a = 16 - (int)age * 16 / 80;
    for (int j = 0; j < 22; j++) { const float ang = j * (6.2831853f / 22) + frand(b + 7, k) * 0.3f, v = 1.2f + frand(j, k + b) * 0.9f;
      const float x = cx + cosf(ang) * v * age * 0.55f, y = cy + sinf(ang) * v * age * 0.55f + 0.0045f * age * age;
      px(img, (int)x + par, (int)y, col, a); px(img, (int)x + 1 + par, (int)y, col, a); px(img, (int)x + par, (int)y + 1, col, a); px(img, (int)x + 1 + par, (int)y + 1, col, a / 2); } } }

static const unsigned char kHeart[6] = { 0x36, 0x7F, 0x7F, 0x3E, 0x1C, 0x08 }; /* 7 wide, 6 high */
static void drawHearts(unsigned *img, int par) {
  for (int i = 0; i < 16; i++) { const int L = i % 3; const float sp = 0.35f + 0.25f * L, ph = frand(i, 5) * 230.0f;
    const float y = 200.0f - fmodf(ph + T * sp, 230.0f), x = frand(i, 6) * 240.0f + 8.0f * sinf(T * 0.025f + i * 1.3f);
    const unsigned col = (i & 1) ? 0x00A070FF : 0x00C0A0FF; /* pink / light pink */
    for (int r = 0; r < 6; r++) for (int c = 0; c < 7; c++) if (kHeart[r] & (0x40 >> c)) px(img, (int)x + c + par * (L - 1), (int)y + r, col, 11); } }

static void drawRain(unsigned *img, int par, int kind) { /* green sparkle rain / red-white sparkles */
  for (int i = 0; i < 40; i++) { const int L = i % 3; const float sp = 0.5f + 0.35f * L, ph = frand(i, 11) * 215.0f;
    const float y = fmodf(ph + T * sp, 215.0f) - 12.0f, x = frand(i, 12) * 256.0f + 5.0f * sinf(T * 0.03f + i);
    unsigned col; if (kind == E_GREEN) col = (i % 6 == 0) ? 0x0060E0FF : (i & 1) ? 0x0050E040 : 0x0090FF90; else col = (i & 1) ? 0x002020E0 : 0x00FFFFFF;
    const int tw = (int)((T / 6 + i) % 4); if (tw == 0) px(img, (int)x + par * (L - 1), (int)y, col, 14); else star(img, (int)x + par * (L - 1), (int)y, tw == 2 ? 2 : 1, col, 13); } }

static void drawWisps(unsigned *img, int par) {
  for (int i = 0; i < 14; i++) { const int L = i % 3; const float sp = 0.22f + 0.12f * L, ph = frand(i, 21) * 240.0f;
    const float y = 205.0f - fmodf(ph + T * sp, 240.0f), x = frand(i, 22) * 240.0f + 14.0f * sinf(T * 0.02f + i * 1.7f);
    const int cx = (int)x + par * (L - 1), cy = (int)y;
    for (int t = 0; t < 5; t++) { const int ox = (int)(3.0f * sinf((T - t * 4) * 0.02f + i * 1.7f + 0.0f) - 3.0f * sinf(T * 0.02f + i * 1.7f)); /* trail sways behind */
      px(img, cx + ox, cy + t * 2, 0x00FF60B0, 9 - t * 2); px(img, cx + ox + 1, cy + t * 2, 0x00FF60B0, 6 - t); }
    for (int dy = -3; dy <= 3; dy++) for (int dx = -2; dx <= 2; dx++) px(img, cx + dx, cy + dy, 0x00FF70C0, 8 - (dx * dx + dy * dy / 2));
    px(img, cx, cy, 0x00FFD0F0, 16); px(img, cx, cy + 1, 0x00FFB0E0, 14); px(img, cx, cy - 1, 0x00FFB0E0, 12); } }

static void drawSnow(unsigned *img, int par) {
  for (int i = 0; i < 48; i++) { const int L = i % 3; const float sp = 0.3f + 0.3f * L, ph = frand(i, 31) * 215.0f;
    const float y = fmodf(ph + T * sp, 215.0f) - 12.0f, x = frand(i, 32) * 256.0f + 6.0f * sinf(T * 0.02f + i * 0.9f);
    const int cx = (int)x + par * (L - 1), cy = (int)y;
    if (L == 2) { px(img, cx, cy, 0x00FFFFFF, 15); px(img, cx + 1, cy, 0x00FFFFFF, 12); px(img, cx, cy + 1, 0x00FFFFFF, 12); px(img, cx + 1, cy + 1, 0x00FFFFFF, 9);
      if (i % 4 == 0) { px(img, cx - 1, cy, 0x00FFFFFF, 6); px(img, cx + 2, cy, 0x00FFFFFF, 6); px(img, cx, cy - 1, 0x00FFFFFF, 6); px(img, cx, cy + 2, 0x00FFFFFF, 6); } }
    else px(img, cx, cy, 0x00FFF0E8, L ? 12 : 8); } }

/* eye 0 = left / only, 1 = right; call once per eye per frame with that eye's top-screen image */
void P3DS_CelebDraw(unsigned *img, int eye) {
  if (!P3DS_InField || !P3DS_CelebOn()) { wasField = 0; return; }
  if (eye == 0) T++;
  int po = 0; const int e = effectNow(&po); if (e == E_NONE) return;
  const int sd = P3DS_StereoShiftNow(), par = eye ? -sd / 2 : sd / 2;
  { static int lastE, lastMap = -2; if (eye == 0) { if (e != lastE || P3DS_MmMap != lastMap || !wasField) { vOn = (e == E_BDAY); vT0 = T; } lastE = e; lastMap = P3DS_MmMap; wasField = 1; } }
  if (e == E_BDAY) drawVariant(img, par);
  switch (e) { case E_BDAY: drawBirthday(img, par); break; case E_FIREWORKS: drawFireworks(img, par); break; case E_HEARTS: drawHearts(img, par); break;
    case E_GREEN: case E_CANADA: drawRain(img, par, e); break; case E_WISPS: drawWisps(img, par); break; case E_SNOW: drawSnow(img, par); break; } }
