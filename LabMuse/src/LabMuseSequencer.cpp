//
//  LabMuseSequencer.cpp
//
//  Musical-time sequencer: absolute-tick event list with a cursor, a single
//  loop region, tempo x speed timing, and jump/scan seeking.  This is the
//  counterpart to LabMidi's MidiSongPlayer, which cannot loop, jump, scan,
//  change speed, or pause -- all of which iMUSE requires.
//
/*
┌──────────────────────────────────────────────────────────────────────────┐
│ ▓▓▓▒▒▒ PROVENANCE NODE [4] // ZONE: [HYBRID_REMIXABLE]                   │
├──────────────────────────────────────────────────────────────────────────┤
│ AUTH_ID  : [Nick Porcino]         | GEN_TOOL : [Claude Opus 4.8]         │
│ SRC_AUTH : [HMN]                  │ SIG_STAT : [UNSIGNED]                │
| INCEPT © : [Tues July 14 08:58:06 2026 -0800]                            |
├──────────────────────────────────────────────────────────────────────────┤
│ INDEX_ALLOW : [YES]               │ CORP_TRAIN: [NO]                     │
│ DERIV_ALLOW : [YES]               │ GOV_SPDX  : [SPDX: BSD 2-Clause]     │
└──────────────────────────────────────────────────────────────────────────┘
*/

#include "LabMuseInternal.h"

#include <algorithm>
#include <cmath>

namespace Lab {
namespace Muse {

namespace {
    // Stable ordering for events sharing a tick: config first (so program /
    // control state is correct before any note sounds), then markers, then
    // note-offs before note-ons (so a re-attack at a boundary doesn't get cut).
    int classRank(const MuseEvent& e)
    {
        switch (e.cls) {
        case EvClass::Config: return 0;
        case EvClass::Marker: return 1;
        case EvClass::Note:   return (e.status & 0xF0) == 0x80 || e.d2 == 0 ? 2 : 3;
        }
        return 4;
    }
}

void Sequencer::addEvent(const MuseEvent& e)
{
    events.push_back(e);
}

void Sequencer::sortEvents()
{
    std::stable_sort(events.begin(), events.end(),
        [](const MuseEvent& a, const MuseEvent& b) {
            if (a.tick != b.tick)
                return a.tick < b.tick;
            return classRank(a) < classRank(b);
        });
}

void Sequencer::reset()
{
    posTicks = looping ? double(loopStart) : 0.0;
    cursor   = firstEventAtOrAfter(int(std::llround(posTicks)));
    // Rewind to the very start for non-looping sounds so tick-0 config emits.
    if (!looping) {
        posTicks = 0.0;
        cursor   = 0;
    }
}

double Sequencer::ticksPerSecond() const
{
    // BPM is quarter-notes per minute; ticksPerQuarter ticks per quarter.
    // speed 128 == as composed, linear scaling around that.
    double base = (tempoBPM / 60.0) * double(ticksPerQuarter);
    return base * (double(speed) / 128.0);
}

size_t Sequencer::firstEventAtOrAfter(int tick) const
{
    size_t i = 0;
    while (i < events.size() && events[i].tick < tick)
        ++i;
    return i;
}

void Sequencer::advance(double dtSeconds)
{
    if (!active || dtSeconds <= 0.0)
        return;

    double remaining = dtSeconds * ticksPerSecond();

    // A guard against pathological loops (e.g. an empty/zero-length loop).
    int wrapGuard = 0;

    while (remaining > 1e-9) {
        double target = posTicks + remaining;
        double segEnd = target;
        bool   willWrap = false;

        if (looping && loopEnd > loopStart && target >= double(loopEnd)) {
            segEnd   = double(loopEnd);
            willWrap = true;
        }

        while (cursor < events.size() &&
               double(events[cursor].tick) <= segEnd + 1e-9) {
            if (onEvent)
                onEvent(events[cursor]);
            ++cursor;
        }

        if (willWrap) {
            double over = target - double(loopEnd);
            if (onLoopWrap)
                onLoopWrap();
            posTicks  = double(loopStart);
            cursor    = firstEventAtOrAfter(loopStart);
            remaining = over;
            if (++wrapGuard > 64)   // don't spin forever on a degenerate loop
                break;
        }
        else {
            posTicks  = target;
            remaining = 0.0;
        }
    }

    if (!looping && cursor >= events.size())
        active = false;   // reached the end; engine flushes held notes
}

void Sequencer::seekTo(int targetTick, bool emitConfig)
{
    if (targetTick < 0)
        targetTick = 0;

    if (double(targetTick) >= posTicks) {
        // Forward: optionally re-emit config between here and the target.
        while (cursor < events.size() && events[cursor].tick <= targetTick) {
            if (emitConfig && events[cursor].cls == EvClass::Config && onEvent)
                onEvent(events[cursor]);
            ++cursor;
        }
    }
    else {
        // Backward: rewind the cursor, then scan forward emitting config.
        cursor = 0;
        while (cursor < events.size() && events[cursor].tick <= targetTick) {
            if (emitConfig && events[cursor].cls == EvClass::Config && onEvent)
                onEvent(events[cursor]);
            ++cursor;
        }
    }
    posTicks = double(targetTick);
}

void Sequencer::musicalPosition(int& bar, int& beat, int& tick) const
{
    int t = int(std::floor(posTicks));
    if (t < 0) t = 0;
    int bt = barTicks > 0 ? barTicks : 1;
    int within = t % bt;
    bar  = t / bt + 1;
    beat = beatTicks > 0 ? within / beatTicks + 1 : 1;
    tick = beatTicks > 0 ? within % beatTicks : within;
}

} // namespace Muse
} // namespace Lab
