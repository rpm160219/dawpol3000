/* Referencja GIN: oryginalny gin_fw.c (kennel, OP1_HOST) z adapterem i slownikiem wspolczynnikow fabrycznych z
 * build_factory_data.py (tak powstaja dane wgrywane na OP-1). Zapis: int32 suma glosow na probke (88,2 kHz). */
#include <stdio.h>
#include <string.h>
#include "gin_fw.h"
#include "gin_events.h"

#include "gin_factory_data.h"   /* wygenerowane: gin_factory_prepared_dictionary / _index */

const gin_preset_owned_t gin_fw_builtin_kit;
const gin_prepared_t gin_fw_builtin_prepared[GIN_PRESET_MAX_SOUNDS];
const uint32_t gin_fw_builtin_patch[GIN_PRESET_MAX_SOUNDS][21];
static struct { uint32_t vptr, pad[17]; } rec[12];
static void* pub[12];
static int16_t bases[8][24 * 8], gain[128];
uint32_t gin_fw_stock_type(const gin_fw_string_t* s) { (void)s; return 0; }
uint32_t gin_fw_stock_factory(uint32_t t, void** p, uint32_t s) { (void)t; (void)p; (void)s; return 0; }
uint32_t gin_fw_native_core(void) { return 0; }
uint32_t gin_fw_native_selected(void) { return 0; }
const int16_t* gin_fw_native_base(uint32_t s) { return bases[s]; }
int gin_fw_native_receiver(uint32_t i, const void* p) { return p == &rec[i]; }
const int16_t* gin_fw_native_gain(void) { return gain; }
static uint32_t st_t(void* s, const int16_t* p) { (void)s; (void)p; return 0; }
static uint32_t st_r(void* s, int16_t* o, const int16_t* p) { (void)s; (void)o; (void)p; return 0; }
static uint32_t st_s(const void* s) { (void)s; return 0; }
static uint32_t st_l(void* s, const int16_t* p) { (void)s; (void)p; return 0; }
const op1_drum_engine_t gin_fw_stock_methods = {st_t, st_r, st_s, st_l};
uint32_t gin_fw_stock_database_open(void) { return 0; }
uint32_t gin_fw_database_exec(const char* q, int (*cb)(void*, int, char**, char**), void* a) { (void)q; (void)cb; (void)a; return 0; }
void** gin_fw_native_publication(void) { return pub; }
int instr_name_eq(const char* a, const gin_fw_string_t* b) { (void)a; (void)b; return 0; }
uint32_t op1_drum_native_status(const void* p) { (void)p; return 0; }
const op1_drum_engine_t* op1_drum_find(const char* n) { return n && !strcmp(n, "gin") ? &DRUM_ENGINE_gin : 0; }

int main(int argc, char** argv) {
    FILE* f = fopen(argc > 1 ? argv[1] : "ref.raw", "wb");
    for (int i = 0; i < 12; i++) { rec[i].vptr = 0x0190f8a4u; pub[i] = &rec[i]; }
    for (int i = 0; i < 128; i++) gain[i] = 16384;
    gin_fw_string_t g;
    memset(&g, 0, sizeof g);
    g.size = 3; g.cap = 15; memcpy(g.u.buf, "gin", 3);
    gin_fw_database_open();
    for (int kit = 0; kit < GE_KITS; kit++) {
        for (int k = 0; k < 24; k++) bases[0][8 * k + 7] = (int16_t)(4096 + kit);
        for (int sl = 0; sl < 8; sl++) gin_fw_activation_type(&g, (uint32_t)sl);  /* wszystkie sloty GIN: bez wbudowanego zestawu */
        if (gin_fw_factory(9, pub, 0) || gin_fw_fault) { printf("factory fault %x\n", gin_fw_fault); return 1; }
        unsigned char fresh[12] = {0};
        for (int b = 0; b < GE_BLOCKS; b++) {
            for (int w = 0; w < 2; w++) {
                const int key = ge_key(kit, b, w);
                if (key < 0) continue;
                int v = -1;
                for (int i = 0; i < 12 && v < 0; i++) {
                    const gin_sdk_binding_t* x = &gin_fw_runtime.binding[i];
                    if (!fresh[i] && !x->owned.active && !x->tail_left && !x->resample_left) v = i;
                }
                if (v < 0) {
                    uint32_t best = 0;
                    for (int i = 0; i < 12; i++)
                        if (!fresh[i] && (v < 0 || gin_fw_runtime.binding[i].block_age > best)) { best = gin_fw_runtime.binding[i].block_age; v = i; }
                }
                if (v < 0) v = 0;
                if (!gin_fw_trigger(&rec[v], &bases[0][8 * key])) fresh[v] = 1;
                else printf("trigger rejected kit %d block %d\n", kit, b);
            }
            int32_t mix[128] = {0};
            int16_t out[128];
            for (int v = 0; v < 12; v++) {
                const gin_sdk_binding_t* x = &gin_fw_runtime.binding[v];
                fresh[v] = 0;
                if (!x->owned.active && !x->tail_left && !x->resample_left) continue;
                if (gin_fw_render(&rec[v], out, bases[0])) continue;
                for (int i = 0; i < 128; i++) mix[i] += out[i];
            }
            fwrite(mix, sizeof mix, 1, f);
        }
    }
    fclose(f);
    return 0;
}
