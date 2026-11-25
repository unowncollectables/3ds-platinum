/* ss3ds 0298/0299/0300: Black 2 / White 2 animated sprites (Settings > Video "Black 2 sprites", key b2_sprites, default OFF).
 *
 * Platinum port (from SoulSilver 08b73fd): same file, pokeplatinum names for the NARC readers below, and the pack is
 * read from sdmc:/3ds/platinum3ds/b2sprites.bin, or else from SoulSilver's folder (the pack is the same for both games).
 *
 * The pack sdmc:/3ds/soulsilver3ds/b2sprites.bin is made on the PC from the user's own Black 2 / White 2 ROM by
 * tools/b2sprites/build_pack.sh (AnimaEngine, MIT). It is never part of the build or the repository. Format: b2pack.py.
 *
 * Game side (hook 0298): pokemon.c tells us what each battle sprite template shows (SS3DS_B2Note), pokepic.c asks for
 * the Black 2 palette when a battler's palette loads (SS3DS_B2Palette) and for the current frame when it draws
 * (SS3DS_B2Pic). Each of the four battler slots gets its own platform texture (g3soft.cpp P3DS_B2TexPixels), so big
 * Pokemon are drawn at full size. Any "no" here makes the game draw its own sprite, unchanged.
 * The toggle is read once at the start of each battle, so a battle never mixes palettes. Never touches the save. */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int P3DS_InBattle;
extern int P3DS_CfgInt(const char *, int);
extern void P3DS_CfgWrite(const char *, int);
extern void PSPNativeMemLog(const char *, ...);
extern u32 *P3DS_B2TexPixels(int slot, unsigned w, unsigned h);
extern void P3DS_B2TexFree(int slot);
extern void P3DS_Use(const char *);

#define B2_PATH_OWN "sdmc:/3ds/platinum3ds/b2sprites.bin"
#define B2_PATH_SS "sdmc:/3ds/soulsilver3ds/b2sprites.bin"
static const char *b2Path = B2_PATH_OWN;
#define B2_PATH b2Path
#define B2_TEX_OFF 0x7FF80 /* + 8 * slot; port-side textures, see g3soft.cpp b2Lookup */
#define B2_PLTT 0x1FE80

typedef struct { u16 species; u8 form, flags; u32 off, size, pad; } B2Dir;

static int b2Cfg = -1;
int P3DS_B2Cfg(void) { if (b2Cfg < 0) { b2Cfg = P3DS_CfgInt("b2_sprites", 0) != 0; PSPNativeMemLog("[B2] b2_sprites=%d (Black 2 battle sprites, hook 0298, default OFF)", b2Cfg); } return b2Cfg; }
void P3DS_B2SetCfg(int on) { b2Cfg = on != 0; P3DS_CfgWrite("b2_sprites", b2Cfg); PSPNativeMemLog("[B2] b2_sprites=%d from settings (next battle)", b2Cfg); }

/* ---- pack directory ---- */
static int packState = -1; /* -1 not tried, 0 missing/bad, 1 ok */
static B2Dir *packDir;
static unsigned packN;
static int packOpen(void) {
    if (packState >= 0) return packState;
    packState = 0;
    FILE *f = fopen(B2_PATH_OWN, "rb");
    if (!f) { f = fopen(B2_PATH_SS, "rb"); if (f) b2Path = B2_PATH_SS; }
    if (!f) { PSPNativeMemLog("[B2] %s (or %s) missing: Pokemon pictures stay the game's own (make it with the builder's Black 2 sprites step)", B2_PATH_OWN, B2_PATH_SS); return 0; }
    u32 h[4];
    if (fread(h, 4, 4, f) == 4 && h[0] == 0x50533242u /* 'B2SP' */ && h[1] == 1 && h[2] && h[2] < 8192) {
        packDir = (B2Dir *)malloc(h[2] * sizeof(B2Dir));
        if (packDir && fread(packDir, sizeof(B2Dir), h[2], f) == h[2]) { packN = h[2]; packState = 1; }
        else { free(packDir); packDir = NULL; }
    }
    fclose(f);
    PSPNativeMemLog("[B2] %s %s: %u sprites", b2Path, packState ? "ok" : "BAD (rebuild it with the builder)", packN);
    return packState;
}
static int packFind(int species, int form, int back) {
    int lo = 0, hi = (int)packN - 1;
    const u32 key = ((u32)species << 16) | ((u32)form << 8) | (u32)back; /* back: bit0 back, bit1 female */
    while (lo <= hi) {
        const int mid = (lo + hi) / 2;
        const u32 k = ((u32)packDir[mid].species << 16) | ((u32)packDir[mid].form << 8) | packDir[mid].flags;
        if (k == key) return mid;
        if (k < key) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

/* ---- slots (defined below) ---- */
typedef struct Slot Slot;
static void slotsFree(void);
/* per battler speed for the Black 2 feel (hook 0300, below); speed4: 4 normal, 6 yellow, 8 red, 0 stopped */
typedef struct { u16 narc, chr, pal; u8 speed4, used; } BState;
static BState bstate[4];

/* ---- per battle latch ---- */
static int latched, wasBattle;
static int battleOn(void) {
    if (P3DS_InBattle && !wasBattle) {
        slotsFree(); /* last battle's sprites */
        memset(bstate, 0, sizeof bstate);
        latched = P3DS_B2Cfg() && packOpen() == 1;
        if (latched) { P3DS_Use("b2_sprites"); PSPNativeMemLog("[B2] battle: Black 2 sprites ON"); }
    }
    wasBattle = P3DS_InBattle != 0;
    return P3DS_InBattle && latched;
}
/* Outside battle (summary, Hall of Fame, starter pick...): live toggle, no latch; pictures loaded while it was off keep
   the game's art because each slot only draws after its own palette load said yes (palOk). */
static int anyLoaded;
int SS3DS_B2Want(void) {
    if (P3DS_InBattle) return battleOn();
    battleOn(); /* notes the end of a battle */
    const int on = P3DS_B2Cfg() && packOpen() == 1;
    if (!on && anyLoaded) { slotsFree(); anyLoaded = 0; }
    return on;
}

/* ---- what each template shows (filled from pokemon.c) ---- */
typedef struct { u16 narc, chr, pal; u16 species; u8 gender, facing, shiny, form, height, used; } Note;
static Note notes[32];
static unsigned noteNext;
void SS3DS_B2Note(int narc, int chr, int pal, int species, int gender, int facing, int shiny, int form, int height) {
    for (unsigned i = 0; i < 32; i++) {
        Note *n = &notes[i];
        if (n->used && n->narc == narc && n->chr == chr && n->pal == pal) {
            n->species = (u16)species; n->gender = (u8)gender; n->facing = (u8)facing; n->shiny = (u8)(shiny != 0); n->form = (u8)form; n->height = (u8)height;
            return;
        }
    }
    Note *n = &notes[noteNext++ & 31];
    n->used = 1; n->narc = (u16)narc; n->chr = (u16)chr; n->pal = (u16)pal; n->species = (u16)species; n->gender = (u8)gender;
    n->facing = (u8)facing; n->shiny = (u8)(shiny != 0); n->form = (u8)form; n->height = (u8)height;
}
static const Note *noteFind(int narc, int chr, int pal) {
    for (unsigned i = 0; i < 32; i++) if (notes[i].used && notes[i].narc == narc && notes[i].chr == chr && notes[i].pal == pal) return &notes[i];
    return NULL;
}

/* ---- slots ---- */
struct Slot {
    int dir;           /* pack entry index, -1 none */
    u8 *blob;          /* whole entry */
    unsigned w, h, tw, th, nUniq, nSeq, delayCs, bw;
    int ox, oy;
    const u16 *palN, *palS, *seq;
    const u8 *frames;
    u64 t0, lastT, acc; /* acc: animation time so far, scaled by the battle speed (hook 0300) */
    int lastFrame;
    int palOk; /* the last palette load of this pic took the Black 2 colours */
    u16 lastPal[16];
};
static Slot slots[4] = { { .dir = -1 }, { .dir = -1 }, { .dir = -1 }, { .dir = -1 } };
static Slot fileSlot = { .dir = -1 }; /* the sprite last used for a 2D picture (file substitution below) */
static void slotsFree(void) { for (int i = 0; i < 4; i++) { free(slots[i].blob); slots[i].blob = NULL; slots[i].dir = -1; slots[i].palOk = 0; P3DS_B2TexFree(i); } }
static unsigned loads;

static unsigned pow2(unsigned v) { unsigned p = 8; while (p < v) p <<= 1; return p; }
static int slotLoad(Slot *s, int dir) {
    if (s->dir == dir) return 1;
    free(s->blob); s->blob = NULL; s->dir = -1;
    const B2Dir *d = &packDir[dir];
    if (d->size < 80 || d->size > 4u << 20) return 0;
    u8 *b = (u8 *)malloc(d->size);
    if (!b) { PSPNativeMemLog("[B2] out of memory for sprite %u (%u bytes)", (unsigned)dir, (unsigned)d->size); return 0; }
    FILE *f = fopen(B2_PATH, "rb");
    const int ok = f && !fseek(f, (long)d->off, SEEK_SET) && fread(b, 1, d->size, f) == d->size;
    if (f) fclose(f);
    const u16 *h16 = (const u16 *)b;
    const unsigned w = h16[0], hh = h16[1], nu = h16[4], ns = h16[5], dl = h16[6];
    const unsigned bw = (w + 1) / 2, hdr = 16 + 64 + ns * 2, hdrA = (hdr + 3) & ~3u;
    if (!ok || !w || !hh || w > 256 || hh > 256 || !nu || !ns || hdrA + nu * bw * hh > d->size) {
        free(b); PSPNativeMemLog("[B2] sprite %u unreadable (%s)", (unsigned)dir, ok ? "bad entry" : "read failed"); return 0;
    }
    s->blob = b; s->dir = dir; s->w = w; s->h = hh; s->tw = pow2(w); s->th = pow2(hh); s->nUniq = nu; s->nSeq = ns; s->delayCs = dl ? dl : 5; s->bw = bw;
    s->ox = (s16)h16[2]; s->oy = (s16)h16[3]; s->palN = h16 + 8; s->palS = h16 + 24; s->seq = h16 + 40; s->frames = b + hdrA;
    s->t0 = s->lastT = svcGetSystemTick(); s->acc = 0; s->lastFrame = -1; anyLoaded = 1;
    if (loads++ < 40) PSPNativeMemLog("[B2] %s %d: species %u form %u %s, %ux%u, %u frames (%u unique)", s == &fileSlot ? "file" : "slot", s == &fileSlot ? 0 : (int)(s - slots), d->species, d->form, (d->flags & 1) ? "back" : "front", w, hh, ns, nu);
    return 1;
}
/* Spinda uses its plain Black 2 sprite (Nathan 2026-11-24: turn the toggle off to see its spots). */
static int noteDir(const Note *n) {
    if (!n || !n->species) return -1;
    const int back = n->facing != 2;
    const int fem = n->gender == 1 ? 2 : 0; /* flags bit1: the Pokemon's own female art, if it has any */
    int dir = fem ? packFind(n->species, n->form, back | fem) : -1;
    if (dir < 0 && fem && n->form) dir = packFind(n->species, 0, back | fem);
    if (dir < 0) dir = packFind(n->species, n->form, back);
    if (dir < 0 && n->form) dir = packFind(n->species, 0, back);
    return dir;
}
static int slotFor(int slot, int narc, int chr, int pal, const Note **outNote) {
    if (slot < 0 || slot > 3 || !SS3DS_B2Want()) return 0;
    const Note *n = noteFind(narc, chr, pal);
    const int dir = noteDir(n);
    if (dir < 0) return 0;
    if (!slotLoad(&slots[slot], dir)) return 0;
    *outNote = n;
    return 1;
}

int SS3DS_B2Palette(int slot, int narc, int chr, int pal, u16 *dst16) {
    const Note *n;
    if (slot >= 0 && slot < 4) slots[slot].palOk = 0;
    if (!slotFor(slot, narc, chr, pal, &n)) return 0;
    memcpy(dst16, n->shiny ? slots[slot].palS : slots[slot].palN, 32);
    slots[slot].palOk = 1;
    return 1;
}

/* Where a w x h frame (feet at ox, oy) goes inside the game's 80x80 box when it has to fit there: scale s256 (<= 1),
   left edge x0 and top y0, feet on the game's ground line (80 - height) as far as the box allows. */
static void fitBox(unsigned w, unsigned h, int ox, int oy, int height, int *s256, int *x0, int *y0) {
    int s = 256;
    if (w > 80) s = 80 * 256 / (int)w;
    if (h > 80 && 80 * 256 / (int)h < s) s = 80 * 256 / (int)h;
    const int sw = (int)w * s >> 8, sh = (int)h * s >> 8;
    int x = 40 - (ox * s >> 8), y = (80 - height) - (oy * s >> 8);
    if (x < 0) x = 0;
    if (x > 80 - sw) x = 80 - sw;
    if (y < 0) y = 0;
    if (y > 80 - sh) y = 80 - sh;
    *s256 = s; *x0 = x; *y0 = y;
}

/* ---- Black 2 battle feel (hook 0300): Battle_Run reports each battler's picture, HP and status every frame. Like Black 2
   the loop slows in the yellow (x1.5) and red (x2) HP range and stops while the Pokemon is asleep or frozen. ---- */
void SS3DS_B2BattleMon(int battler, int narc, int chr, int pal, int hp, int maxHp, unsigned status) {
    if ((unsigned)battler >= 4) return;
    BState *b = &bstate[battler];
    if (!b->used) { b->speed4 = 4; PSPNativeMemLog("[B2] battler %d: speed follows HP and status (hook 0300)", battler); }
    b->narc = (u16)narc; b->chr = (u16)chr; b->pal = (u16)pal; b->used = 1;
    unsigned sp = 4;
    if (status & (7u | (1u << 5))) sp = 0; /* STATUS_SLEEP, STATUS_FREEZE */
    else if (maxHp > 0 && hp > 0) {
        unsigned px = (unsigned)hp * 48u / (unsigned)maxHp; /* the HP bar's 48 pixels, same colour rule as HpBar_GetColorIdx */
        if (px == 0) px = 1;
        sp = px * 2 > 48 ? 4 : px * 5 > 48 ? 6 : 8;
    }
    if (sp != b->speed4) PSPNativeMemLog("[B2] battler %d %s", battler, sp == 0 ? "asleep/frozen: still" : sp == 4 ? "normal speed" : sp == 6 ? "yellow HP: slower" : "red HP: slowest");
    b->speed4 = (u8)sp;
}
static unsigned battleSpeed4(int narc, int chr, int pal) {
    if (!P3DS_InBattle) return 4;
    for (int i = 0; i < 4; i++) if (bstate[i].used && bstate[i].narc == narc && bstate[i].chr == chr && bstate[i].pal == pal) return bstate[i].speed4;
    return 4;
}

typedef struct { int texOff, sizeS, sizeT, w, h, ox, oy, height, scale256, pltt; } SS3DSB2Pic;
int SS3DS_B2Pic(int slot, int narc, int chr, int pal, const u16 *livePltt, SS3DSB2Pic *o) {
    const Note *n;
    if (!slotFor(slot, narc, chr, pal, &n) || !slots[slot].palOk) return 0;
    Slot *s = &slots[slot];
    const u64 now = svcGetSystemTick();
    u64 dt = now - s->lastT;
    s->lastT = now;
    if (dt > CPU_TICKS_PER_MSEC * 500ull) dt = CPU_TICKS_PER_MSEC * 500ull; /* a long pause (menu, load) is not a jump */
    const unsigned sp4 = battleSpeed4(narc, chr, pal);
    if (sp4) s->acc += dt * 4u / sp4;
    const unsigned step = (unsigned)(s->acc / (CPU_TICKS_PER_MSEC * 10ull * s->delayCs));
    const int fr = s->seq[step % s->nSeq] < s->nUniq ? s->seq[step % s->nSeq] : 0;
    if (fr != s->lastFrame || memcmp(livePltt, s->lastPal, 32)) {
        u32 *px = P3DS_B2TexPixels(slot, s->tw, s->th);
        if (!px) return 0;
        u32 c[16];
        for (int i = 0; i < 16; i++) { const unsigned v = livePltt[i], r = v & 31, g = (v >> 5) & 31, b = (v >> 10) & 31; c[i] = (r << 3 | r >> 2) | ((g << 3 | g >> 2) << 8) | ((b << 3 | b >> 2) << 16) | 0xFF000000u; }
        c[0] = 0;
        memset(px, 0, s->tw * s->th * 4);
        const u8 *src = s->frames + (size_t)fr * s->bw * s->h;
        for (unsigned y = 0; y < s->h; y++) {
            u32 *row = px + y * s->tw;
            const u8 *sr = src + y * s->bw;
            for (unsigned x = 0; x < s->w; x++) row[x] = c[(sr[x >> 1] >> ((x & 1) * 4)) & 15];
        }
        s->lastFrame = fr;
        memcpy(s->lastPal, livePltt, 32);
    }
    unsigned ls = 0, lt = 0;
    while ((8u << ls) < s->tw) ls++;
    while ((8u << lt) < s->th) lt++;
    o->texOff = B2_TEX_OFF + slot * 8; o->sizeS = (int)ls; o->sizeT = (int)lt; o->w = (int)s->w; o->h = (int)s->h; o->ox = s->ox; o->oy = s->oy;
    o->height = n->height; o->scale256 = 256; o->pltt = B2_PLTT;
    if (!P3DS_InBattle) { /* summary, Hall of Fame...: keep it inside the game's own 80x80 picture box */
        int sc, x0, y0;
        fitBox(s->w, s->h, s->ox, s->oy, n->height, &sc, &x0, &y0);
        o->scale256 = sc;
        o->ox = (40 - x0) * 256 / sc;
        o->height = 80 - (y0 + (s->oy * sc >> 8));
    }
    return 1;
}

/* ---- the game's own file reads (link wraps, see Makefile) ----
   Battle move effects (Intimidate, Rock Throw...), send-outs and the 2D screens build their Pokemon pictures straight
   from the pokegra files, not from the 3D quad above. When a file read is a sprite or palette a note stands for, the
   loaded file is rewritten in place: the NCGR gets the first Black 2 frame fitted into the game's 80x80 box (both
   80x80 halves, scrambled again the way UnscanPokepic expects), the NCLR the Black 2 colours. Same size, same format. */
static const Note *noteForFile(int narc, int file, int *isPal) {
    for (unsigned i = 0; i < 32; i++) {
        const Note *n = &notes[i];
        if (!n->used || n->narc != narc) continue;
        if (n->chr == file) { *isPal = 0; return n; }
        if (n->pal == file) { *isPal = 1; return n; }
    }
    return NULL;
}
static u8 *nitroSection(u8 *f, unsigned size, u32 magic) {
    if (size < 0x10) return NULL;
    unsigned off = ((u16 *)f)[6]; /* header size */
    const unsigned nsec = ((u16 *)f)[7];
    for (unsigned i = 0; i < nsec && off + 8 <= size; i++) {
        u32 m, sl;
        memcpy(&m, f + off, 4); memcpy(&sl, f + off + 4, 4);
        if (m == magic) return f + off;
        if (sl < 8) break;
        off += sl;
    }
    return NULL;
}
static unsigned fileSubs;
static void fileSubst(int narc, int file, void *data, unsigned size) {
    if (!data || size < 0x30 || !packState || !SS3DS_B2Want()) return;
    int isPal;
    const Note *n = noteForFile(narc, file, &isPal);
    const int dir = noteDir(n);
    if (dir < 0) return;
    u8 *f = (u8 *)data;
    u32 magic, fsz; /* Nitro file header: magic, BOM/version, file size */
    memcpy(&magic, f, 4); memcpy(&fsz, f + 8, 4);
    if (magic != (isPal ? 0x4E434C52u /* 'RLCN' */ : 0x4E434752u /* 'RGCN' */) || fsz < 0x30 || fsz > size) return;
    size = fsz;
    if (isPal) {
        u8 *sec = nitroSection(f, size, 0x504C5454u /* 'TTLP' */);
        if (!sec || !slotLoad(&fileSlot, dir)) return;
        u32 doff; memcpy(&doff, sec + 0x14, 4);
        if (sec + 8 + doff + 32 > f + size) return;
        memcpy(sec + 8 + doff, n->shiny ? fileSlot.palS : fileSlot.palN, 32);
    } else {
        u8 *sec = nitroSection(f, size, 0x43484152u /* 'RAHC' */);
        if (!sec) return;
        u32 doff; memcpy(&doff, sec + 0x1C, 4);
        u16 *c = (u16 *)(sec + 8 + doff);
        if ((u8 *)(c + 3200) > f + size || ((uintptr_t)c & 1) || !slotLoad(&fileSlot, dir)) return;
        static u16 plain[3200]; /* 160x80 4bpp, 80 bytes a row, two 80x80 frames side by side */
        memset(plain, 0, sizeof(plain));
        u8 *pb = (u8 *)plain;
        const Slot *s = &fileSlot;
        int sc, x0, y0;
        fitBox(s->w, s->h, s->ox, s->oy, n->height, &sc, &x0, &y0);
        const int fr = s->seq[0] < s->nUniq ? s->seq[0] : 0;
        const u8 *src = s->frames + (size_t)fr * s->bw * s->h;
        const int sw = (int)s->w * sc >> 8, sh = (int)s->h * sc >> 8;
        for (int y = 0; y < sh; y++) {
            const unsigned sy = (unsigned)(y * 256 / sc);
            if (sy >= s->h || y0 + y >= 80) break;
            for (int x = 0; x < sw; x++) {
                const unsigned sx = (unsigned)(x * 256 / sc);
                if (sx >= s->w || x0 + x >= 80) break;
                const unsigned v = (src[sy * s->bw + (sx >> 1)] >> ((sx & 1) * 4)) & 15;
                if (!v) continue;
                for (int half = 0; half < 2; half++) {
                    const int px = half * 80 + x0 + x, py = y0 + y;
                    pb[py * 80 + (px >> 1)] |= (u8)(v << ((px & 1) * 4));
                }
            }
        }
        plain[0] = 0; /* UnscanPokepic always turns the first word into 0 */
        u32 seed = c[0];
        for (int i = 1; i < 3200; i++) { seed = seed * 1103515245u + 24691u; c[i] = plain[i] ^ (u16)seed; }
    }
    if (fileSubs++ < 24) PSPNativeMemLog("[B2] file %d/%d (%s) -> Black 2 species %u", narc, file, isPal ? "palette" : "sprite", n->species);
}

/* NARC_ctor knows the archive id; the NARC_* readers only get the handle. */
typedef struct { void *narc; int id; } NarcId;
static NarcId narcIds[8];
extern void *__real_NARC_ctor(int narcId, int heapID);
void *__wrap_NARC_ctor(int narcId, int heapID) {
    void *h = __real_NARC_ctor(narcId, heapID);
    if (h) { static unsigned k; narcIds[k++ & 7] = (NarcId){ h, narcId }; }
    return h;
}
static int narcIdOf(void *h) { for (int i = 0; i < 8; i++) if (narcIds[i].narc == h) return narcIds[i].id; return -1; }
extern void *__real_NARC_AllocAndReadWholeMemberByIndexPair(int narcId, int fileId, int heapID);
void *__wrap_NARC_AllocAndReadWholeMemberByIndexPair(int narcId, int fileId, int heapID) {
    void *d = __real_NARC_AllocAndReadWholeMemberByIndexPair(narcId, fileId, heapID);
    if (d) fileSubst(narcId, fileId, d, 1u << 20);
    return d;
}
extern void __real_NARC_ReadWholeMemberByIndexPair(void *dest, int narcId, int fileId);
void __wrap_NARC_ReadWholeMemberByIndexPair(void *dest, int narcId, int fileId) {
    __real_NARC_ReadWholeMemberByIndexPair(dest, narcId, fileId);
    fileSubst(narcId, fileId, dest, 1u << 20);
}
extern void *__real_NARC_AllocAndReadWholeMember(void *narc, u32 fileId, int heapID);
void *__wrap_NARC_AllocAndReadWholeMember(void *narc, u32 fileId, int heapID) {
    void *d = __real_NARC_AllocAndReadWholeMember(narc, fileId, heapID);
    const int id = narcIdOf(narc);
    if (d && id >= 0) fileSubst(id, (int)fileId, d, 1u << 20);
    return d;
}
extern void __real_NARC_ReadWholeMember(void *narc, u32 fileId, void *dest);
void __wrap_NARC_ReadWholeMember(void *narc, u32 fileId, void *dest) {
    __real_NARC_ReadWholeMember(narc, fileId, dest);
    const int id = narcIdOf(narc);
    if (id >= 0) fileSubst(id, (int)fileId, dest, 1u << 20);
}
