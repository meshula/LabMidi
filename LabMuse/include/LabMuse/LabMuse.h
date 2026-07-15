//
//  LabMuse.h
//
//  A scoped but faithful reconstruction of LucasArts' iMUSE interactive-music
//  engine, built on top of LabMidi's built-in software synthesizer.
//
//  The public surface mirrors the opcode set documented in the accompanying
//  essay (LabMuse/imuse-technical.md): every command routes through a single
//  SoundCall() dispatcher, sounds are layered "parts" (MIDI channels) of a
//  master musical sequence, and transitions are quantized to musical markers.
//
/*
┌──────────────────────────────────────────────────────────────────────────┐
│ ▓▓▓▒▒▒ PROVENANCE NODE [9] // ZONE: [HYBRID_REMIXABLE]                   │
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

#include <cstdint>

namespace Lab {

class MidiOutBase;   // LabMidi/MidiInOut.h -- a MIDI output sink

namespace Muse {

//----------------------------------------------------------------------------
// Opcodes.  Values agree with the essay's enum so the numbers are recognizable
// to anyone who has read the iMUSE documentation.  The 0x00 range is system
// commands, 0x01 is MIDI-specific, 0x02 is WAVE-specific.
//----------------------------------------------------------------------------

enum Opcode {
    /* System Commands */
    imInitialize    = 0x0000,  // initialize the system
    imTerminate     = 0x0001,  // shut down the system
    imPause         = 0x0002,  // pause all sound (nested)
    imResume        = 0x0003,  // resume all sound (nested)
    imSave          = 0x0004,  // save system state          (stub)
    imRestore       = 0x0005,  // restore system state       (stub)
    imSetGroupVol   = 0x0006,  // set volume for a sound group
    imPrepareSound  = 0x0007,  // preload a sound            (no-op success)
    imStartSound    = 0x0008,  // begin playing a sound
    imStopSound     = 0x0009,  // stop playing a sound
    imStopAllSounds = 0x000A,  // stop all sounds
    imGetNextSound  = 0x000B,  // find next active sound
    imSetParam      = 0x000C,  // set a sound parameter
    imGetParam      = 0x000D,  // get a sound parameter
    imFade          = 0x000E,  // fade a parameter over time
    imSetHook       = 0x000F,  // set a hook value
    imGetHook       = 0x0010,  // get a hook value
    imSetTrigger    = 0x0011,  // set an event trigger
    imCheckTrigger  = 0x0012,  // check if a trigger exists
    imClearTrigger  = 0x0013,  // remove a trigger
    imDeferCommand  = 0x0014,  // schedule a command for later
    imUndefined     = 0x0015,

    /* MIDI-specific commands */
    mdGetParam      = 0x0100,  // get MIDI parameter (measure/beat/tick/speed/tempo/part-trim)
    mdJump          = 0x0101,  // jump to musical position (playing notes sustain)
    mdScan          = 0x0102,  // scan to musical position (re-emit config, no notes)
    mdSetSpeed      = 0x0103,  // set MIDI playback speed (128 == as composed)
    mdSendMessage   = 0x0104,  // send a raw MIDI message
    mdSetPartTrim   = 0x0105,  // set MIDI part (channel) volume trim
    mdShareParts    = 0x0106,  // share MIDI parts between sounds (stub)
    mdUndefined     = 0x0107,

    /* WAVE-specific commands (present for surface completeness) */
    wvGetParam       = 0x0200, // (stub)
    wvStartDiskSound = 0x0201, // (stub)
    wvUndefined      = 0x0202
};

//----------------------------------------------------------------------------
// Group volume busses (hierarchical, master scales the rest).
//----------------------------------------------------------------------------

enum Group {
    groupMaster   = 0,
    groupSfx      = 1,
    groupVoice    = 2,
    groupMusic    = 3,
    groupMusicDip = 4,
    GROUP_COUNT   = 5
};

//----------------------------------------------------------------------------
// Fadeable parameters (imFade param argument).  Part trims occupy a contiguous
// range so channel c uses (fadeMidiPartTrim + c).
//----------------------------------------------------------------------------

enum FadeParam {
    fadePriority       = 0,
    fadeVol            = 1,
    fadePan            = 2,
    fadeDetune         = 3,
    fadeMidiSpeed      = 4,
    fadeMidiPartTrim   = 5,   // channel 0
    fadeMidiPartTrim16 = 20,  // channel 15
    fadeUndefined      = 21
};

//----------------------------------------------------------------------------
// Sound parameters (imSetParam / imGetParam param argument).
//----------------------------------------------------------------------------

enum SoundParam {
    soundVol       = 0,
    soundPan       = 1,
    soundTranspose = 2,
    soundDetune    = 3,
    soundPriority  = 4
};

//----------------------------------------------------------------------------
// MIDI parameters (mdGetParam param argument).  Matches the essay's midi* enum,
// extended with a part-trim query.
//----------------------------------------------------------------------------

enum MidiParam {
    midiChunk    = 0,
    midiMeasure  = 1,
    midiBeat     = 2,
    midiTick     = 3,
    midiSpeed    = 4,
    midiTempo    = 5,
    midiPartTrim = 6   // reads channel given by the 'chan' argument
};

//----------------------------------------------------------------------------
// Result codes returned by SoundCall().  imSuccess is 0; query opcodes return
// the requested value (which may be >= 0).
//----------------------------------------------------------------------------

enum Result {
    imSuccess        = 0,
    imArgErr         = -1,
    imNotImplemented = -2,
    imNoSound        = -3,
    imFail           = -4
};

//----------------------------------------------------------------------------
// Notifications emitted back to the directing system (the "bidirectional
// communication model" from the essay).  Used by the demo's status line and by
// the headless self-test to assert quantized behavior.
//----------------------------------------------------------------------------

struct Notification {
    enum Type {
        Marker,        // the cursor crossed a marker
        TriggerFired,  // a queued trigger matched a marker and ran its command
        DeferFired,    // a deferred command ran
        PartTrim,      // a part's trim changed
        LoopWrap,      // a looping sound wrapped back to its loop start
        SoundStarted,
        SoundStopped
    };
    Type   type   = Marker;
    int    sound  = 0;
    int    marker = 0;   // marker id (Marker/TriggerFired)
    int    part   = 0;   // channel (PartTrim), or fired opcode (TriggerFired)
    int    value  = 0;   // trim value / arg
    int    bar    = 0;   // musical position when the event fired (1-based)
    int    beat   = 0;   // 1-based, counts the meter's beat unit
    int    tick   = 0;   // 0-based ticks within the beat
};

typedef void (*NotifyFn)(void* user, const Notification&);

//----------------------------------------------------------------------------
// The engine.
//----------------------------------------------------------------------------

class LabMuse {
public:
    // enableAudio == false builds the engine with no MidiSoftSynth (headless
    // self-test / sandbox use, or when you intend to call setOutput()); musical
    // logic still runs and notifies regardless.
    explicit LabMuse(bool enableAudio = true);
    ~LabMuse();

    // Route MIDI output to a caller-owned sink (e.g. a Lab::MidiOut hardware
    // port, or a MidiSoftSynth you manage) instead of the built-in synth.  The
    // sink must outlive the engine's playback.  Passing nullptr silences output.
    void setOutput(MidiOutBase* sink);

    LabMuse(const LabMuse&) = delete;
    LabMuse& operator=(const LabMuse&) = delete;

    // The single command dispatcher.  Unused arguments are ignored; see the
    // Opcode comments and the essay for the per-opcode argument mapping.
    int SoundCall(int opcode,
                  int a = 0, int b = 0, int c = 0,
                  int d = 0, int e = 0, int f = 0);

    // Advance the engine to absolute wall-clock time 'now' (seconds).  Catches
    // up all events with tick <= now and steps fades on a fixed 60 Hz counter.
    void update(double now);

    void setNotify(NotifyFn fn, void* user);

    //-- Composition-database builder ---------------------------------------
    // Define a sound (a soundfile becoming a sound in iMUSE terms).  meter is
    // beats-per-bar; beatTicks is the tick length of one beat.  For 5/8 use
    // meter=5, beatTicks=120, ticksPerQuarter=240.
    int  createSound(int soundId, int group,
                     int ticksPerQuarter, double tempoBPM,
                     int meter, int beatTicks);

    void setLoop(int soundId, int startTick, int endTick);
    void definePart(int soundId, int channel, const char* name,
                    int gmProgram, int initialTrim);
    // Override a part's instrument, e.g. to map a General-MIDI score onto a
    // non-GM device's voice bank.  bankMSB/bankLSB < 0 are not sent.  The
    // bank-select + program-change are emitted when the sound starts.
    void setPartInstrument(int soundId, int channel, int program,
                           int bankMSB = -1, int bankLSB = -1);
    void addNote(int soundId, int channel, int startTick, int durTicks,
                 int note, int velocity);
    void addControlChange(int soundId, int channel, int tick, int cc, int value);
    void addProgramChange(int soundId, int channel, int tick, int program);
    void addMarker(int soundId, int tick, int markerId);

    //-- Introspection (for UI / self-test) ---------------------------------
    bool soundActive(int soundId) const;
    // Musical position of a sound; returns false if the sound doesn't exist.
    bool musicalPosition(int soundId, int& bar, int& beat, int& tick) const;
    double tempo(int soundId) const;
    int  speed(int soundId) const;

    int  partCount(int soundId) const;
    // For part index i (0..partCount-1): channel, trim (0..127), whether a
    // trigger is pending for it and the pending target trim.
    bool partInfo(int soundId, int index, int& channel, const char*& name,
                  int& trim, bool& pending, int& pendingTrim) const;

private:
    class Detail;
    Detail* _detail;
};

} // namespace Muse
} // namespace Lab
