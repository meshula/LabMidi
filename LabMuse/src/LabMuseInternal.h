//
//  LabMuseInternal.h
//
//  Shared internal types for the LabMuse engine and sequencer.  Not part of the
//  public API.
//

/*
┌──────────────────────────────────────────────────────────────────────────┐
│ ▓▓▓▒▒▒ PROVENANCE NODE [3] // ZONE: [HYBRID_REMIXABLE]                   │
├──────────────────────────────────────────────────────────────────────────┤
│ AUTH_ID  : [Nick Porcino]         | GEN_TOOL : [Claude Opus 4.8]         │
│ SRC_AUTH : [HMN]                  │ SIG_STAT : [UNSIGNED]                │
| INCEPT © : [Tues July 14 08:58:06 2026 -0800]                            |
├──────────────────────────────────────────────────────────────────────────┤
│ INDEX_ALLOW : [YES]               │ CORP_TRAIN: [NO]                     │
│ DERIV_ALLOW : [YES]               │ GOV_SPDX  : [SPDX: BSD 2-Clause]     │
└──────────────────────────────────────────────────────────────────────────┘
*/
 
#pragma once

#include "LabMidi/MidiInOut.h"

#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace Lab {
namespace Muse {

//----------------------------------------------------------------------------
// A single scored event, tagged with a class so the sequencer can treat notes,
// configuration (program change / control change / pitch bend), and markers
// differently during jump/scan/loop-wrap.
//----------------------------------------------------------------------------

enum class EvClass : uint8_t {
    Note,     // note-on (d2 > 0) or note-off (status 0x80 or d2 == 0)
    Config,   // program change, control change, pitch bend
    Marker    // musical anchor; carries markerId
};

struct MuseEvent {
    int      tick   = 0;      // absolute tick within the sound
    EvClass  cls    = EvClass::Note;
    uint8_t  status = 0;      // full MIDI status byte (command | channel)
    uint8_t  d1     = 0;
    uint8_t  d2     = 0;
    int      channel  = 0;
    int      markerId = 0;
};

//----------------------------------------------------------------------------
// The musical-time sequencer.  Holds an absolute-tick event list and a cursor.
// It advances by a wall-clock delta converted to ticks via tempo x speed,
// handles a single loop region, and calls back for each emitted event.  All
// note-tracking / synth routing lives in the engine via the onEvent callback.
//----------------------------------------------------------------------------

class Sequencer {
public:
    std::vector<MuseEvent> events;

    // Musical grid.
    int    ticksPerQuarter = 240;   // tempo reference
    int    beatTicks       = 120;   // one displayed beat (eighth for 5/8)
    int    meter           = 5;     // beats per bar
    int    barTicks        = 600;   // meter * beatTicks

    // Playback state.
    double posTicks = 0.0;          // fractional current position
    size_t cursor   = 0;            // index of next unemitted event
    bool   active   = false;

    // Loop region (ticks).  Inactive when !looping.
    bool   looping   = false;
    int    loopStart = 0;
    int    loopEnd   = 0;

    // Tempo / speed.  speed 128 == "as composed".
    double tempoBPM = 52.0;
    int    speed    = 128;

    // Engine-supplied hooks.
    std::function<void(const MuseEvent&)> onEvent;    // route note/config/marker
    std::function<void()>                 onLoopWrap; // flush held notes, notify

    void   addEvent(const MuseEvent& e);
    void   sortEvents();
    void   reset();                       // rewind to start of playback
    double ticksPerSecond() const;        // includes speed scaling

    // Advance musical time by dtSeconds of (unpaused) wall-clock, emitting
    // events and wrapping the loop as needed.
    void advance(double dtSeconds);

    // Move the cursor/position to an absolute tick.  When emitConfig is true
    // (scan), Config events between the current position and target are emitted
    // in order; notes are never emitted here.
    void seekTo(int targetTick, bool emitConfig);

    size_t firstEventAtOrAfter(int tick) const;

    // 1-based bar and beat, 0-based tick-within-beat.
    void musicalPosition(int& bar, int& beat, int& tick) const;
};

} // namespace Muse
} // namespace Lab
