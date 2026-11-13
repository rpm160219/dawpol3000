/* fxs.h — strona ekranu efektu op1r (core/ui/fx_screen_*.c): zrodlo efektu skompilowane drugi raz, z prawdziwymi
 * tablicami; FXS_TFIELD daje ten sam uklad telemetrii co FXW_TFIELD we wrapperze, FXS_TRESTORE wpisuje ja do [0]. */
#ifndef DP_FXS_H
#define DP_FXS_H
#include <string.h>
#define FXS_TFIELD(n) __typeof__(n[0]) n##_t;
#define FXS_TRESTORE(t, n) memcpy((void*)&n[0], &(t)->n##_t, sizeof (t)->n##_t);
#define FXS_TFIELD1(n) __typeof__(n) n##_t;
#define FXS_TRESTORE1(t, n) memcpy((void*)&n, &(t)->n##_t, sizeof (t)->n##_t);
#endif
