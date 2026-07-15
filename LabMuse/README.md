# LabMuse


```
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
 ```
 
**LabMuse** is a scoped but faithful reconstruction of LucasArts' **iMUSE**
(Interactive MUsic Streaming Engine), built on top of
[LabMidi](../README.md). It is a small C++ engine for *interactive music*:
music that shifts, layers, branches, and transitions in response to game state
— always at musically appropriate moments — rather than playing back as a fixed
linear track.

The companion essay [`imuse-technical.md`](./imuse-technical.md) is a historical
and technical analysis of the original system, written by Nick Porcino, who led
the team responsible for the final iteration of iMUSE (aiMUSE) used in *Star
Wars: The Force Unleashed*. LabMuse turns the concepts in that essay into
running code.

The bundled example, **"Die Toteninsel"** — an A-minor, Lento, 5/8 tone poem
after Rachmaninoff's *Isle of the Dead* — may well be the first original iMUSE
composition authored in decades.

---

## What makes this "iMUSE" and not just a MIDI player

LabMidi's `MidiSongPlayer` pre-flattens a song to a time-sorted event list. It
cannot loop, jump, scan, change speed, or pause. That is exactly the wrong shape
for interactive music. LabMuse replaces it with a **musical-time sequencer** and
wraps it in the iMUSE command model:

- **One `SoundCall(opcode, …)` dispatcher.** Every command — start, stop, fade,
  set a trigger, jump, change speed — routes through a single entry point, just
  as in the original. Opcodes keep the essay's numeric ranges (`0x00xx` system,
  `0x01xx` MIDI, `0x02xx` WAVE).
- **Musical time, not wall-clock time.** The sequencer tracks position in
  measures / beats / ticks and converts a wall-clock delta into ticks via
  `tempo × speed` (speed `128` = "as composed"). This is what lets transitions
  land on a bar line instead of "wherever the audio happened to be".
- **Markers, triggers, hooks, and deferred commands — kept distinct:**
  - A **marker** is a static musical anchor baked into the score (here, every
    5/8 bar line). Crossing one emits a notification.
  - A **trigger** is set at runtime by the director: "when sound *S* next
    crosses marker *M*, run command *C*." One-shot; re-armable.
  - A **hook** is a conditional gate stored per-sound, compared against values
    the game sets — the mechanism for branching within a score.
  - A **deferred command** runs at a scheduled time.
- **Layered parts on one clock.** In LabMuse a "theme" is not a separate track
  that has to be phase-synced; it is a **part** (a MIDI channel) of one master
  sequence. Laying a theme in or out is just a part-trim change. Because every
  part shares the master's clock, a theme laid in at bar 8 is *already
  rhythmically aligned* — no phase math, no drift.
- **A pause-aware, nested timebase.** `imPause`/`imResume` use a counter, not a
  flag, so independent systems can pause the music without fighting. Musical
  time is an accumulated delta, so pausing genuinely freezes the score.
- **Correct note lifecycle.** A per-channel active-note table tracks held notes;
  on stop, loop-wrap, jump-without-sustain, or a part being laid out, the engine
  emits the matching note-offs (plus all-notes-off) so the shared DLS synth
  never strands a voice.

Audio is rendered by LabMidi's built-in macOS `MidiSoftSynth` (the Apple DLS
General-MIDI synth). **One synth instance drives all 16 channels**, so every
layer and the one-shot stinger mix through a single sink.

---

## Building & running

From the LabMidi repository root:

```sh
cmake -B build -DLABMIDI_BUILD_EXAMPLES=ON -DLABMUSE_BUILD_EXAMPLES=ON
cmake --build build
```

Two ways to exercise it:

```sh
# Interactive audio demo (needs a real terminal + audio device):
./build/DieToteninsel

# Headless verification (no audio, no TTY — safe for CI / sandboxes):
./build/DieToteninsel --selftest
```

### The interactive demo

At startup the demo lists the available MIDI outputs and lets you choose one:

```
Select audio output:
  0: Built-in software synth (Apple DLS General MIDI)
  1: Yamaha DX7
  2: Yamaha CLP-840
  3: IAC Driver Bus 1
Choice [0]:
```

Choice `0` (the default) uses LabMidi's built-in soft synth. Any other choice
routes to that hardware port. The controls are then:

```
  1 lament   2 viola   3 dies-irae   4 agitato   5 shimmer
  s stinger   [ / ] tempo(speed)   space pause   q quit
```

The A-minor 5/8 ostinato loops continuously. Press `1`–`5` to lay a theme in or
out — the status line shows it as `PEND-IN` / `PEND-OUT` until the cursor reaches
the next bar line, where it flips to `IN` / `out`. Press `s` to fire a one-shot
tubular-bell stinger over the top (a second, independent sequencer). `[` / `]`
change the tempo via `mdSetSpeed`; space pauses and resumes cleanly with no
stuck notes.

### Output devices and voice profiles

The score is authored in General MIDI, but a non-GM instrument uses different
program numbers. Rather than re-author the score per device, the demo carries a
small, data-driven **device-profile** table (in
[`examples/DieToteninsel.h`](./examples/DieToteninsel.h) /
[`LabMuseComposition.cpp`](./examples/LabMuseComposition.cpp)):

```cpp
struct VoiceAssignment { const char* role; int program, bankMSB, bankLSB; };
struct DeviceProfile   { const char* name; const char* const* match;
                         int matchCount; const VoiceAssignment* voices;
                         int voiceCount; };
```

A profile is selected by matching any of a list of **case-insensitive
substrings** against the chosen port's name. Two profiles ship:

| Device | Matches | Voices |
|--------|---------|--------|
| Yamaha DX-7 | `dx7`, `dx-7` | stock ROM1A voices per role (`"dies-irae"` → `VOICE 1`, `"ostinato"` → `BASS 1`, …) |
| Yamaha Clavinova CLP-840 | `clp-840`, `clp840`, `clp` | General MIDI — it's GM-compatible, so the score's own voices are already right |

Aliases matter once you own more than one device from the same maker: a DX-7 and
a CLP-840 both report *"Yamaha"*, so each profile matches a **specific** token
rather than the shared brand. Ports that match nothing (including a bare
*"Yamaha"*) fall through to the General-MIDI default — the voices the score
declared. `ProfileForPort(name)` does the lookup; `ApplyDeviceProfile(muse,
profile)` overrides each matching part's instrument via `setPartInstrument()`
before the sound starts, so the remap costs the score nothing (and a
GM/empty profile is a no-op).

Covering another synth is a one-line table entry — no engine or composition
changes. (Note: a real single-timbre DX-7 responds to one voice at a time, so
its layers share whichever program was last selected; a multitimbral device like
the CLP-840, or one synth per part, gets the full layered effect.)

### The self-test

`--selftest` drives the engine with a virtual clock and asserts the behaviors
that define the system: lay-in and lay-out fire *exactly* on a bar line
(beat 1, tick 0), every marker lands on the bar, the nested pause counter
freezes and restores musical time, `imFade` ramps a part trim smoothly, the loop
wraps and re-arms its markers, and the non-looping stinger starts and stops
independently while the master keeps looping. It prints a marker/trigger log and
a PASS/FAIL summary.

---

## Writing a composition

A composition is built programmatically against the public API in
[`include/LabMuse/LabMuse.h`](./include/LabMuse/LabMuse.h). See
[`examples/LabMuseComposition.cpp`](./examples/LabMuseComposition.cpp) for the
complete "Die Toteninsel" score — the walkthrough below is the shape of it.

Note that the composition lives with the **example**, not the library: LabMuse
the library ships the engine only, so applications author their own scores.

### 1. Create the engine and a master sound

```cpp
#include <LabMuse/LabMuse.h>
using namespace Lab::Muse;

LabMuse muse(/*enableAudio=*/true);

// A 5/8 sound: 5 beats per bar, each beat an eighth = 120 ticks, so a bar is
// 600 ticks. Tempo (52 BPM) is quarter-notes; ticksPerQuarter = 240.
const int kMaster = 1;
muse.createSound(kMaster, groupMusic,
                 /*ticksPerQuarter*/ 240, /*tempoBPM*/ 52.0,
                 /*meter*/ 5, /*beatTicks*/ 120);
```

### 2. Define parts (channels) and write notes

Each layer is a part on its own MIDI channel with its own GM program and an
initial trim (`0` = silent/laid-out, `127` = full). Notes are absolute-tick.

```cpp
// channel, name, GM program, initial trim
muse.definePart(kMaster, 0, "ostinato",  43 /*Contrabass*/, 100); // always on
muse.definePart(kMaster, 3, "dies-irae", 52 /*Choir Aahs*/,   0); // laid out

// soundId, channel, startTick, durationTicks, note, velocity
muse.addNote(kMaster, 0, 0,   110, 45, 60);   // A2 on beat 1
muse.addNote(kMaster, 0, 120, 110, 52, 60);   // E3 on beat 2
// … program changes and control changes are available too:
// muse.addProgramChange(...); muse.addControlChange(...);
```

Each defined part's bank-select + program-change are emitted automatically when
the sound *starts* (not as scored events), so the synth is configured the moment
playback begins — and a device profile can still remap the voice up until then
via `setPartInstrument(sound, channel, program, bankMSB, bankLSB)`.

### 3. Place markers and a loop

Markers are the quantization grid. Put one on every bar line so the director can
target "the next bar" without computing where that is.

```cpp
const int kBar = 1;                 // a marker id meaning "a bar line"
for (int bar = 0; bar < 8; ++bar)
    muse.addMarker(kMaster, bar * 600, kBar);

muse.setLoop(kMaster, 0, 8 * 600);  // loop the whole 8-bar master forever
```

### 4. Drive it at runtime

Start the sound, then call `update(now)` every frame with the wall-clock time in
seconds. Everything else is `SoundCall`.

```cpp
muse.update(0.0);                       // prime the timebase
muse.SoundCall(imStartSound, kMaster);

while (running) {
    usleep(1000);
    muse.update(nowSeconds());
}
```

### 5. Lay a theme in — quantized to the next bar

This is the signature move. The director does **not** compute "the next bar"; it
targets the bar-marker class and lets the sequencer fire on the boundary:

```cpp
// "When the master next crosses a bar marker, set part 3's trim to full."
muse.SoundCall(imSetTrigger, kMaster, kBar, mdSetPartTrim,
               /*channel*/ 3, /*trim*/ 127);
```

The trigger is one-shot: it fires once at the next bar line, sets the trim (the
theme becomes audible, already phase-aligned), and clears itself. Laying out is
symmetric — set the target trim to `0`. For a smooth entrance instead of an
instant one, fade the trim over musical time:

```cpp
// param fadeMidiPartTrim + channel, target 127, over 500 ms
muse.SoundCall(imFade, kMaster, fadeMidiPartTrim + 3, 127, 500);
```

### 6. Fire a one-shot over the top

A stinger is just another sound with its own sequencer — start it whenever, and
it plays concurrently through the same synth on its own channel:

```cpp
muse.createSound(kStinger, groupSfx, 240, 52.0, 5, 120);
muse.definePart(kStinger, 6, "bells", 14 /*Tubular Bells*/, 127);
muse.addNote(kStinger, 6, 0, 720, 81, 95);
// no setLoop() → plays once and stops, flushing its own notes
muse.SoundCall(imStartSound, kStinger);
```

### Getting information back

Introspection for a UI or for logic that reacts to the music:

```cpp
int bar, beat, tick;
muse.musicalPosition(kMaster, bar, beat, tick);
int trim = muse.SoundCall(mdGetParam, kMaster, midiPartTrim, /*chan*/ 3);
```

…and a notification callback (`setNotify`) reports markers crossed, triggers
fired, part-trim changes, loop wraps, and sound start/stop — the "bidirectional"
half of the iMUSE model, where the sound driver talks back to the director.

---

## A note on quantization latency

A key press sets a trigger that fires on the **next** bar marker. If the press
lands just after a bar line, the theme waits until the following one — up to one
5/8 bar (~2.9 s at 52 BPM). This is intentional: it is what keeps entrances
musical. A production system would offer finer marker grids (beat-level, or
phrase-level "safe zones") so the composer can choose the trade-off per cue.

---

## Gap analysis / future work

LabMuse implements the *behavioral core* of iMUSE and the *full opcode surface*,
but several capabilities are scoped down or stubbed. To fully embody the iMUSE
specification as described in the essay and patent, the following would need to
be built out.

### Opcodes present but not fully realized

- **`mdShareParts`** — *stubbed.* The original could hand instrument parts from
  one sequence to another to make seamless transitions between whole pieces.
  LabMuse sidesteps this by modeling layers as parts of *one* master sequence.
  True `shareParts` requires multiple sequencers that can donate/adopt channels
  mid-phrase while preserving held notes and controller state.
- **`imSave` / `imRestore`** — *not implemented.* Full state serialization of
  every active sound, cursor, fade, trigger, deferred command, hook, and group
  volume, so a game save/load restores the exact musical moment.
- **`mdJump` / `mdScan`** — *implemented but lightly exercised.* Jump (playing
  notes sustain, no intermediate processing) and scan (re-emit all config
  between here and the target) work, but a real transition library that *uses*
  them — jump-hooks that branch to transition sequences and then to a
  destination cue — is not built here.
- **WAVE opcodes (`wvGetParam`, `wvStartDiskSound`)** — *stubbed.* iMUSE unified
  MIDI, CD, and streamed digital audio under one command interface. LabMuse is
  MIDI-only. A WAVE module driving LabSound (or the platform mixer) through the
  same `SoundCall` dispatcher would restore the multi-format architecture.

### Authoring model

- **Score-embedded hooks and markers via SysEx.** The original stored hooks,
  markers, loops, part-allocations, and param-adjusts as custom system-exclusive
  messages *inside* the MIDI stream, so a composer authored behavior in a DAW.
  LabMuse builds the composition database programmatically. A parser for iMUSE's
  soundbundle format + specialized header chunk (and a modern equivalent that
  round-trips through Logic/Cubase/Ableton markers) is the missing authoring
  half.
- **Conditionalized branching.** Hooks are stored and readable, but the score
  data model does not yet contain in-line *jump hooks* ("at this hook, if the
  game set it, jump to sequence X"). The essay's fight-scene example — enable a
  jump hook, enqueue a trigger, jump to a transition, then start victory/defeat
  music at the transition's end marker — is the target behavior.
- **Priority-based voice management.** `soundPriority` is stored but not acted
  on. Real iMUSE dropped/pre-empted low-priority sounds (and parts) under
  resource pressure, with `fadePriority` for graceful hand-off.

### Timing & fidelity

- **Fade curves beyond linear.** Fades are linear, stepped at 60 Hz. The
  original's Bresenham-style integer remainder accumulation is faithfully linear;
  a modern engine would add equal-power and exponential curves for musical
  cross-fades.
- **Sub-buffer timing accuracy.** Events are dispatched on the app's update tick
  (~1 ms) and rendered by the DLS synth on its own thread. iMUSE ran a ~333 Hz
  interrupt for sample-accurate scheduling. Sample-accurate dispatch would want
  timestamped events handed to the synth/mixer rather than fired "now".
- **Tempo maps and time-signature changes.** A sound has one tempo and one
  meter. Accelerandi, ritardandi, and mid-piece meter changes (all supported by
  the original's musical-time model) are not yet represented.
- **`imGetParam`/`mdGetParam` coverage.** A useful subset is queryable; the full
  original parameter set (pan, detune, transpose, per-part priority, chunk
  navigation) is only partially wired.

### Architecture

- **Group-volume ducking (`groupMusicDip`).** The group-volume busses and the
  master-scaling math are implemented, but automatic ducking — lowering music
  under dialogue — is not driven by anything. Hooking `groupMusicDip` to a
  voice-activity signal would complete the mixing model.
- **File manager / streaming.** Everything is resident in memory. The original's
  File Manager loaded and managed resources from disk/CD on demand.

None of these are required to *demonstrate* iMUSE — the loop, the layered parts,
the markers, the triggers, the quantized lay-in/lay-out, the concurrent stinger,
and the nested pause are all here and verifiable. They are the road from a
faithful demonstrator to a shippable interactive-music middleware.
