/* SPDX-License-Identifier: GPL-3.0-only */
#include "ritual.h"
#include <stddef.h>

static const int16_t season_cutoff[8] = {-14,-18,0,6,12,18,5,-7};
static const int16_t season_res[8]    = {  8, -2,-6,-4,2,4,6, 0};
static const int16_t season_reverb[8] = { 14,  4,-8,-4,3,8,2, 6};
static const int16_t moon_mix[8]      = {-24,-18,-10,-3,12,5,-8,-18};
static const int16_t moon_scan[8]     = {-12, -9, -5, 2,12,8,2, -6};
static const int16_t moon_motion[8]   = {  0,  2,  4, 8,12,9,5,  2};
static const char *const festivals[8] = {
    "Samhain", "Alban Arthan", "Imbolc", "Alban Eilir", "Beltane",
    "Alban Hefin", "Lunasa", "Alban Elfed"
};
static const char *const moons[8] = {
    "New", "Waxing Crescent", "First Quarter", "Waxing Gibbous",
    "Full", "Waning Gibbous", "Last Quarter", "Waning Crescent"
};

static uint8_t clamp_u7(int32_t v)
{
    return (uint8_t)(v < 0 ? 0 : (v > 127 ? 127 : v));
}

void ritual_defaults(RitualState *s)
{
    if (s == NULL) return;
    s->tir = RITUAL_TIR_MEN_AN_TOL;
    s->veil = 32;
    s->cylch = 16;
    s->gealach = 0;
    s->awen = 64;
    s->phase_mode = 0;
}

bool ritual_set(RitualState *s, RitualParam p, uint8_t value)
{
    uint8_t *field = NULL;
    if (s == NULL || value > 127u) return false;
    switch (p) {
    case RITUAL_VEIL: field = &s->veil; break;
    case RITUAL_CYLCH: field = &s->cylch; break;
    case RITUAL_GEALACH: field = &s->gealach; break;
    case RITUAL_AWEN: field = &s->awen; break;
    default: return false;
    }
    if (*field == value) return false;
    *field = value;
    return true;
}

/* Divide by 16 with a symmetric nearest-integer rounding. Values remain between the
 * two adjacent anchors (including the 7->0 wrap), so transitions have no DSP jumps. */
int16_t ritual_ring8(const int16_t anchors[8], uint8_t position)
{
    int32_t i, fraction, numerator;
    if (anchors == NULL) return 0;
    position &= 127u; /* protect the lookup even if a caller hands us an invalid 8-bit value */
    i = position >> 4;
    fraction = position & 15;
    numerator = (16 - fraction) * (int32_t)anchors[i] +
                fraction * (int32_t)anchors[(i + 1) & 7];
    return (int16_t)(numerator < 0 ? -((-numerator + 8) / 16) : (numerator + 8) / 16);
}

RitualOffsets ritual_offsets(const RitualState *s)
{
    RitualOffsets o = {0,0,0,0,0,0};
    if (s == NULL) return o;
    const int32_t veil = s->veil > 127u ? 127 : s->veil;
    const int32_t awen = s->awen > 127u ? 127 : s->awen;
    o.filter_cutoff = (int16_t)(ritual_ring8(season_cutoff, s->cylch) + ((awen - 64) * 20) / 64);
    o.filter_resonance = (int16_t)(ritual_ring8(season_res, s->cylch) + veil * 12 / 127);
    o.reverb_send = (int16_t)(ritual_ring8(season_reverb, s->cylch) + veil * 8 / 127);
    o.osc2_balance = (int16_t)(ritual_ring8(moon_mix, s->gealach) + ((veil - 32) * 32) / 95);
    o.osc2_scan = (int16_t)(ritual_ring8(moon_scan, s->gealach) + veil * 10 / 127);
    o.movement_depth = ritual_ring8(moon_motion, s->gealach);
    return o;
}

RitualEffective ritual_effective(const RitualEffective *base, const RitualOffsets *o)
{
    RitualEffective out = {0,0,0,0,0,0};
    if (base == NULL || o == NULL) return out;
    out.cutoff = clamp_u7((int32_t)base->cutoff + o->filter_cutoff);
    out.resonance = clamp_u7((int32_t)base->resonance + o->filter_resonance);
    out.reverb_send = clamp_u7((int32_t)base->reverb_send + o->reverb_send);
    out.osc_balance = clamp_u7((int32_t)base->osc_balance + o->osc2_balance);
    out.osc2_scan = clamp_u7((int32_t)base->osc2_scan + o->osc2_scan);
    out.movement_depth = clamp_u7((int32_t)base->movement_depth + o->movement_depth);
    return out;
}

RitualDecodeStatus ritual_decode(const uint8_t r[RITUAL_RESERVED_BYTES], RitualState *out)
{
    if (out == NULL || r == NULL) return RITUAL_DECODE_INVALID;
    ritual_defaults(out);
    if (r[0] == 0u) return RITUAL_DECODE_LEGACY;
    if (r[0] != RITUAL_LAYOUT_VERSION) return RITUAL_DECODE_UNSUPPORTED;
    if (r[1] != RITUAL_TIR_MEN_AN_TOL || r[2] > 127u || r[3] > 127u ||
        r[4] > 127u || r[5] > 127u || r[6] != 0u || r[7] != 0u)
        return RITUAL_DECODE_INVALID;
    out->tir = r[1];
    out->veil = r[2];
    out->cylch = r[3];
    out->gealach = r[4];
    out->awen = r[5];
    out->phase_mode = r[6];
    return RITUAL_DECODE_OK;
}

bool ritual_encode(uint8_t r[RITUAL_RESERVED_BYTES], const RitualState *s)
{
    if (r == NULL || s == NULL ||
        s->tir != RITUAL_TIR_MEN_AN_TOL || s->phase_mode != 0u ||
        s->veil > 127u || s->cylch > 127u || s->gealach > 127u || s->awen > 127u ||
        (r[0] != 0u && r[0] != RITUAL_LAYOUT_VERSION)) return false;
    r[0] = RITUAL_LAYOUT_VERSION;
    r[1] = s->tir;
    r[2] = s->veil;
    r[3] = s->cylch;
    r[4] = s->gealach;
    r[5] = s->awen;
    r[6] = s->phase_mode;
    r[7] = 0;
    return true;
}

bool ritual_midi_cc(RitualState *s, uint8_t status, uint8_t cc, uint8_t value,
                    bool channel_routed, uint8_t custom_cc1, uint8_t custom_cc2)
{
    if (s == NULL || !channel_routed || (status & 0xF0u) != 0xB0u ||
        value > 127u || (cc != RITUAL_CC_VEIL && cc != RITUAL_CC_CYLCH &&
        cc != RITUAL_CC_GEALACH && cc != RITUAL_CC_AWEN)) return false;
    if ((custom_cc1 != 0u && cc == custom_cc1) ||
        (custom_cc2 != 0u && cc == custom_cc2)) return false;
    return ritual_set(s, (RitualParam)(cc - RITUAL_CC_VEIL), value);
}

/* Convert effective controls to Q8; Carnac's own per-knob glide is distinct.
 * The caller must use one consistent control-tick cadence. */
static void eff_to_array(const RitualEffective *e, uint8_t a[6])
{
    a[0] = e->cutoff; a[1] = e->resonance; a[2] = e->reverb_send;
    a[3] = e->osc_balance; a[4] = e->osc2_scan; a[5] = e->movement_depth;
}

void ritual_smooth_reset(RitualSmoother *sm, const RitualEffective *target)
{
    uint8_t a[6];
    if (sm == NULL || target == NULL) return;
    eff_to_array(target, a);
    for (unsigned i = 0; i < 6u; i++) sm->q8[i] = (int32_t)a[i] << 8;
}

void ritual_smooth_tick(RitualSmoother *sm, const RitualEffective *target)
{
    uint8_t a[6];
    if (sm == NULL || target == NULL) return;
    eff_to_array(target, a);
    for (unsigned i = 0; i < 6u; i++) {
        int32_t delta = ((int32_t)a[i] << 8) - sm->q8[i];
        int32_t change = delta / 16;
        if (delta != 0 && change == 0) change = delta > 0 ? 1 : -1;
        sm->q8[i] += change;
    }
}

RitualEffective ritual_smooth_read(const RitualSmoother *sm)
{
    RitualEffective out = {0,0,0,0,0,0};
    uint8_t *values[6] = {&out.cutoff, &out.resonance, &out.reverb_send,
                          &out.osc_balance, &out.osc2_scan, &out.movement_depth};
    if (sm == NULL) return out;
    for (unsigned i = 0; i < 6u; i++) *values[i] = clamp_u7((sm->q8[i] + 128) / 256);
    return out;
}

const char *ritual_festival_name(uint8_t position) { return festivals[(position >> 4) & 7u]; }
const char *ritual_lunar_name(uint8_t position) { return moons[(position >> 4) & 7u]; }
