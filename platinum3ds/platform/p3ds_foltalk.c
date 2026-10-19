/* run 64 follower area dialogue (port-side text table, drawn by render3ds in the game's own font + message box).
   Picks a line for the follower from: the area (town / route / cave / lake / legendary spot ..., from p3ds_areas.h,
   generated from the decomp's map header NAMES), the weather, the time of day, the species and its types.
   Every line is original port text; nothing here comes from the ROM. "%s" = the follower's nickname. */
#include <stdio.h>
#include <string.h>
#include "p3ds_areas.h"
enum { OTHER, TOWN, ROUTE, SEA, LAKE, LEGEND, FOREST, CENTER, MART, GYM, MARSH, SPOOKY, LEAGUE, GALACTIC, INDOOR, SNOW, CAVE };
static const char *const kTown[18][2] = {
  { 0, 0 },
  { "%s seems to remember this quiet little town.", "%s is sniffing the fresh Twinleaf breeze." },
  { "%s keeps glancing at the lab. Professor Rowan is in there!", "%s can smell the sea from Sandgem." },
  { "%s is amazed by all the people in Jubilife!", "%s is staring up at the tall TV tower." },
  { "%s's nose twitches. It smells like coal dust.", "%s is listening to the mine carts rumble." },
  { "%s is surrounded by the sweet smell of flowers.", "%s wants to roll around in the flower beds." },
  { "%s is looking at the old Pokemon statue.", "%s is enjoying the calm Eterna air." },
  { "%s is humming along to the music in Hearthome.", "%s looks ready for a contest!" },
  { "%s is peeking at the Day Care.", "%s is gazing at the ruins near Solaceon." },
  { "%s is staring at the giant meteorites.", "%s wants to see the Department Store." },
  { "%s is hopping around near the marsh.", "%s is looking at the Great Marsh with excitement." },
  { "%s feels something ancient in Celestic Town.", "%s is quiet. This town feels very old." },
  { "%s watches the boats come into the harbor.", "%s is looking at the library with curiosity." },
  { "%s is shivering a little in the cold!", "%s is catching snowflakes!" },
  { "%s is squinting in the bright Sunyshore sun.", "%s is watching the solar panels glitter." },
  { "%s is pumped up! This place is full of strong Trainers.", "%s wants to battle someone!" },
  { "%s feels like it could survive anything here!", "%s is looking at the rocky cliffs." },
  { "%s is relaxing in the warm resort breeze.", "%s wants to take a nap by the villa." },
};
static const char *const kCat[17][3] = {
  [OTHER]    = { "%s is looking around curiously.", "%s seems to be enjoying itself.", 0 },
  [TOWN]     = { "%s is happy to be in town.", 0, 0 },
  [ROUTE]    = { "%s is sniffing the tall grass.", "%s is enjoying the walk along the route.", "%s is keeping an eye out for wild Pokemon." },
  [SEA]      = { "%s is watching the waves roll in.", "%s can taste salt in the sea breeze.", 0 },
  [LAKE]     = { "%s is staring at the lake. Something lives here...", "%s is gazing at the still water. It feels sacred.", 0 },
  [LEGEND]   = { "%s is trembling. A powerful presence is nearby...", "%s is very quiet. This place feels special.", "%s is looking around warily." },
  [FOREST]   = { "%s is listening to the rustling trees.", "%s is a little nervous in the dark forest.", 0 },
  [CENTER]   = { "%s looks relieved to be at the Pokemon Center.", "%s is waving at Nurse Joy... maybe.", 0 },
  [MART]     = { "%s is looking at all the shelves.", "%s seems to want a new item!", 0 },
  [GYM]      = { "%s is fired up for a Gym battle!", "%s is staring down the Gym Leader's spot.", 0 },
  [MARSH]    = { "%s is stepping carefully through the mud.", "%s is splashing in the marsh!", 0 },
  [SPOOKY]   = { "%s is sticking close to you...", "%s is shaking. Did something just move?", "%s keeps looking over its shoulder." },
  [LEAGUE]   = { "%s looks determined. This is the big one!", "%s feels the pressure of the Pokemon League.", 0 },
  [GALACTIC] = { "%s doesn't like this place one bit.", "%s is on guard. Team Galactic is around!", 0 },
  [INDOOR]   = { "%s is looking around the room.", "%s seems comfy indoors.", 0 },
  [SNOW]     = { "%s is shivering in the snow!", "%s is leaving little tracks in the snow.", "%s is catching snowflakes!" },
  [CAVE]     = { "%s is listening to its voice echo in the cave.", "%s is peering into the darkness.", "%s is sniffing the damp cave air." },
};
static const char *weatherLine(int w, unsigned s) {
  static const char *const rain[] = { "%s is shaking off the rain.", "%s doesn't seem to mind the rain." };
  static const char *const snow[] = { "%s is shivering in the snow!", "%s is catching snowflakes!" };
  static const char *const fog[]  = { "%s is sticking close in the thick fog.", "%s can barely see through the fog..." };
  static const char *const sand[] = { "%s is squinting against the sandstorm.", "%s is shaking sand out of its fur." };
  static const char *const ash[]  = { "%s is sneezing at the falling ash.", 0 };
  if (w >= 2 && w <= 4) return rain[s & 1];
  if ((w >= 5 && w <= 7) || w == 11) return snow[s & 1];
  if (w == 14 || w == 15) return fog[s & 1];
  if (w == 10) return sand[s & 1];
  if (w == 9) return ash[0];
  return 0; }
static const char *todLine(int tod, unsigned s) {
  static const char *const morning[] = { "%s greets the morning sun!", "%s is full of energy this morning." };
  static const char *const evening[] = { "%s is watching the sunset.", 0 };
  static const char *const night[] = { "%s is looking up at the stars.", "%s is getting a little sleepy..." };
  if (tod == 0) return morning[s & 1]; if (tod == 2) return evening[0]; if (tod >= 3) return night[s & 1]; return 0; }
static const char *typeLine(int t, int outdoor, int weather, unsigned s) {
  switch (t) {
  case 10: return weather >= 2 && weather <= 4 ? "%s is trying to keep its flame dry!" : "%s is warm to the touch.";
  case 11: return weather >= 2 && weather <= 4 ? "%s loves the rain!" : "%s wants to go for a swim.";
  case 12: return outdoor ? "%s is soaking up the sunlight." : "%s misses the sunshine.";
  case 13: return "%s's fur is crackling with static!";
  case 15: return (weather >= 5 && weather <= 7) ? "%s is right at home in the snow!" : "%s is giving off a little chill.";
  case 7:  return "%s is fading in and out of the shadows...";
  case 14: return "%s seems to know what you're thinking.";
  case 2:  return outdoor ? "%s is watching the sky." : "%s wants to fly outside.";
  case 4: case 5: return "%s is digging at the ground.";
  case 6:  return "%s is chasing a bug... no wait, it is a bug.";
  case 16: return "%s looks proud and mighty.";
  case 17: return "%s is grinning mischievously.";
  case 8:  return "%s is polishing its steel body.";
  case 1:  return "%s is practicing its punches!";
  case 3:  return "%s is oozing a little. Ew.";
  }
  (void)s; return 0; }
static const char *speciesLine(int sp) {
  switch (sp) {
  case 25: return "%s's cheeks are sparking with happiness!";
  case 133: return "%s is wagging its fluffy tail.";
  case 387: case 388: case 389: return "%s is stretching its leaves.";
  case 390: case 391: case 392: return "%s's flame is burning brightly!";
  case 393: case 394: case 395: return "%s is puffing out its chest proudly.";
  case 143: return "%s let out a huge yawn...";
  case 54: return "%s is holding its head. Headache again?";
  case 448: case 447: return "%s is sensing the aura around you.";
  case 483: return "%s seems to be looking through time itself...";
  case 484: return "%s seems to be looking through space itself...";
  case 487: return "%s is staring at a world no one else can see.";
  }
  return 0; }
int P3DS_FolAreaCat(int map, int isCave) {
  int a = map >= 0 && map < (int)(sizeof kP3dsArea / sizeof kP3dsArea[0]) ? kP3dsArea[map] : 0;
  if ((a & 31) == OTHER && isCave) a = CAVE;
  return a; }
/* returns 1 and fills out */
int P3DS_FolLine(char *out, int n, const char *name, int map, int isCave, int isOutdoor, int weather, int tod, int t1, int t2, int species, unsigned seed) {
  const char *cand[16]; int nc = 0;
  int a = P3DS_FolAreaCat(map, isCave), cat = a & 31, sub = a >> 5;
  const char *w = weatherLine(weather, seed >> 3);
  if (w && isOutdoor) { cand[nc++] = w; cand[nc++] = w; }
  if (cat == TOWN || cat == INDOOR || (cat == SNOW && sub == 13)) { if (sub >= 1 && sub <= 17) { cand[nc++] = kTown[sub][(seed >> 2) & 1]; cand[nc++] = kTown[sub][((seed >> 2) & 1) ^ 1]; } }
  for (int i = 0; i < 3; i++) if (kCat[cat][i]) cand[nc++] = kCat[cat][i];
  if (cat == LEGEND || cat == LAKE || cat == SPOOKY) { cand[nc++] = kCat[cat][0]; cand[nc++] = kCat[cat][1]; }
  if (isOutdoor) { const char *t = todLine(tod, seed >> 4); if (t) cand[nc++] = t; }
  { const char *t = typeLine(t1, isOutdoor, weather, seed); if (t) cand[nc++] = t; }
  if (t2 != t1) { const char *t = typeLine(t2, isOutdoor, weather, seed); if (t) cand[nc++] = t; }
  { const char *t = speciesLine(species); if (t) cand[nc++] = t; }
  if (cat != OTHER) cand[nc++] = kCat[OTHER][seed & 1];
  if (!nc) cand[nc++] = kCat[OTHER][0];
  const char *f = cand[(seed * 2654435761u >> 16) % (unsigned)nc];
  /* "Pokemon" -> "Pok\xe9mon" (game font has the accent) */
  char tmp[128]; snprintf(tmp, sizeof tmp, f, name);
  int o = 0; for (int i = 0; tmp[i] && o < n - 1; i++) { if (!strncmp(tmp + i, "Pokemon", 7)) { const char *p = "Pok\xe9mon"; for (int k = 0; p[k] && o < n - 1; k++) out[o++] = p[k]; i += 6; } else out[o++] = tmp[i]; }
  out[o] = 0;
  return 1; }
