/*
 * TIR / CYLCH / GEALACH / AWEN - portable performance control core, v0.1
 * SPDX-License-Identifier: GPL-3.0-only
 * Original implementation for a proposed Carnac OS derivative.
 * No Carnac upstream files are included in this distribution.
 */
#ifndef RITUAL_H
#define RITUAL_H
#include <stdint.h>
#include <stdbool.h>

#define RITUAL_RESERVED_BYTES 16u
#define RITUAL_LAYOUT_VERSION 1u
#define RITUAL_TIR_MEN_AN_TOL 0u
#define RITUAL_CC_VEIL 20u
#define RITUAL_CC_CYLCH 21u
#define RITUAL_CC_GEALACH 22u
#define RITUAL_CC_AWEN 23u

typedef enum {
    RITUAL_VEIL = 0, RITUAL_CYLCH = 1, RITUAL_GEALACH = 2, RITUAL_AWEN = 3,
    RITUAL_PARAM_COUNT = 4
} RitualParam;

typedef struct {
    uint8_t tir;
    uint8_t veil;
    uint8_t cylch;
    uint8_t gealach;
    uint8_t awen;
    uint8_t phase_mode; /* 0 = manual; no clocked or astronomical operation in v0.1 */
} RitualState;

typedef struct {
    int16_t filter_cutoff;
    int16_t filter_resonance;
    int16_t reverb_send;
    int16_t osc2_balance;
    int16_t osc2_scan;
    int16_t movement_depth;
} RitualOffsets;

/* Non-persistent target parameters in the same 0..127 domains as Carnac's base patch fields.
 * Not an EngineState substitute: Carnac adapts these to its live DSP effective parameters. */
typedef struct {
    uint8_t cutoff;
    uint8_t resonance;
    uint8_t reverb_send;
    uint8_t osc_balance;
    uint8_t osc2_scan;
    uint8_t movement_depth;
} RitualEffective;

typedef enum {
    RITUAL_DECODE_LEGACY = 0,
    RITUAL_DECODE_OK = 1,
    RITUAL_DECODE_UNSUPPORTED = 2,
    RITUAL_DECODE_INVALID = 3
} RitualDecodeStatus;

void ritual_defaults(RitualState *s);
bool ritual_set(RitualState *s, RitualParam which, uint8_t value);
int16_t ritual_ring8(const int16_t anchors[8], uint8_t position);
RitualOffsets ritual_offsets(const RitualState *s);
RitualEffective ritual_effective(const RitualEffective *base, const RitualOffsets *o);
/* Safely reads the 16-byte performance reserved field. For legacy or invalid/unsupported
 * versions, returns defaults. Never overwrites unknown future extensions. */
RitualDecodeStatus ritual_decode(const uint8_t reserved[RITUAL_RESERVED_BYTES], RitualState *out);
bool ritual_encode(uint8_t reserved[RITUAL_RESERVED_BYTES], const RitualState *s);
/* Routes proposed fixed CC20..23 only if channel_routed is true, and refuses collisions
 * with currently configured Carnac custom CC1/2. Call before or after native MIDI parser;
 * never steal a custom CC. Return true only when a ritual parameter changed. */
bool ritual_midi_cc(RitualState *s, uint8_t status, uint8_t cc, uint8_t value,
                    bool channel_routed, uint8_t custom_cc1, uint8_t custom_cc2);

/* An integer low-pass smoother for bounded effective targets (Q8 fractional units).
 * `tick` at the existing voice/layer control rate, not once per audio sample.
 * Initial load/reset snaps to a target, subsequent ticks approach it without stalling.
 */
typedef struct { int32_t q8[6]; } RitualSmoother;
void ritual_smooth_reset(RitualSmoother *sm, const RitualEffective *target);
void ritual_smooth_tick(RitualSmoother *sm, const RitualEffective *target);
RitualEffective ritual_smooth_read(const RitualSmoother *sm);

const char *ritual_festival_name(uint8_t position);
const char *ritual_lunar_name(uint8_t position);
#endif
