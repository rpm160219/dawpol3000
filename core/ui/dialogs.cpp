#include "ui/dialogs.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include "ui/elg_screen.h"
#include "ui/eva_screen.h"
#include "ui/mbira_screen.h"
#include "ui/reactor_screen.h"
#include "ui/oxbow_screen.h"
#include "ui/sid_screen.h"
#include "ui/monolit_screen.h"
#include "ui/vapora_screen.h"
#include "ui/supersaw_screen.h"
#include "ui/mixtur_screen.h"
#include "ui/brass_screen.h"
#include "ui/hollowsaint_screen.h"
#include "synth/oxbow_plus_dsp.h"
#include <stdio.h>
#include <string.h>

namespace dp {

// Wspolny uklad okna: lewa belka (szara), prawa belka (tan), pigulki na dole.
static const float LBX = 16, LBW = 372, RBX = 400, RBW = 224, BY = 12, BH = 42;
static const float PY = 430, PH = 34;

static void pills(Canvas& c, const char* l0, const char* l1, const char* l2, const char* l3) {
    pillButton(c, 16, PY, 140, PH, l0, pal::steel, 'X');
    pillButton(c, 170, PY, 140, PH, l1, pal::tan, 0);
    pillButton(c, 330, PY, 140, PH, l2, pal::pill, 'A');
    pillButton(c, 484, PY, 140, PH, l3, pal::red, 'B');
}

// ---------------- ENGINE ----------------

struct EngineInfo { const char* name; const char* sub; const char* lines[7]; };
static const int NENG = 18;
static const EngineInfo kEngines[NENG] = {
    {"WAVETABLE", "ORIGINAL 1:1",
     {"PORT OF THE OP1R WAVETABLE ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "TWO DETUNED OSCILLATORS MORPH",
      "8 WAVES FROM SINE TO DIGITAL.", "4-POLE LADDER FILTER WITH", "RESONANCE UP TO SELF-OSCILLATION.", ""}},
    {"WAVETABLE+", "EXTENDED",
     {"THE SAME CORE, OPENED UP:", "TWO INDEPENDENT OSCILLATORS -", "WAVE, INTERVAL, DETUNE, LEVEL.",
      "SUB OSCILLATOR ONE OCTAVE DOWN.", "FILTER MODES LP24 LP12 BP HP", "WITH DRIVE. LFO TARGETS PER OSC.", ""}},
    {"TAFFY", "PHASE DISTORTION",
     {"PORT OF THE OP1R TAFFY ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "CASIO CZ STYLE: A COSINE READ", "THROUGH A BENT PHASE.",
      "SAW, SQUARE, PULSE, SAW-PULSE.", "DCW = BRIGHTNESS, NO FILTER.", "CZ RESONANT WAVE ON TOP."}},
    {"PEW", "DRAWBAR ORGAN",
     {"PORT OF THE OP1R PEW ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "NINE DRAWBARS THROUGH A ROTARY", "SPEAKER: REGISTER, PERCUSSION,",
      "SCANNER CHORUS AND TUBE DRIVE,", "LESLIE STOP / SLOW / FAST.", "BARS PAGE PUSHES LOW/MID/HIGH."}},
    {"ELG", "DRONE",
     {"PORT OF THE OP1R ELG ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "THREE DETUNED SAWS PER VOICE,", "THE THIRD AN INTERVAL ABOVE.",
      "GROWL DRIVE AND A RESONANT", "LOW-PASS, SLOWLY MOVING.", "THE ENGINE ONLY, AS ORIGINAL."}},
    {"EVA", "SPACE CHOIR",
     {"PORT OF THE OP1R EVA ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "A SUNG 80S FILM PAD: SAW + PULSE,", "VOWELS OO-AH-EE, BREATH, CHORUS.",
      "THE HELMET RADIO ON TOP: BAND,", "HISS, DROPOUTS, QUINDAR BEEPS.", "L3 ON SOUND: BEEP OCTAVE."}},
    {"MBIRA", "THUMB PIANO",
     {"PORT OF THE OP1R MBIRA ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "MODAL SYNTHESIS: A PLUCKED TINE,", "FOUR DECAYING PARTIALS + CLICK,",
      "WOOD / STEEL / GLASS, A WOODEN BOX,", "BOTTLE-CAP BUZZ. L3 ON SOUND:", "RING, DOUBLE TINE, WAH."}},
    {"REACTOR", "FEEDBACK FM CORE",
     {"PORT OF THE OP1R REACTOR ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "FOUR SINE RODS IN A FEEDBACK", "LOOP, A CORE THAT HEATS UP.",
      "PAST CRITICAL: MORE DRIVE, AN", "OCTAVE-DOWN GROWL, SAGGING PITCH.", "COOLING, RADIATION, RODS, HEAT."}},
    {"OXBOW", "OB-STYLE POLY",
     {"PORT OF HUNDEKRAFT'S OXBOW, AFTER", "ZVENSON'S OBERVAVE. BIT-EXACT.", "TWO OSCILLATORS WITH SYNC AND", "CROSS-MOD, NOISE, DRIVE, A STATE",
      "VARIABLE FILTER (LP, HP, BP, NOTCH,", "LP24, SEM), FILTER ENVELOPE, VINTAGE.", "L3 ON SOUND: WAVE, FILTER, DRIVE."}},
    {"OXBOW+", "EXTENDED",
     {"THE OXBOW CORE, OPENED UP:", "TWO INDEPENDENT OSCILLATORS -", "WAVE, INTERVAL, DETUNE, LEVEL, PW.",
      "NOISE, SYNC, X-MOD, ENV TO PITCH.", "FILTER ENVELOPE WITH ADSR, KEY", "TRACK, VELOCITY, VINTAGE.", "POLY / MONO, GLIDE, UNISON, AGE."}},
    {"SID", "C64 CHIP VOICE",
     {"PORT OF THE OP1R SID ENGINE:", "CRSID 1.58 PRIMITIVES BY HERMIT,", "ADAPTED BY HUNDEKRAFT. BIT-EXACT.", "THREE OSCILLATORS PER VOICE -",
      "TRIANGLE, SAW, PULSE, NOISE,", "SYNC, RING - INTO THE 8580", "RESONANT LOW-PASS FILTER."}},
    {"MONOLIT", "LADDER BASS / LEAD",
     {"PORT OF THE OP1R MONOLIT ENGINE", "BY HUNDEKRAFT, BIT-EXACT.", "THREE SAWS + SUB INTO A FOUR-POLE", "ZERO-DELAY LADDER. MACRO KNOBS:",
      "SHAPE, CUTOFF, CONTOUR, HEAT.", "L3 ON SOUND: BODY, VEL, AGE.", "POLY / MONO, GLIDE, UNISON."}},
    {"VAPORA", "PADSYNTH PAD",
     {"PORT OF THE OP1R VAPORA ENGINE", "BY AIAAAA, BIT-EXACT.", "PADSYNTH SPECTRA, 14 CHARACTERS,", "MORPHED, WITH A SECOND VOICE",
      "AT AN INTERVAL, A SOFT FILTER", "AND SLOW MOTION. L3 ON SOUND:", "CHARACTER, INTERVAL, SUN, SPEED."}},
    {"SUPERSAW", "DETUNED SAWS",
     {"PORT OF THE OP1R SUPERSAW ENGINE", "BY PCPPCP, BIT-EXACT.", "TWO DETUNED SAWS MORPHING INTO", "A SQUARE, A SINE SUB ONE OCTAVE",
      "DOWN AND A SOFT-CLIP DRIVE.", "POLY / MONO AND GLIDE.", ""}},
    {"MIXTUR", "UNDERTONE ORGAN",
     {"PORT OF THE OP1R MIXTUR ENGINE", "BY ULTRALAZR, BIT-EXACT.", "AFTER SALA'S MIXTUR-TRAUTONIUM:", "THE KEY IS THE TOP NOTE, FOUR",
      "PHASE-LOCKED DIVIDERS SOUND F/N", "BELOW IT, TWO FORMANTS SHAPE", "VOWELS. L3 ON SOUND: FORMANTS."}},
    {"BRASSOPHONE", "WARPED SINE HORN",
     {"PORT OF THE OP1R BRASSOPHONE", "ENGINE BY NVMSOCOOL, BIT-EXACT.", "A SINE WITH A WARPED PHASE: THE", "ENVELOPE BRIGHTENS THE TIMBRE",
      "AND BENDS THE PITCH UP A LITTLE,", "VIBRATO FADES IN ON SUSTAIN.", "SHAPE IT WITH THE AMP ENV."}},
    {"HOLLOW SAINT", "HAUNTED HURDY-GURDY",
     {"PORT OF THE OP1R HOLLOW SAINT", "ENGINE (KENNEL BUILD), BIT-EXACT.", "BOWED STRING, DRONE STRINGS, A", "BUZZING BRIDGE ON TEMPO STROKES,",
      "CATHEDRAL BODY, ROOM AND BELL.", "L3 ON SOUND: DRONE KEY, RELIC", "AGE, STROKES, LO-FI RELIC TAPE."}},
    {"TEST SAW", "UTILITY",
     {"SIMPLE POLYPHONIC SAW.", "FOR TESTING PATTERNS AND TIMING.", "NO SOUND PARAMETERS.", "", "", "", ""}},
};

static int engineIndexOf(const Track& t) {
    return t.engine == ENG_SAW ? 17 : t.engine == ENG_HOLLOW ? 16 : t.engine == ENG_BRASS ? 15 : t.engine == ENG_MIXTUR ? 14 : t.engine == ENG_SUPERSAW ? 13 : t.engine == ENG_VAPORA ? 12 : t.engine == ENG_MONOLIT ? 11 : t.engine == ENG_SID ? 10 : t.engine == ENG_OXBOW ? (t.alt ? 9 : 8) : t.engine == ENG_REACTOR ? 7 : t.engine == ENG_MBIRA ? 6 : t.engine == ENG_EVA ? 5 : (t.engine == ENG_ELG ? 4 : t.engine == ENG_PEW ? 3 : (t.engine == ENG_TAFFY ? 2 : (t.alt ? 1 : 0)));
}
// Ustawia silnik ze wskazanej pozycji listy; przy zmianie silnika galki brzmienia = domyslne nowego silnika
static void applyEngine(Track& t, int idx) {
    int e = idx == 17 ? ENG_SAW : idx == 16 ? ENG_HOLLOW : idx == 15 ? ENG_BRASS : idx == 14 ? ENG_MIXTUR : idx == 13 ? ENG_SUPERSAW : idx == 12 ? ENG_VAPORA : idx == 11 ? ENG_MONOLIT : idx == 10 ? ENG_SID : idx == 8 || idx == 9 ? ENG_OXBOW : idx == 7 ? ENG_REACTOR : idx == 6 ? ENG_MBIRA : idx == 5 ? ENG_EVA : idx == 4 ? ENG_ELG : (idx == 3 ? ENG_PEW : (idx == 2 ? ENG_TAFFY : ENG_WAVETABLE));
    if (e != t.engine) {  // nowy silnik: domyslne galki i strona 2 (np. SUPERSAW: filtr otwarty)
        engineDefaultKnobs(e, t.knob);
        engineDefaultPush(e, t.push);
        t.relic = false;
        t.preset[0] = 0;
    }
    const bool wasObp = t.engine == ENG_OXBOW && t.alt;
    t.engine = (uint8_t)e;
    t.alt = idx == 1 || idx == 9;  // WAVETABLE+, OXBOW+
    if (idx == 9 && !wasObp) {  // OXBOW+: oscylatory i obwiednia filtra jak neutralny glos oryginalu
        t.obx = ObExt();
        t.fenv[0] = oxbowPlusAttackSeconds(20);
        t.fenv[1] = oxbowPlusDecaySeconds(70);
        t.fenv[2] = 40.f / 127.f;
        t.fenv[3] = oxbowPlusDecaySeconds(60);
    }
}

// ---------------- NEW TRACK ----------------

void TrackDialog::open() {
    edit_ = -1;
    engine_ = ENG_WAVETABLE;
    card_ = 0;
    engSel_ = 0;  // nowa sciezka MIDI: domyslnie WAVETABLE
    Project& p = app_->project();
    type_ = p.canAddTrack(TRACK_MIDI) ? TRACK_MIDI : TRACK_AUDIO;
    p.nextTrackName(type_, name_, sizeof name_);
    nameTouched_ = false;
}

void TrackDialog::openEdit(int track) {
    Project& p = app_->project();
    edit_ = track;
    color_ = p.tracks[track].color % pal::TRACK_COLORS;
    engine_ = p.tracks[track].engine;
    card_ = 0;
    engSel_ = engineIndexOf(p.tracks[track]);
    type_ = p.tracks[track].type;
    memcpy(name_, p.tracks[track].name, sizeof name_);
    confirmDelete_ = false;
}


void TrackDialog::updateEdit(const Buttons& b) {
    Project& p = app_->project();
    const bool midi = p.tracks[edit_].type == TRACK_MIDI;
    if (b.pressed(B_L2) && card_ > 0) card_--;
    if (b.pressed(B_R2) && card_ < 1 && midi) card_++;
    if (card_ == 1) {  // ENGINE
        if (b.repeat(B_DOWN) && engSel_ < NENG - 1) engSel_++;
        if (b.repeat(B_UP) && engSel_ > 0) engSel_--;
        if (b.pressed(B_B)) app_->showArrange();
        if (b.pressed(B_A)) {
            Track& t = p.tracks[edit_];
            memcpy(t.name, name_, sizeof t.name);
            t.color = (uint8_t)color_;
            applyEngine(t, engSel_);
            app_->markDirty();
            app_->showArrange();
        }
        return;
    }
    // siatka kolorow 4 kolumny x 8 wierszy
    const int cols = 4, n = pal::TRACK_COLORS;
    int nc = color_;
    if (b.repeat(B_RIGHT) && color_ % cols < cols - 1) nc++;
    if (b.repeat(B_LEFT) && color_ % cols > 0) nc--;
    if (b.repeat(B_DOWN) && color_ + cols < n) nc += cols;
    if (b.repeat(B_UP) && color_ - cols >= 0) nc -= cols;
    if (nc != color_) { color_ = nc; confirmDelete_ = false; }
    if (b.pressed(B_X)) ed_.begin(name_);
    if (b.pressed(B_B)) app_->showArrange();
    if (b.pressed(B_Y)) {
        if (!confirmDelete_) confirmDelete_ = true;
        else {
            p.removeTrack(edit_);
            app_->markDirty();
            app_->arrange.setCursor(edit_ < p.nTracks ? edit_ : p.nTracks, 0);
            app_->showArrange();
        }
    }
    if (b.pressed(B_A)) {
        Track& t = p.tracks[edit_];
        memcpy(t.name, name_, sizeof t.name);
        t.color = (uint8_t)color_;
        if (t.type == TRACK_MIDI) applyEngine(t, engSel_);  // wybor z karty ENGINE (bez zmiany = ten sam silnik)
        app_->markDirty();
        app_->showArrange();
    }
}

void TrackDialog::renderEdit(Canvas& c) {
    Project& p = app_->project();
    const Track& t = p.tracks[edit_];
    c.clear(pal::bg);
    if (card_ == 1) { renderEngineCard(c); return; }
    headerBar(c, LBX, BY, LBW, BH, "TRACK", pal::silver);
    headerBar(c, RBX, BY, RBW, BH, "COLOR", pal::tan);
    if (type_ == TRACK_MIDI) drawSynthIllustration(c, 24, 84, 196, 150);
    else drawTapeIllustration(c, 24, 84, 196, 150);
    nameBox(c, 236, 104, 152, 50, "NAME", name_, &ed_, pal::steel);
    Color tc = pal::track[color_];
    drawIcon(c, type_ == TRACK_MIDI ? ICON_KEYS : ICON_WAVE, 236, 186, 22, tc);
    drawText(c, 266, 191, type_ == TRACK_MIDI ? "MIDI TRACK" : "AUDIO TRACK", 11, 1.4f, tc);
    int clips = 0;
    for (int i = 0; i < p.nClips; i++) if (p.clips[i].track == edit_) clips++;
    char b[48];
    snprintf(b, sizeof b, "%d CLIPS   VOL %d", clips, (int)(t.volume * 100 + 0.5f));
    drawText(c, 236, 218, b, 10, 1.3f, pal::dim);
    if (type_ == TRACK_MIDI) {  // tylko nazwa silnika (wybor na karcie ENGINE: R2)
        drawText(c, 236, 244, "ENGINE", 8, 1.1f, pal::dim);
        drawText(c, 236, 258, kEngines[engSel_].name, 13, 1.5f, pal::white);
        drawText(c, 236, 278, "R2 = CHANGE ENGINE", 7, 1.f, rgb(80, 80, 80));
    }
    if (confirmDelete_) drawText(c, 28, 300, "PRESS Y AGAIN TO DELETE TRACK + CLIPS", 10, 1.4f, pal::red);
    else drawText(c, 28, 300, "Y = DELETE TRACK", 10, 1.3f, pal::dim);
    if (ed_.active()) drawText(c, 28, 330, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 9, 1.2f, pal::steel);
    // siatka 32 kolorow (4 x 8), wybrany w bialej obwodce
    const float lx = RBX + 6, ly = 72, lw = RBW - 12;
    c.line(lx, ly, lx, ly + 330, 2.f, pal::steel);
    c.line(lx + lw, ly, lx + lw, ly + 330, 2.f, rgb(70, 70, 70));
    const int cols = 4;
    const float cell = 46, gx = lx + (lw - cols * cell) * 0.5f, gy = ly + 10, sw = 32;
    for (int i = 0; i < pal::TRACK_COLORS; i++) {
        float cx = gx + (i % cols) * cell + (cell - sw) * 0.5f, cy = gy + (i / cols) * 39 + 2;
        c.fillRoundRect(cx, cy, sw, sw - 4, 6, pal::track[i]);
        if (i == color_) c.strokeRoundRect(cx - 4, cy - 4, sw + 8, sw + 4, 9, 2.f, pal::white);
    }
    pillButton(c, 16, PY, 140, PH, "NAME", pal::steel, 'X');
    pillButton(c, 170, PY, 140, PH, confirmDelete_ ? "SURE?" : "DELETE", pal::tan, 'Y');
    pillButton(c, 330, PY, 140, PH, "OK", pal::pill, 'A');
    pillButton(c, 484, PY, 140, PH, "CANCEL", pal::red, 'B');
    if (type_ == TRACK_MIDI) cardDots(c, 0);
}

// Kropki kart okna TRACK (TRACK, ENGINE) na dole ekranu
void TrackDialog::cardDots(Canvas& c, int card) {
    for (int i = 0; i < 2; i++) {
        float x = 314.5f + i * 12, y = 474.5f;
        if (i == card) c.fillCircle(x, y, 2.4f, pal::white);
        else c.circle(x, y, 2.f, 1.f, rgb(90, 90, 90));
    }
}

// Karta ENGINE: jedna kolumna silnikow, po prawej ilustracja i opis
void TrackDialog::renderEngineCard(Canvas& c) {
    headerBar(c, 16, BY, 236, BH, "ENGINE", pal::silver);
    headerBar(c, 264, BY, 360, BH, name_, pal::tan);
    const int cur = edit_ >= 0 ? engineIndexOf(app_->project().tracks[edit_]) : -1;  // nowa sciezka: brak
    // przewijana lista: 6 pozycji w normalnym rozmiarze, zaznaczenie trzymane w polu widzenia
    const int VIS = 6;
    int top = engSel_ - 2;
    top = top > NENG - VIS ? NENG - VIS : top;
    top = top < 0 ? 0 : top;
    for (int i = top; i < top + VIS && i < NENG; i++) {
        float y = 80 + (i - top) * 54;
        bool sel = i == engSel_;
        if (sel) c.fillRoundRect(16, y - 8, 236, 48, 8, rgb(pal::tan.r, pal::tan.g, pal::tan.b, 60));
        Color col = sel ? rgb(245, 210, 160) : pal::tan;
        drawText(c, 30, y, kEngines[i].name, 16, 1.8f, col);
        drawText(c, 30, y + 22, kEngines[i].sub, 8, 1.1f, sel ? pal::white : pal::dim);
        if (i == cur) c.fillCircle(238, y + 14, 4.f, pal::green);  // silnik zapisany na sciezce
    }
    // strzalki: jest cos powyzej / ponizej
    if (top > 0) { c.line(128, 66, 134, 60, 1.5f, pal::dim); c.line(134, 60, 140, 66, 1.5f, pal::dim); }
    if (top + VIS < NENG) { c.line(128, 400, 134, 406, 1.5f, pal::dim); c.line(134, 406, 140, 400, 1.5f, pal::dim); }
    c.line(258, 70, 258, 410, 1.5f, pal::steel);
    if (engSel_ == 4) drawElgIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 5) drawEvaIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 6) drawMbiraIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 7) drawReactorIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 8 || engSel_ == 9) drawOxbowIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 10) drawSidIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 11) drawMonolitIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 12) drawVaporaIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 13) drawSupersawIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 14) drawMixturIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 15) drawBrassIllustration(c, 280, 68, 336, 190);
    else if (engSel_ == 16) drawHollowIllustration(c, 280, 68, 336, 190);
    else drawEngineIllustration(c, engSel_, 280, 68, 336, 190);
    for (int l = 0; l < 7; l++)
        if (kEngines[engSel_].lines[l][0])
            drawText(c, 280, 276 + l * 18, kEngines[engSel_].lines[l], 9, 1.2f, l == 0 ? pal::white : rgb(170, 170, 170));
    if (engSel_ < 4) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  ADSR, MOD ENV, 2 LFO", 7, 1.f, pal::dim);
    else if (engSel_ == 4) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  AMP ENV ONLY", 7, 1.f, pal::dim);
    else if (engSel_ == 9) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  ADSR, MOD ENV, 2 LFO", 7, 1.f, pal::dim);
    else if (engSel_ == 11) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  MOD ENV, 2 LFO, VOICE", 7, 1.f, pal::dim);
    else if (engSel_ < 10) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  AMP, MOD ENV, 2 LFO", 7, 1.f, pal::dim);
    else if (engSel_ == 10 || engSel_ == 12) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  FENV, MOD ENV, 2 LFO", 7, 1.f, pal::dim);
    else if (engSel_ == 14) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  AMP, MOD ENV, LFO", 7, 1.f, pal::dim);
    else if (engSel_ == 15 || engSel_ == 16) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  AMP ENV ONLY", 7, 1.f, pal::dim);
    else if (engSel_ == 13) drawText(c, 280, 408, "6 VOICES  -  2X OVERSAMPLED  -  MOD ENV, 2 LFO, VOICE", 7, 1.f, pal::dim);
    drawText(c, 16, 414, "L2 = TRACK", 7, 1.f, rgb(80, 80, 80));
    pillButton(c, 330, PY, 140, PH, edit_ >= 0 ? "OK" : "CREATE", pal::pill, 'A');
    pillButton(c, 484, PY, 140, PH, "CANCEL", pal::red, 'B');
    cardDots(c, 1);
}

void TrackDialog::update(const Buttons& b, const InputState&, float) {
    Project& p = app_->project();
    if (ed_.active()) { ed_.update(b); return; }
    if (edit_ >= 0) { updateEdit(b); return; }
    // karty TRACK <-> ENGINE (L2/R2), ENGINE tylko dla sciezki MIDI
    if (b.pressed(B_L2) && card_ > 0) card_--;
    if (b.pressed(B_R2) && card_ < 1 && type_ == TRACK_MIDI) card_++;
    if (card_ == 1) {
        if (b.repeat(B_DOWN) && engSel_ < NENG - 1) engSel_++;
        if (b.repeat(B_UP) && engSel_ > 0) engSel_--;
    } else {
        int nt = type_;
    if (b.repeat(B_UP) || b.repeat(B_DOWN) || b.repeat(B_LEFT) || b.repeat(B_RIGHT)) nt = 1 - type_;
        if (nt != type_ && p.canAddTrack(nt)) {
            type_ = nt;
            if (!nameTouched_) p.nextTrackName(type_, name_, sizeof name_);
        }
        if (b.pressed(B_X)) { ed_.begin(name_); nameTouched_ = true; }
    }
    if (b.pressed(B_B)) app_->showArrange();
    if (b.pressed(B_A) && p.canAddTrack(type_)) {
        int t = p.addTrack(type_, name_);
        if (type_ == TRACK_MIDI) applyEngine(p.tracks[t], engSel_);  // silnik z karty ENGINE
        app_->markDirty();
        app_->arrange.setCursor(t, 0);
        if (type_ == TRACK_MIDI) app_->openNewPattern(t, 0);
        else app_->showArrange();
    }
}

void TrackDialog::render(Canvas& c) {
    if (edit_ >= 0) { renderEdit(c); return; }
    Project& p = app_->project();
    c.clear(pal::bg);
    if (card_ == 1) { renderEngineCard(c); return; }
    headerBar(c, LBX, BY, LBW, BH, "NEW TRACK", pal::silver);
    headerBar(c, RBX, BY, RBW, BH, "TYPE", pal::tan);
    if (type_ == TRACK_MIDI) drawSynthIllustration(c, 24, 84, 196, 150);
    else drawTapeIllustration(c, 24, 84, 196, 150);
    nameBox(c, 236, 104, 152, 50, "NAME", name_, &ed_, pal::steel);
    char b[48];
    snprintf(b, sizeof b, "MIDI  %d/%d", p.countTracks(TRACK_MIDI), MAX_MIDI_TRACKS);
    drawText(c, 236, 190, b, 11, 1.4f, pal::dim);
    snprintf(b, sizeof b, "AUDIO %d/%d", p.countTracks(TRACK_AUDIO), MAX_AUDIO_TRACKS);
    drawText(c, 236, 212, b, 11, 1.4f, pal::dim);
    if (type_ == TRACK_MIDI) {  // tylko nazwa silnika (wybor na karcie ENGINE: R2)
        drawText(c, 236, 244, "ENGINE", 8, 1.1f, pal::dim);
        drawText(c, 236, 258, kEngines[engSel_].name, 13, 1.5f, pal::white);
        drawText(c, 236, 278, "R2 = CHANGE ENGINE", 7, 1.f, rgb(80, 80, 80));
    }
    drawText(c, 28, 300, type_ == TRACK_MIDI ? "NOTES FROM PATTERNS -> SYNTH" : "AUDIO CLIPS FROM TAPE RECORDING", 10, 1.3f,
             pal::dim);
    if (ed_.active()) drawText(c, 28, 330, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 9, 1.2f, pal::steel);
    const char* items[2] = {"MIDI", "AUDIO"};
    bool dis[2] = {!p.canAddTrack(TRACK_MIDI), !p.canAddTrack(TRACK_AUDIO)};
    listBox(c, RBX + 6, 72, RBW - 12, 330, items, 2, type_, 0, pal::tan, dis);
    for (int r = 0; r < 2; r++) {
        float rowMid = 72 + 6 + r * 30 + 9;  // srodek wiersza listy (listBox: rowH 30, tekst 18)
        drawIcon(c, r == 0 ? ICON_KEYS : ICON_WAVE, RBX + RBW - 48, rowMid - 10, 20,
                 type_ == r ? pal::tan : rgb(90, 70, 50));
    }
    pills(c, "NAME", "TYPE", "CREATE", "CANCEL");
    if (type_ == TRACK_MIDI) cardDots(c, 0);
}

// ---------------- NEW PATTERN ----------------

void PatternDialog::open(int track, int bar) {
    Project& p = app_->project();
    track_ = track;
    bar_ = bar;
    sel_ = 0;
    first_ = 0;
    bars_ = 1;
    p.nextPatternName(name_, sizeof name_);
}

void PatternDialog::update(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    if (ed_.active()) { ed_.update(b); return; }
    int n = p.nPatterns + 1;
    if (b.repeat(B_DOWN) && sel_ < n - 1) sel_++;
    if (b.repeat(B_UP) && sel_ > 0) sel_--;
    if (sel_ < first_) first_ = sel_;
    if (sel_ >= first_ + 9) first_ = sel_ - 8;
    if (sel_ == 0) {
        int d = encLen_.steps(in.axis[AX_LX], dt, 6.f);
        if (b.repeat(B_RIGHT)) d++;
        if (b.repeat(B_LEFT)) d--;
        bars_ += d;
        bars_ = bars_ < 1 ? 1 : (bars_ > MAX_PATTERN_BARS ? MAX_PATTERN_BARS : bars_);
        if (b.pressed(B_X)) ed_.begin(name_);
    }
    if (b.pressed(B_B)) app_->showArrange();
    if (b.pressed(B_A)) {
        int pat = sel_ == 0 ? p.addPattern(name_, bars_) : sel_ - 1;
        if (pat < 0) return;
        int bar = bar_;
        if (!p.fits(track_, bar, p.patterns[pat].bars())) {  // szukaj miejsca dalej na sciezce
            while (bar < MAX_SONG_BARS && !p.fits(track_, bar, p.patterns[pat].bars())) bar++;
        }
        if (p.addClip(track_, pat, bar) >= 0) app_->arrange.setCursor(track_, bar);
        p.tracks[track_].lastPattern = pat;
        app_->markDirty();
        app_->openPattern(pat, track_);
    }
}

void PatternDialog::render(Canvas& c) {
    Project& p = app_->project();
    c.clear(pal::bg);
    headerBar(c, LBX, BY, LBW, BH, "NEW PATTERN", pal::silver);
    headerBar(c, RBX, BY, RBW, BH, "PATTERNS", pal::tan);
    drawSynthIllustration(c, 24, 84, 196, 150);

    bool isNew = sel_ == 0;
    const Pattern* ex = isNew ? nullptr : &p.patterns[sel_ - 1];
    nameBox(c, 236, 104, 152, 50, "NAME", isNew ? name_ : ex->name, isNew ? &ed_ : nullptr, pal::steel);
    char b[32];
    snprintf(b, sizeof b, "%d", isNew ? bars_ : ex->bars());
    valueBox(c, 236, 196, 84, 56, "LENGTH", b, (isNew ? bars_ : ex->bars()) == 1 ? "BAR" : "BARS", pal::tan, isNew);
    const Track& tr = p.tracks[track_];
    drawIcon(c, ICON_KEYS, 28, 290, 18, pal::track[tr.color % pal::TRACK_COLORS]);
    snprintf(b, sizeof b, "%s  -  BAR %d", tr.name, bar_ + 1);
    drawText(c, 54, 294, b, 11, 1.4f, pal::white);
    drawText(c, 28, 322, "4/4  -  16 STEPS PER BAR", 10, 1.3f, pal::dim);
    if (ed_.active()) drawText(c, 28, 350, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 9, 1.2f, pal::steel);

    static const char* items[MAX_PATTERNS + 1];
    items[0] = "+ NEW";
    for (int i = 0; i < p.nPatterns; i++) items[i + 1] = p.patterns[i].name;
    listBox(c, RBX + 6, 72, RBW - 12, 330, items, p.nPatterns + 1, sel_, first_, pal::tan);
    pills(c, "NAME", "LENGTH", isNew ? "CREATE" : "USE", "CANCEL");
}

}  // namespace dp
