/* ss3ds wide_layouts: the built-in wide layout of each 2D screen (see ss3ds_widescreens.c for the row kinds).
   Columns and lines are the DS's (0..255, 0..191); the wide screen shows DS columns -32..287.
   WL_MOVE(bgs, top, bottom, x0, x1, dx): those BG columns drawn dx px to the right (negative: left).
   WL_FILL(bgs, top, bottom, x0, x1, col): wide columns x0..x1-1 show the BGs' column col.
   WL_OBJS(top, bottom, x0, x1, dx): sprites whose middle is in that box move dx px.
   WL_COPY(bgs, top, bottom, x0, x1, col): wide columns x0..x1-1 show the BGs' columns col, col+1, ... (textured stretch).
   WL_WRAP(bgs, top, bottom): beside the DS screen the BGs' own map continues (repeating patterns, also scrolling ones).
   bgs: bit n = BG n; add WL_SCREEN when x0..x1 are screen columns of a scrolling BG. */
static const short kWlNone[1][8] = { { 0 } };
/* Pokemon summary (engine A on top; BG0 3D Pokemon, BG1 text, BG2 page extras, BG3 the panel art). The left page panel gets
   64 px wider: its interior just left of the frame (columns 127..134, clear on all three pages) is repeated eight times, so the
   memo's dotted lines continue as dots; on the HP bar's lines one plain column is used instead. Text stays at the panel's left;
   the name / portrait / item block moves to the right edge with the Pokemon (3D) and its sprites. */
#define WL_SUM8(k) WL_COPY(0x8, 0, 41, 104 + 8 * (k), 112 + 8 * (k), 127), WL_COPY(0x8, 48, 192, 104 + 8 * (k), 112 + 8 * (k), 127)
static const short kWlSummaryA[][8] = {
    WL_MOVE(0x8, 0, 192, 0, 136, -32),
    WL_SUM8(0), WL_SUM8(1), WL_SUM8(2), WL_SUM8(3), WL_SUM8(4), WL_SUM8(5), WL_SUM8(6), WL_SUM8(7), WL_FILL(0x8, 41, 48, 104, 168, 135),
    WL_MOVE(0x8, 0, 192, 136, 256, 32),
    WL_MOVE(0x6, 0, 192, 0, 141, -32), WL_MOVE(0x6, 0, 192, 141, 256, 32),
    WL_OBJS(0, 192, 0, 141, -32), WL_OBJS(0, 192, 141, 256, 32), WL_SHIFT3D(32),
};
/* Party menu (engine B on top: BG1 the framed panel with the wave and Poke Ball, BG0 the bar along the bottom edge). The frame's
   left edge goes to the screen edge, everything else 32 px right; the 64 px between continue the panel (column 21). */
static const short kWlPartyB[][8] = {
    WL_MOVE(0x2, 0, 192, 0, 20, -32), WL_FILL(0x2, 0, 192, -12, 52, 21), WL_MOVE(0x2, 0, 192, 20, 256, 32),
    WL_MOVE(0x1, 0, 192, 0, 16, -32), WL_FILL(0x1, 0, 192, -16, 48, 16), WL_MOVE(0x1, 0, 192, 16, 256, 32),
};
/* Bag (engine A on top: BG0 3D player, BG1 text, BG2 the description bar, BG3 a banded backdrop). The backdrop's bands run on to
   both edges; the description bar's end pieces go to the screen edges and its middle is continued. */
static const short kWlBagA[][8] = {
    WL_FILL(0x8, 0, 192, -32, 0, 0), WL_FILL(0x8, 0, 192, 256, 288, 255),
    WL_MOVE(0x4, 0, 192, 0, 12, -32), WL_FILL(0x4, 0, 192, -20, 12, 14), WL_MOVE(0x4, 0, 192, 244, 256, 32), WL_FILL(0x4, 0, 192, 244, 276, 242),
};
/* Options (top: BG0 a banded backdrop, BG1 the sample text box, which stays the size the game's text boxes are). Either engine. */
static const short kWlOptions[][8] = { WL_FILL(0x1, 0, 192, -32, 0, 0), WL_FILL(0x1, 0, 192, 256, 288, 255) };
/* Pokegear, all four tabs (engine B on top: BG0 the curved backdrop, BG3 the call / radio / map window, sprites the clock). The backdrop opens in the
   middle, where its curves are level: the halves move to the edges and column 128 fills the 64 px between. */
static const short kWlGearPhoneB[][8] = { WL_MOVE(0x1, 0, 192, 0, 128, -32), WL_FILL(0x1, 0, 192, 96, 160, 128), WL_MOVE(0x1, 0, 192, 128, 256, 32) };
/* Trainer card, front (engine A on top: BG3 a scrolling tiled pattern, BG2 the badge band and the LEAGUE BADGES tab, sprites the
   badges). The pattern's own map continues past the edges, the band runs on, the tab sits at the left edge. */
static const short kWlTrainerCardA[][8] = {
    WL_WRAP(0x8 | WL_SCREEN, 0, 192), WL_FILL(0x4, 0, 192, -32, 0, 0), WL_FILL(0x4, 0, 192, 256, 288, 255), WL_MOVE(0x4, 0, 26, 0, 112, -32),
};
/* PC boxes (engine B on top: BG1 the title bar, the info panel flush with the left edge and the striped blue behind it,
   BG0 the text, sprites the Pokemon and its type). The panel goes to the real left edge, the blue runs on to the right. */
static const short kWlPcBoxB[][8] = {
    WL_MOVE(0x2, 0, 192, 0, 220, -32), WL_FILL(0x2, 0, 192, 188, 252, 220), WL_MOVE(0x2, 0, 192, 220, 256, 32),
    WL_MOVE(0x1, 0, 192, 0, 256, -32), WL_OBJS(0, 192, 0, 256, -32) };
/* Side strips continue the picture's own edge columns (given to both engines). Naming screen: the top is black with the question box,
   so the sides are black. Fly map, Voltorb Flip, Safari editors, berry pots, Scratch-Off, signature: a plain or
   gradient top, so the sides continue it. */
static const short kWlBlackSides[][8] = { WL_FILL(0xF, 0, 192, -32, 0, 0), WL_FILL(0xF, 0, 192, 256, 288, 255) };
/* Geonet: the grid is on BG3 alone, the dialogue frame must not wrap with it. */
static const short kWlWrapBg3[][8] = { WL_WRAP(0x8, 0, 192) };
/* Apricorn box (engine B on top: the diagonal stripes and the dialogue frame are both on BG3). Above the frame the stripes wrap;
   beside the frame the edge columns continue, so no piece of the frame is repeated. */
static const short kWlApricornB[][8] = { WL_WRAP(0x8, 0, 130), WL_FILL(0x8, 130, 192, -32, 0, 0), WL_FILL(0x8, 130, 192, 256, 288, 255) };
static const WlScreen kWlScreens[] = {
    { "summary", kWlSummaryA, WL_N(kWlSummaryA), kWlNone, 0 },
    { "party", kWlNone, 0, kWlPartyB, WL_N(kWlPartyB) },
    { "bag", kWlBagA, WL_N(kWlBagA), kWlNone, 0 },
    { "options", kWlOptions, WL_N(kWlOptions), kWlOptions, WL_N(kWlOptions) },
    { "gearphone", kWlNone, 0, kWlGearPhoneB, WL_N(kWlGearPhoneB) },
    { "pokegear", kWlNone, 0, kWlGearPhoneB, WL_N(kWlGearPhoneB) },
    { "gearconfig", kWlNone, 0, kWlGearPhoneB, WL_N(kWlGearPhoneB) },
    { "gearradio", kWlNone, 0, kWlGearPhoneB, WL_N(kWlGearPhoneB) },
    { "gearmap", kWlNone, 0, kWlGearPhoneB, WL_N(kWlGearPhoneB) },
    { "naming", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "flymap", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "unownreport", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "voltorbflip", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "safaricustom", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "safarideco", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "berrypots", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "scratch", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "signature", kWlBlackSides, WL_N(kWlBlackSides), kWlBlackSides, WL_N(kWlBlackSides) },
    { "geonet", kWlWrapBg3, WL_N(kWlWrapBg3), kWlWrapBg3, WL_N(kWlWrapBg3) },
    { "apricorn", kWlNone, 0, kWlApricornB, WL_N(kWlApricornB) },
    { "pcbox", kWlNone, 0, kWlPcBoxB, WL_N(kWlPcBoxB) },
    { "trainercardmain", kWlTrainerCardA, WL_N(kWlTrainerCardA), kWlNone, 0 },
};
