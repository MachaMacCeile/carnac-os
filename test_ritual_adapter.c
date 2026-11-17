/* GPL-3.0-only: integration smoke-test against Carnac's real PresetStructures.h */
#include "ritual_carnac_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    MasterPerformancePreset p;
    EngineState e;
    RitualState s, got;
    RitualEffective out;
    memset(&p, 0, sizeof(p));
    memset(&e, 0, sizeof(e));
    ritual_defaults(&s);
    assert(ritual_from_carnac_perf(&p, &got) == RITUAL_DECODE_LEGACY);
    assert(ritual_into_carnac_perf(&p, &s));
    assert(ritual_from_carnac_perf(&p, &got) == RITUAL_DECODE_OK);
    assert(memcmp(&s, &got, sizeof(s)) == 0);
    e.filter[0].cutoff = 70;
    e.oscmix.balance = 64;
    e.osc[1].scan = 64;
    out = ritual_for_carnac_engine(&e, &s);
    assert(out.cutoff <= 127 && out.osc_balance <= 127);
    assert(e.filter[0].cutoff == 70 && e.oscmix.balance == 64);
    puts("PASS: Carnac struct compatibility, saved-state roundtrip, non-destructive overlay");
    return 0;
}
