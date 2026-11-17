/* SPDX-License-Identifier: GPL-3.0-only */
#include "ritual.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define CK(x) do { assert(x); ++checks; } while (0)
static unsigned checks = 0;

static void defaults_and_bounds(void)
{
    RitualState s;
    ritual_defaults(&s);
    CK(s.tir == 0 && s.veil == 32 && s.cylch == 16 && s.gealach == 0 && s.awen == 64);
    CK(!ritual_set(&s, (RitualParam)255, 10));
    CK(!ritual_set(&s, RITUAL_AWEN, 255));
    CK(!ritual_set(&s, RITUAL_AWEN, 64));
    CK(ritual_set(&s, RITUAL_AWEN, 127) && s.awen == 127);
    CK(strcmp(ritual_festival_name(0), "Samhain") == 0);
    CK(strcmp(ritual_festival_name(16), "Alban Arthan") == 0);
    CK(strcmp(ritual_lunar_name(64), "Full") == 0);
}
static void interpolation(void)
{
    const int16_t a[8] = {0,16,32,48,64,80,96,112};
    for (unsigned i = 0; i < 8; i++) CK(ritual_ring8(a, (uint8_t)(i * 16)) == a[i]);
    CK(ritual_ring8(a, 8) == 8);
    CK(ritual_ring8(a, 120) == 56); /* ring wrap midpoint, 112 to 0 */
    CK(ritual_ring8(a, 127) == 7);
    CK(ritual_ring8(a, 0) == 0);
    const int16_t b[8] = {-16,-8,0,8,16,8,0,-8};
    CK(ritual_ring8(b, 8) == -12);
    CK(ritual_ring8(b, 120) == -12);
}
static void preset_roundtrip(void)
{
    RitualState s, got;
    uint8_t reserved[16] = {0};
    ritual_defaults(&s);
    CK(ritual_decode(reserved, &got) == RITUAL_DECODE_LEGACY && got.cylch == 16);
    for (unsigned i = 8; i < 16; i++) reserved[i] = 0xEE; /* preserve future bytes */
    CK(ritual_encode(reserved, &s));
    for (unsigned i = 8; i < 16; i++) CK(reserved[i] == 0xEE);
    CK(ritual_decode(reserved, &got) == RITUAL_DECODE_OK);
    CK(memcmp(&s, &got, sizeof(s)) == 0);
    s.veil = s.cylch = s.gealach = s.awen = 0;
    CK(ritual_encode(reserved, &s));
    CK(ritual_decode(reserved, &got) == RITUAL_DECODE_OK);
    CK(got.veil == 0 && got.cylch == 0 && got.gealach == 0 && got.awen == 0);
    reserved[0] = 2; CK(!ritual_encode(reserved, &s));
    CK(ritual_decode(reserved, &got) == RITUAL_DECODE_UNSUPPORTED && got.awen == 64);
    reserved[0] = 1; reserved[6] = 1;
    CK(ritual_decode(reserved, &got) == RITUAL_DECODE_INVALID);
    reserved[6] = 0; reserved[5] = 255;
    CK(ritual_decode(reserved, &got) == RITUAL_DECODE_INVALID);
}
static void midi(void)
{
    RitualState s;
    ritual_defaults(&s);
    CK(!ritual_midi_cc(&s, 0x90, 20, 90, true, 0, 0));
    CK(!ritual_midi_cc(&s, 0xB0, 20, 90, false, 0, 0));
    CK(!ritual_midi_cc(&s, 0xB0, 20, 90, true, 20, 0)); /* collision */
    CK(!ritual_midi_cc(&s, 0xB0, 21, 90, true, 0, 21));
    CK(ritual_midi_cc(&s, 0xB1, 20, 90, true, 0, 0) && s.veil == 90);
    CK(ritual_midi_cc(&s, 0xB1, 21, 127, true, 0, 0) && s.cylch == 127);
    CK(ritual_midi_cc(&s, 0xB1, 22, 12, true, 0, 0) && s.gealach == 12);
    CK(ritual_midi_cc(&s, 0xB1, 23, 0, true, 0, 0) && s.awen == 0);
    CK(!ritual_midi_cc(&s, 0xB1, 23, 0, true, 0, 0));
}
static void effective_and_smoothing(void)
{
    const RitualEffective base = {64,60,60,64,64,30};
    RitualState s; ritual_defaults(&s);
    for (unsigned p = 0; p < 128u; p++) {
        s.cylch = s.gealach = (uint8_t)p;
        s.veil = (uint8_t)(127u-p);
        s.awen = (uint8_t)p;
        RitualOffsets o = ritual_offsets(&s);
        RitualEffective eff = ritual_effective(&base, &o);
        CK(eff.cutoff <= 127 && eff.resonance <= 127 && eff.reverb_send <= 127);
        CK(eff.osc_balance <= 127 && eff.osc2_scan <= 127 && eff.movement_depth <= 127);
        /* Underlying patch must remain bit-identical. */
        CK(base.cutoff == 64 && base.osc_balance == 64 && base.osc2_scan == 64);
    }
    RitualSmoother sm;
    RitualEffective a = {0,0,0,0,0,0}, b = {127,127,127,127,127,127};
    ritual_smooth_reset(&sm, &a);
    CK(ritual_smooth_read(&sm).cutoff == 0);
    RitualEffective prior = ritual_smooth_read(&sm);
    for (int tick = 0; tick < 220; tick++) {
        ritual_smooth_tick(&sm, &b);
        RitualEffective curr = ritual_smooth_read(&sm);
        CK(curr.cutoff >= prior.cutoff && curr.cutoff <= 127);
        CK(curr.osc_balance >= prior.osc_balance && curr.osc_balance <= 127);
        prior = curr;
    }
    CK(ritual_smooth_read(&sm).cutoff == 127);
    ritual_smooth_reset(&sm, &b);
    CK(ritual_smooth_read(&sm).cutoff == 127);
    for (int tick = 0; tick < 220; tick++) ritual_smooth_tick(&sm, &a);
    CK(ritual_smooth_read(&sm).cutoff == 0);
}
int main(void)
{
    defaults_and_bounds(); interpolation(); preset_roundtrip(); midi(); effective_and_smoothing();
    printf("PASS: %u assertions; defaults, ring/wrap, save/recall, MIDI collisions, bounds and smoothing\n", checks);
    return 0;
}
