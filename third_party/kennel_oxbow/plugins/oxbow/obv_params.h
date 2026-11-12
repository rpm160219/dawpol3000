/* OberVave parameter definitions and the neutral voice defaults; no program bank.
 * From OberVave (github.com/zvenson/obervave a2fb3d0, firmware/src/obv_params.h and obv_patches.h).
 * Copyright (C) 2026 zvenson. Used in kennel with the author's permission (2026-10-15).
 * GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
 */
enum {
    P_O1_WAVE, P_O1_PW, P_O1_LVL, P_NOISE,
    P_O2_WAVE, P_O2_PW, P_O2_LVL, P_O2_SEMI,
    P_SYNC, P_XMOD, P_ENV_O2, P_O2_FINE,
    P_CUTOFF, P_RESO, P_FENV_AMT, P_KEYTRK, P_FTYPE,
    P_DRIVE, P_VEL_FLT, P_VEL_AMP,
    P_FA, P_FD, P_FS, P_FR,
    P_AA, P_AD, P_AS, P_AR,
    P_LFO_SHAPE, P_LFO_RATE, P_LFO_DELAY, P_LFO_PHASE,
    P_LFO_PITCH, P_LFO_PWM, P_LFO_FLT, P_LFO_AMP, P_LFO_PITCH_TO, P_LFO_PWM_TO,
    P_MODE, P_UNI_N, P_UNI_DET, P_GLIDE,
    P_VINTAGE, P_SPREAD, P_BEND, P_VOLUME,
    P_MOD_DEPTH,
    P_ARP_MODE, P_ARP_DIV, P_ARP_OCT, P_ARP_GATE,
    P_FMORPH,                                   /* (appended: the MIDI CC numbers of the above stay) */
    P_CHO_RATE, P_CHO_DEPTH, P_CHO_MIX,
    P_DLY_TIME, P_DLY_FB, P_DLY_TONE, P_DLY_MIX,
    P_REV_SIZE, P_REV_DAMP, P_REV_PRE, P_REV_MIX,
    NPARAM
};
enum { W_OFF, W_SAW, W_PLS, W_BOTH };
enum { F_LP12, F_HP12, F_BP12, F_NOTCH, F_LP24, F_SEM, NFTYPE };   /* SEM: LP .. notch .. HP by MORPH */
enum { M_POLY, M_DUAL, M_UNISON, M_MONO };
enum { L_TRI, L_SAW, L_RAMP, L_SQR, L_SH, L_RND };
enum { LP_SYNC, LP_FREE, LP_SPREAD };
enum { TO_O1, TO_O2, TO_BOTH };
enum { A_OFF, A_UP, A_DOWN, A_UPDN, A_RND, A_PLAY, NARPMODE };
enum { D_4, D_8, D_8T, D_16, D_16T, D_32, D_32T, NDIV };
enum { DD_16, DD_8T, DD_8, DD_8D, DD_4, DD_4D, DD_2, NDDIV };   /* the delay's note values */

static const uint8_t OBV_DEF[NPARAM] = {
    [P_O1_WAVE] = W_SAW,
    [P_O1_PW] = 64,
    [P_O1_LVL] = 110,
    [P_NOISE] = 0,
    [P_O2_WAVE] = W_SAW,
    [P_O2_PW] = 64,
    [P_O2_LVL] = 100,
    [P_O2_SEMI] = 24,
    [P_SYNC] = 0,
    [P_XMOD] = 0,
    [P_ENV_O2] = 64,
    [P_O2_FINE] = 70,
    [P_CUTOFF] = 80,
    [P_RESO] = 20,
    [P_FENV_AMT] = 90,
    [P_KEYTRK] = 64,
    [P_FTYPE] = F_LP24,
    [P_DRIVE] = 0,
    [P_VEL_FLT] = 40,
    [P_VEL_AMP] = 60,
    [P_FA] = 20,
    [P_FD] = 70,
    [P_FS] = 40,
    [P_FR] = 60,
    [P_AA] = 10,
    [P_AD] = 70,
    [P_AS] = 100,
    [P_AR] = 60,
    [P_LFO_SHAPE] = L_TRI,
    [P_LFO_RATE] = 70,
    [P_LFO_DELAY] = 0,
    [P_LFO_PHASE] = LP_FREE,
    [P_LFO_PITCH] = 0,
    [P_LFO_PWM] = 0,
    [P_LFO_FLT] = 0,
    [P_LFO_AMP] = 0,
    [P_LFO_PITCH_TO] = TO_BOTH,
    [P_LFO_PWM_TO] = TO_BOTH,
    [P_MODE] = M_POLY,
    [P_UNI_N] = 4,
    [P_UNI_DET] = 30,
    [P_GLIDE] = 0,
    [P_VINTAGE] = 30,
    [P_SPREAD] = 60,
    [P_BEND] = 2,
    [P_VOLUME] = 100,
    [P_MOD_DEPTH] = 0,
    [P_ARP_MODE] = A_OFF,
    [P_ARP_DIV] = D_16,
    [P_ARP_OCT] = 1,
    [P_ARP_GATE] = 64,
    [P_FMORPH] = 0,
    [P_CHO_RATE] = 40,
    [P_CHO_DEPTH] = 50,
    [P_CHO_MIX] = 0,
    [P_DLY_TIME] = DD_8D,
    [P_DLY_FB] = 50,
    [P_DLY_TONE] = 70,
    [P_DLY_MIX] = 0,
    [P_REV_SIZE] = 80,
    [P_REV_DAMP] = 60,
    [P_REV_PRE] = 20,
    [P_REV_MIX] = 0,
};

