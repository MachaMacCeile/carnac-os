/* SPDX-License-Identifier: GPL-3.0-only
 * Optional adapter for Carnac's live EngineState and MasterPerformancePreset types.
 * This header is NOT wired into upstream Carnac: its dependencies are supplied by
 * Carnac when copied into src/dsp alongside ritual.c / ritual.h.
 */
#ifndef RITUAL_CARNAC_ADAPTER_H
#define RITUAL_CARNAC_ADAPTER_H
#include "PresetStructures.h"
#include "ritual.h"
#include <stddef.h>

/* Upstream, as reviewed 2026-11-17. Stop the build if the preset layout changed. */
#if defined(__cplusplus)
static_assert(sizeof(MasterPerformancePreset) == 70u, "Carnac performance layout changed");
static_assert(offsetof(MasterPerformancePreset, reserved) == 54u, "Carnac reserved layout changed");
#else
_Static_assert(sizeof(MasterPerformancePreset) == 70u, "Carnac performance layout changed");
_Static_assert(offsetof(MasterPerformancePreset, reserved) == 54u, "Carnac reserved layout changed");
#endif

static inline RitualDecodeStatus ritual_from_carnac_perf(
    const MasterPerformancePreset *p, RitualState *out)
{
    return ritual_decode(p->reserved, out);
}

static inline bool ritual_into_carnac_perf(
    MasterPerformancePreset *p, const RitualState *state)
{
    return ritual_encode(p->reserved, state);
}

/* A snapshot of the fields our control overlays can affect.
 * IMPORTANT: do not assign the result back to EngineState. Feed the six effective
 * values into the appropriate existing per-voice / per-layer calculations instead.
 */
static inline RitualEffective ritual_base_from_carnac_engine(const EngineState *e)
{
    RitualEffective r;
    r.cutoff = e->filter[0].cutoff;
    r.resonance = e->filter[0].resonance;
    r.reverb_send = e->amp.reverb_send;
    r.osc_balance = e->oscmix.balance;
    r.osc2_scan = e->osc[1].scan;
    r.movement_depth = e->osc[1].scan_mod_amt;
    return r;
}

static inline RitualEffective ritual_for_carnac_engine(
    const EngineState *e, const RitualState *state)
{
    RitualEffective base = ritual_base_from_carnac_engine(e);
    RitualOffsets offsets = ritual_offsets(state);
    return ritual_effective(&base, &offsets);
}
#endif
