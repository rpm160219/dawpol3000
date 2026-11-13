#include "fx/fx_registry.h"

namespace dp {

static const FxInfo kFx[FX_COUNT] = {
    {"NONE", "-", {"NO EFFECT.", "", "", ""}, {"", "", "", ""}, {"", "", ""}, {0, 0, 0, 0, 0, 0, 0, 0}},
    {"JUNI CHORUS", "CHORUS",
     {"PORT OF THE OP1R JUNI CHORUS BY", "HUNDEKRAFT, BIT-EXACT. EARLY-80S", "BBD ENSEMBLE: MODES I, II, I+II,", "DEPTH, DARKNESS OF THE WET, MIX."},
     {"MODE", "DEPTH", "TONE", "MIX"}, {"", "", ""}, {4608, 27852, 6000, 13239, 0, 0, 0, 0}},
    {"SISTER", "SISTER",
     {"PORT OF THE OP1R SISTER BY HUNDEKRAFT,", "BIT-EXACT. A CATHEDRAL REVERB WHOSE", "TAIL SINGS (CHOIR), RINGS IN TUNE", "(PIPES) OR SWELLS BACKWARDS."},
     {"SPACE", "VOICE", "SWELL", "MIX"}, {"", "KEY", ""}, {19661, 6554, 0, 6620, 0, 0, 16384, 0}},
};

const FxInfo& fxInfo(int fx) { return kFx[fx >= 0 && fx < FX_COUNT ? fx : 0]; }

void fxDefaults(FxSlot& s, int fx) {
    s.fx = (uint8_t)(fx >= 0 && fx < FX_COUNT ? fx : FX_NONE);
    for (int i = 0; i < 8; i++) s.w[i] = kFx[s.fx].def[i];
}

}  // namespace dp
