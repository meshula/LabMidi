# Understanding LucasArts' iMUSE System: A Historical and Technical Analysis

by Nick Porcino

- [Understanding LucasArts' iMUSE System: A Historical and Technical Analysis](#understanding-lucasarts-imuse-system-a-historical-and-technical-analysis)
  - [Introduction](#introduction)
    - [About the author](#about-the-author)
  - [Key Terminology](#key-terminology)
    - [System Roles](#system-roles)
    - [Authoring Concepts](#authoring-concepts)
    - [Runtime Concepts](#runtime-concepts)
    - [Data Structures \& Timing](#data-structures--timing)
    - [MIDI Resolution Terms:](#midi-resolution-terms)
  - [Authoring for iMUSE](#authoring-for-imuse)
    - [MIDI Foundations](#midi-foundations)
    - [Soundfiles versus Sounds](#soundfiles-versus-sounds)
    - [Markers and Hooks](#markers-and-hooks)
      - [Markers: Musical Anchors](#markers-musical-anchors)
      - [Hooks: Conditional Logic](#hooks-conditional-logic)
  - [Core Architecture](#core-architecture)
    - [System Overview](#system-overview)
    - [Command Interface and Opcode Routing](#command-interface-and-opcode-routing)
    - [Extensibility and Longevity](#extensibility-and-longevity)
  - [Time in iMUSE](#time-in-imuse)
    - [Absolute Time (System Time)](#absolute-time-system-time)
      - [Fades and Absolute Time](#fades-and-absolute-time)
    - [Musical Time](#musical-time)
      - [Expressive Control](#expressive-control)
    - [Game Time (Narrative Time)](#game-time-narrative-time)
    - [Reconciling Time Domains](#reconciling-time-domains)
      - [Nested Pausing](#nested-pausing)
      - [Multiple Sequencers](#multiple-sequencers)
      - [Time Synchronization Challenges](#time-synchronization-challenges)
      - [Summary](#summary)
  - [Implementation](#implementation)
    - [System Commands](#system-commands)
    - [MIDI-Specific Commands](#midi-specific-commands)
    - [WAVE-Specific Commands](#wave-specific-commands)
    - [MIDI Implementation and Extensions](#midi-implementation-and-extensions)
      - [Soundbundle Structure](#soundbundle-structure)
      - [Specialized Header Chunk](#specialized-header-chunk)
      - [Custom MIDI Messages](#custom-midi-messages)
      - [MIDI Command Set](#midi-command-set)
    - [Parsing Implications](#parsing-implications)
    - [MIDI Implementation Details](#midi-implementation-details)
    - [Parameter Controls](#parameter-controls)
    - [Parameter Controls](#parameter-controls-1)
      - [Sound Priority (`soundPriority`)](#sound-priority-soundpriority)
    - [Group Volume Busses](#group-volume-busses)
    - [Fades System](#fades-system)
    - [Interrupt-Based Processing](#interrupt-based-processing)
    - [Composition Database Formation and Utilization](#composition-database-formation-and-utilization)
    - [Composing and Conditionalizing MIDI Sequences](#composing-and-conditionalizing-midi-sequences)
    - [Example: Assembling a Sound Track](#example-assembling-a-sound-track)
    - [Practical Example: A Fight Scene](#practical-example-a-fight-scene)
    - [State Management](#state-management)
  - [Building iMuse Today](#building-imuse-today)
    - [Key Components to Implement](#key-components-to-implement)
    - [Modern Advantages](#modern-advantages)
  - [Example Modern Implementation (Pseudo-code)](#example-modern-implementation-pseudo-code)
  - [Conclusion](#conclusion)
  - [References](#references)

## Introduction

This document explores LucasArts' groundbreaking iMUSE (Interactive Music Streaming Engine) system, a revolutionary technology from the early 1990s that fundamentally changed how music functioned in interactive media. The goal here is twofold: to explain to a modern audience how this historic system worked, and to explore how one might recreate similar functionality using today's technology.

The research presented here draws from multiple sources, including analysis of the original iMUSE source code, the system's patent documentation (US5315057)[¹], and historical knowledge about its implementation in classic LucasArts games. While iMUSE was first fully implemented in Monkey Island 2, its development was deeply influenced by the 1990 game Loom, which was organized around musical motifs and puzzles—establishing the conceptual foundation that would evolve into the sophisticated iMUSE system. By examining both the technical architecture and the creative workflows that powered iMUSE, we can better understand its significance and extract principles relevant to contemporary interactive audio design.

This document is aimed at game developers, composers, audio programmers, and fan of classic gaming history; it provides an historical perspective and practical insights for modern applications.

### About the author

Nick Porcino has worked at the intersection of art and engineering since the early days of 8-bit computing. His career has spanned roles in computer graphics, neural networks, robotics, toys, games, film production, and immersive technologies. Over the years, he has contributed to tools and engines at companies including Bandai, Disney, LucasArts, Industrial Light & Magic, Apple, Oculus Research, and Pixar.

While at LucasArts, Nick wrote the audio engine for internally developed games and led the team responsible for the final iteration of iMUSE—aiMUSE—used in Star Wars: The Force Unleashed. His experience offers a rare insider’s perspective on both the original system and its evolution.

Nick created and maintains the open-source projects LabSound and LabMidi, and continues to be active in advancing real-time audio systems for interactive media.

____
## Key Terminology

Understanding iMUSE requires familiarity with a number of specialized terms that describe how music is authored, interpreted, and controlled in real time. These terms are drawn from the system's original documentation and usage in shipped games.

### System Roles

- **Directing System**: The system that controls music playback decisions. This is typically the game engine or runtime logic, which reacts to player input and story progression by triggering musical changes in iMUSE.

- **Composer**: The creator of the interactive score. In iMUSE, composers not only write the music but also define how it behaves under different game conditions, embedding logic into the structure of the score.

- **Sound Driver**: The playback engine responsible for rendering music and sound effects based on performance data and real-time instructions. It interprets the composition database and manages hardware or software synthesizers.
  
### Authoring Concepts

- **Composition Database**: A data structure created before gameplay begins, containing musical sequences along with logic (conditional messages, markers, etc.) that guide how and when those sequences are played. This is authored by the composer and interpreted by the sound driver at runtime.

- **Control Instructions**: A set of commands or guidelines authored by the composer and implemented by the game programmer. These define how the directing system should trigger transitions, set hooks, or respond to game events in order to manipulate music playback.

- **Conditionalizing**: The process of embedding conditional logic into musical content. Composers define alternative paths, branches, and transitions based on expected gameplay scenarios, enabling music to adapt dynamically.

- **Marker**: A designated point within a musical sequence (usually at a measure, beat, or tick) used to indicate where a transition or action may occur. These are used to schedule musical changes with minimal disruption to musical flow.

- **Hook**: A logical variable or state set by the game to indicate that a certain condition has been met. Hooks are used to signal iMUSE to check for transitions or trigger events at designated markers.

- **Control Mapping**: The process by which composers and game designers collaborate to map game events and narrative beats to corresponding musical changes. This mapping ensures the interactive score reacts appropriately to gameplay context.

### Runtime Concepts

- **Decision Point**: A pre-defined moment within a sequence where iMUSE evaluates current game conditions and determines which path to follow (e.g., remain in a loop, transition to a new section, or end the piece). Decision points are where conditional logic is executed.

- **Soundfile**: A computer file containing one or more musical sequences. It is a static asset on disk that becomes a dynamic “sound” when interpreted by the sound driver.

- **Sound**: A soundfile that is actively being played. This distinction is important because iMUSE can alter a sound’s playback behavior in real time based on game conditions.

### Data Structures & Timing

- **Sequence**: A stream of time-stamped musical performance data intended to be played as a unit (e.g., an intro, loop, stinger, or transition). Sequences are the fundamental building blocks of interactive music in iMUSE.

### MIDI Resolution Terms:

- **Measure, Beat, Tick**: Hierarchical subdivisions of time in MIDI. iMUSE uses this resolution to precisely place markers and schedule transitions.
- **Tempo**: Musical speed, measured in beats per minute (BPM).
- **Speed**: An internal value affecting playback timing, often used to fine-tune sequence behavior.

_____
## Authoring for iMUSE

Authoring music for iMUSE is a fundamentally different process than traditional game scoring. iMuse composers don't simply write music to accompany gameplay, they design **musical systems** that shift, branch, and adapt in real time. This requires a unique mindset, blending musical intuition with procedural thinking. At the heart of the process is **conditionalization**, the embedding of logic into the structure of a composition so that the music reacts to the game. Composers authoring runtime *behavior*. In iMUSE, a piece of music is rarely a linear track from beginning to end. Instead, it resembles a graph: multiple sequences interconnected by possible transitions, each designed to be musically and emotionally coherent regardless of how the player arrived there.

### MIDI Foundations

Much of iMUSE’s flexibility came from its tight integration with the MIDI format. MIDI, being lightweight and expressive, allows composers and engineers to manipulate performance parameters programmatically. Under the hood, iMUSE exposed control over fundamental MIDI structures:

```c
enum {
    midiChunk   = 0,  // MIDI file chunk
    midiMeasure = 1,  // Measure number
    midiBeat    = 2,  // Beat within measure
    midiTick    = 3,  // Tick within beat (0–479)
    midiSpeed   = 4,  // Relative speed
    midiTempo   = 5,  // Tempo in BPM
    // ...
};
```

Composers could specify what should be played when, down to the individual tick. Tempo changes, accelerandi, and sync points were all possible, and essential for aligning music with on-screen action.

A typical iMUSE composition might include:

* A **main theme** with looping sections that could continue indefinitely.
* **Variation layers** that could fade in or out depending on player activity.
* **Transition phrases** to bridge between emotional states or narrative beats.
* **Stingers** and **punctuations** for specific events—say, when a door slams shut, or a villain reveals himself.

An illustration in practice from *Monkey Island* is that when Guybrush enters a room the music shifts seamlessly to a new theme, retaining the same tempo and key, creating the illusion of a single continuous performance, even though multiple sequences are stitched together under the hood.

### Soundfiles versus Sounds

An important distinction in iMUSE authoring is the difference between a **soundfile** and a **sound**. A soundfile is static; containing MIDI data and markers. A sound, on the other hand, is the active in process compenent: a soundfile that’s actively being interpreted by the sound driver, with logic hooks firing, transitions being evaluated, and sequences being joined in real time. Composers had to work in both domains, anticipating choice points and themes in the soundfiles, and orchestrating the proceduralism via game logic and conditionalization. This duality between latent content, and procedural evolution is embedded in the foundational concepts of **Markers** and **Hooks**.

### Markers and Hooks

One of iMUSE’s most powerful innovations is the ability to link the dynamic state of the game with the fixed structure of a musical score. This is accomplished through a two-part system: markers, which act as musical breakpoints, and hooks, which encode the logic of conditional transitions. Together, markers and hooks bridge music and game state, enabling composers and designers to write music that adapts in real time without sacrificing musicality.

#### Markers: Musical Anchors

Markers are embedded directly into MIDI sequences and defined specific musical positions where evaluation could occur. A marker might be placed at, say, measure 12, beat 3, tick 240, to signal a cadence point or natural transition zone. Each marker includes a marker id, used for matching logic, and a sound number, to disambiguate which sequence the marker belongs to.

When a marker is reached during playback, a marker processing routine checks the associated id and sound number versus triggers that have been queued by the game logic and keyed to that marker ID and sound number. If so, an associated command (like a musical jump or change) executes. If not, playback continued uninterrupted. This deferred execution model allows composers to score potential transitions without forcing the the transitions. Branching occurs only when game logic requires it, and at an authored musically appropriate point.

#### Hooks: Conditional Logic

Hooks were the logic predicates, or gatekeepers, of musical transitions, encoding the conditions under which a branch should occur. Each hook message embedded in a score specifies a hook ID (from 0-127), a hook class, such as jump,  transpose, part-enable, part-volume, part-program, part-transpose, and so on, and class specific parameters.

During playback, the system evaluates hooks dynamically. Hook IDs are compared against values set by the game's directing system. For example, if the player had triggered a chase sequence, the directing system might set hook 42 to 1, prompting a marker-associated hook to redirect the music into a high-intensity variant. This allows branching within the musical logic according to live game state. Because hooks are modular and decoupled from their triggering markers, composers can use them to encode many possible futures without recomposing entire cues.

Hooks and markers work together to create a model of conditional, musically aware branching, rare in audio systems even today. Musical timing integrity is served through precise manangement of transitions. Game logic can trigger branching organically allowing music to respond without looping awkwardly or resetting. Composers can embed several possible musical outcomes in a single score and let the game choose paths in real time.

As an example, a loop might continue indefinitely during exploration. When the player entered danger, the directing system would set a hook value. At the next marker, iMUSE would evaluate that hook, and if conditions were met, branch to a more intense passage—seamlessly, musically, and without breaking immersion.

The system of markers and hooks was not only ahead a breakthrough at the time, but remains largely unrecognized for the sophistication it brought to adaptive music. By placing intelligence in the music data itself, iMUSE gave composers and developers a shared language for real-time scoring, an interactive musical system that responds to the player as a performer, not just a listener.

______
## Core Architecture

iMUSE was built as a modular, hierarchical system designed to give composers and game designers fine-grained control over music playback and interaction. At its core, it provided a flexible structure that could handle MIDI, CD audio, and digital audio seamlessly, all while responding in real-time to events in the game world.

### System Overview

INSERT FIGURE 2 HERE

The architecture, as described in Figure 2 of the iMUSE patent, consists of three major components:

1. **Composition Database (101)**: This houses the musical content and all conditional instructions needed for performance. It’s the source of truth for what music is available, how it’s structured, and under what conditions different segments should play.

2. **Game System (111)**: The host or directing system — typically the game engine — which drives the iMUSE system by issuing commands based on player actions, narrative beats, or system events.

3. **Sound Driver (100)**: This is the core of iMUSE — a suite of modules that interpret and execute commands from the game system. It is further subdivided into:

   * **General Modules (102):**

     * **Command Interface (104):** Central command dispatcher — all commands from the game route through here.
     * **Time Control (106):** Maintains consistent timing across music and effects, synchronizing beats, loops, and transitions.
     * **File Manager (108):** Loads and manages sound resources from disk or other media.

   * **Sound Type Modules:**

     * **MIDI Module (110):** Plays MIDI-based music, with dynamic tempo, speed, and branching support.
     * **CD Module (112):** Interfaces with CD audio (used on platforms like CD-ROM consoles).
     * **Audio Module (114):** Plays digital audio such as WAV files or streamed formats.

This separation allowed the iMUSE system to evolve over time as new audio formats emerged, without redesigning the core infrastructure.

---

### Command Interface and Opcode Routing

The command interface provides a clean abstraction for control. All commands enter through `SoundCall()`, which dispatches them based on their opcode:

* **0x0000 – 0x00FF:** System commands (common to all audio types)
* **0x0100 – 0x01FF:** MIDI-specific commands
* **0x0200 – 0x02FF:** WAVE-specific commands

This makes the system extensible and modular. New modules could be introduced by reserving additional opcode ranges, and new functionality could be layered on without affecting existing logic.

The complete set of opcodes was:

```c
enum {
    /* System Commands */
    imInitialize           = 0x0000,  /* Initialize the system */
    imTerminate            = 0x0001,  /* Shut down the system */
    imPause                = 0x0002,  /* Pause all sound */
    imResume               = 0x0003,  /* Resume all sound */
    imSave                 = 0x0004,  /* Save system state */
    imRestore              = 0x0005,  /* Restore system state */
    imSetGroupVol          = 0x0006,  /* Set volume for a sound group */
    imPrepareSound         = 0x0007,  /* Preload a sound */
    imStartSound           = 0x0008,  /* Begin playing a sound */
    imStopSound            = 0x0009,  /* Stop playing a sound */
    imStopAllSounds        = 0x000A,  /* Stop all sounds */
    imGetNextSound         = 0x000B,  /* Find next active sound */
    imSetParam             = 0x000C,  /* Set a sound parameter */
    imGetParam             = 0x000D,  /* Get a sound parameter */
    imFade                 = 0x000E,  /* Fade a parameter over time */
    imSetHook              = 0x000F,  /* Set a hook value */
    imGetHook              = 0x0010,  /* Get a hook value */
    imSetTrigger           = 0x0011,  /* Set an event trigger */
    imCheckTrigger         = 0x0012,  /* Check if a trigger exists */
    imClearTrigger         = 0x0013,  /* Remove a trigger */
    imDeferCommand         = 0x0014,  /* Schedule a command */
    imUndefined            = 0x0015,  /* End of system commands */
    
    /* MIDI-specific commands */
    mdGetParam             = 0x0100,  /* Get MIDI parameter */
    mdJump                 = 0x0101,  /* Jump to position in MIDI */
    mdScan                 = 0x0102,  /* Scan to position in MIDI */
    mdSetSpeed             = 0x0103,  /* Set MIDI playback speed */
    mdSendMessage          = 0x0104,  /* Send a MIDI message */
    mdSetPartTrim          = 0x0105,  /* Set MIDI part volume trim */
    mdShareParts           = 0x0106,  /* Share MIDI parts between sounds */
    mdUndefined            = 0x0107,  /* End of MIDI commands */
    
    /* WAVE-specific commands */
    wvGetParam             = 0x0200,  /* Get WAVE parameter */
    wvStartDiskSound       = 0x0201,  /* Stream a sound from disk */
    wvUndefined            = 0x0202   /* End of WAVE commands */
};
```

These opcodes allowed the game engine to control nearly every aspect of playback: when to start or stop, when to jump to a new section, how to handle transitions, and how to respond to game state changes.


### Extensibility and Longevity

By assigning opcode ranges and building around a dispatch system, iMUSE was built to evolve. CD audio and WAV modules were later additions. Had new formats or systems arisen (like streaming audio or positional 3D sound), they could have been slotted into the architecture cleanly.


## Time in iMUSE

One of iMUSE’s most innovative achievements was its ability to reconcile multiple notions of time within an interactive environment. Music in games must not only sound good—it must also remain in sync with gameplay, adapt to changing player actions, and transition smoothly between states. To make this possible, iMUSE carefully managed three overlapping time domains: **absolute time**, **musical time**, and **game (narrative) time**.

### Absolute Time (System Time)

Absolute time is the bedrock of audio fidelity. It ensures that samples play at the correct rate, fades progress smoothly, and that all audio processes are consistently timed. iMUSE managed this with regular interrupts fired at a frequency of approximately 333 Hz (every 3 milliseconds), with additional tracking at 60Hz intervals for less critical updates.

```c
#define USEC_PER_INT        3000        /* microseconds per interrupt */
#define USEC_PER_60TH       16667       /* some useful time constants */
#define MSEC_PER_60TH       17
```

The Time Control module maintained this interrupt system, serving as the heartbeat for the entire sound engine. Listeners can be very sensitive to inaccuracies in timing so this system is critical.

#### Fades and Absolute Time

Fades in iMUSE used a simple linear interpolation scheme, incrementally adjusting parameters at regular intervals to ensure perceptual smoothness:

```c
void FdIntHandler()
{
    // Called at 60Hz to update all active fades
    // ...
    if ((fdp->modOvfloCounter += fdp->slopeMod) >= fdp->length) {
        fdp->modOvfloCounter -= fdp->length;
        val += fdp->nudge;
    }
    // ...
}
```

Fades had to be tightly tied to system time to avoid audible glitches or unnatural parameter shifts.


### Musical Time

**Musical time** is how humans perceive music—measures, beats, and ticks. In iMUSE, musical time was managed by player-sequencer pairs. Each sequencer tracked progress through a score using a hierarchical structure:

```c
enum {
    midiChunk            = 0,  /* midifile chunk */
    midiMeasure          = 1,  /* measure */
    midiBeat             = 2,  /* beat */
    midiTick             = 3,  /* tick 0-479 */
    // ...
};
```

This hierarchical time structure allowed **musical navigation** and **tempo flexibility**. Musical time enabled score-based control over playback, and crucially, non-linear navigation within compositions. The system allowed dynamic tempo changes (`MdSetSpeed()`), jumps (`MdJump()`), scans (`MdScan()`), and conditional transitions—all expressed in musical terms.

Commands like `MdJump` and `MdScan` operated in musical time, enabling the system to move to specific points in a composition while maintaining musical coherence. Playback speed could be adjusted without disrupting the relative musical relationships.

```c
int32 MdSetSpeed(int32 sound, int32 speed);
```

Setting a speed of 128 would play "as composed," while higher or lower values would adjust the tempo.

#### Expressive Control

- **Hooks and Markers**: Placed at musical positions to trigger events only when musically appropriate.
- **Tempo Manipulation**: Allowed acceleration/deceleration without compromising timing fidelity.
- **Looping**: Executed based on musical start and end points.

```c
int32 MdSetLoop(int32 sound, int32 count, 
                int32 start_beat, int32 start_tick, 
                int32 end_beat, int32 end_tick);
```

By placing hooks and markers at specific musical times, composers could ensure transitions occurred only at musically appropriate moments. This was controlled by **conditional branching at musical points**. Unlike absolute time, musical time wasn't strictly linear in iMUSE. The system could jump or scan to different points in musical time, creating a non-linear experience while maintaining musical coherence.

### Game Time (Narrative Time)

Game time refers to the temporal flow of the player's experience. It’s irregular, nonlinear, and dictated by player agency:

- Players can pause the game, enter menus, or leave the controller idle.
- They may replay sections, skip scenes, or perform actions in unpredictable order.
- Narrative progression is subjective and asynchronous.

iMUSE did not explicitly track game time as a variable, but it integrated it indirectly, via host commands and conditional logic. The game (via the Directing System) could issue control commands to start, stop, pause, or manipulate music based on narrative events. These commands were designed to coordinate transitions between musical and game-time events seamlessly.


### Reconciling Time Domains

Synchronizing absolute, musical, and game time is nontrivial. iMUSE achieved this with several mechanisms:

#### Nested Pausing

The system used a pause counter rather than a binary pause flag. This allowed multiple components to request pauses independently, ensuring music only resumed when all systems had unpaused.


#### Multiple Sequencers

Multiple player-sequencer pairs ran concurrently, each with its own musical timeline. This allowed:

- Simultaneous sound layers (e.g., background themes + interactive stingers)
- Asynchronous control (one stream could fade while another looped or branched)
- Sequencers (module 124) and players (module 130), as noted in the patent, formed these independently controlled music channels.

Multiple player-sequencer pairs in the MIDI module allowed for simultaneous, independent musical timelines. Each player-sequencer pair maintained its own musical time and could be independently controlled. This meant that stingers or sound effects could play over main themes without disrupting either timeline.


#### Time Synchronization Challenges

Several subtle issues emerged in managing time synchronization:

1. **Jump vs. Scan**: When using `MdJump`, notes that were already playing would continue to sustain, but new notes at the destination wouldn't be triggered. `MdScan` would read through all messages, ensuring configuration was correct but potentially causing disruption during the scan.

2. **Loop Management**: Loops operated in musical time but needed to coordinate with game events that might occur at any point:

```c
int32 MdSetLoop(int32 sound, int32 count, 
                int32 start_beat, int32 start_tick, 
                int32 end_beat, int32 end_tick);
```

3. **Fade Alignment**: Fades needed to operate in absolute time for smoothness, but might need to complete by specific points in musical time.

4. **Conditional Time-Shifting**: A hook might cause a jump in musical time at any point, requiring the system to gracefully reconcile currently playing notes with the new timeline.

#### Summary

The iMUSE system's temporal design was one of its greatest strengths. By clearly distinguishing and managing absolute, musical, and game time—and allowing them to interact through well-structured APIs and flexible timing rules—iMUSE enabled adaptive music that was both expressive and reactive.

This nuanced approach to time remains a foundational model for modern adaptive music engines in games. Modern adaptive music engines — FMOD, Wwise, and even game-specific systems like the one in *The Legend of Zelda: Breath of the Wild* — use ideas pioneered here: modular composition, branching paths, real-time control, hooks into game state. iMUSE was not just a playback engine. It was a music director. It orchestrated — quite literally — a richer emotional experience for players, using structure and timing as its score.

____
## Implementation

### System Commands

The commands were implemented in the system through corresponding functions:

- `ImInitialize(InitDataPtr idp)`: Set up the iMUSE system with configuration data
- `ImTerminate()`: Shut down the system
- `ImPause()` and `ImResume()`: Temporarily pause and resume all sounds
- `ImSave(char *buf, int32 size)` and `ImRestore(char *buf)`: Save/load system state
- `ImSetGroupVol(int32 group, int32 vol)`: Control volume for sound categories
- `ImPrepareSound(int32 sound, int32 flag)`: Preload sound data
- `ImStartSound(int32 sound)`: Begin playback of a sound
- `ImStopSound(int32 sound)`: Stop a playing sound
- `ImStopAllSounds()`: Stop all playing sounds
- `ImGetNextSound(int32 sound)`: Find the next active sound ID after the given one
- `ImSetParam(int32 sound, int32 param, int32 val)`: Modify a sound parameter
- `ImGetParam(int32 sound, int32 param)`: Query a sound parameter
- `ImFade(int32 sound, int32 param, int32 val, int32 time)`: Smoothly change a parameter
- `ImSetHook(int32 sound, int32 val)` and `ImGetHook(int32 sound)`: Set/get hook values
- `ImSetTrigger(int32 sound, int32 marker, int32 opcode, ...)`: Create event listeners
- `ImCheckTrigger(int32 sound, int32 marker, int32 opcode)`: Test if a trigger exists
- `ImClearTrigger(int32 sound, int32 marker, int32 opcode)`: Remove a trigger
- `ImDeferCommand(int32 time, int32 opcode, ...)`: Schedule a command for later

### MIDI-Specific Commands

- `MdGetParam(int32 sound, int32 param, int32 chan)`: Get MIDI-specific parameters
- `MdJump(int32 sound, int32 chunk, int32 measure, int32 beat, int32 tick, int32 sustain)`: Jump to a specific position in a MIDI sequence
- `MdScan(int32 sound, int32 chunk, int32 measure, int32 beat, int32 tick)`: Scan to a position
- `MdSetSpeed(int32 sound, int32 speed)`: Change MIDI playback speed
- `MdSendMessage(int32 sound, int32 arg1, int32 arg2, int32 arg3)`: Send MIDI messages
- `MdSetPartTrim(int32 sound, int32 chan, int32 trim)`: Adjust channel volume
- `MdShareParts(int32 sound1, int32 sound2)`: Share MIDI parts between two sequences

### WAVE-Specific Commands

- `WvGetParam(int32 sound, int32 param)`: Get WAVE-specific parameters
- `WvStartDiskSound(FILE *file, ulong offset, int32 *idPtr, int32 type)`: Stream audio from disk

### MIDI Implementation and Extensions

iMUSE was built upon the standard MIDI specification but extended it in several important ways to support interactive music capabilities. Understanding these extensions is crucial for anyone looking to parse iMUSE MIDI files or implement a similar system.
The iMUSE system extended the standard MIDI file format in two significant ways:

#### Soundbundle Structure

iMUSE introduced a container format called a "soundbundle":

```
FIG. 1(c) shows the format of a soundbundle 52 which contains multiple versions 
of the same soundfile orchestrated for different target hardware. Preferred versions 
of the soundfile are located closer to the front of the soundbundle, so that in 
cases in which the hardware is capable of playing several versions, the preferred 
versions will be found first.
```

This structure allowed a single file to contain multiple orchestrations of the same music, optimized for different audio hardware. This was particularly important in the early 1990s when sound card capabilities varied dramatically.

#### Specialized Header Chunk

Standard MIDI files begin with a header chunk (type "MThd"), but iMUSE added its own specialized header chunk at the beginning of each soundfile:

```c
typedef struct {
    char        type[4];            /* ASCII chunk type */
    uint32      length;             /* bytes following length */
    uint16      version;            /* hardware version ID */
    uint8       priority;           /* playback priority */
    uint8       volume;             /* default volume */
    uint8       pan;                /* default pan */
    uint8       transpose;          /* default transpose */
    uint8       detune;             /* default detune */
    uint8       speed;              /* default speed */
} SpecializedHeader;
```

This header provided default performance parameters that the MIDI module would use when playing the file, creating a convenient initialization mechanism.

#### Custom MIDI Messages

The most significant extension was the addition of custom MIDI messages to support interactivity. These were implemented using the "system exclusive" message format provided in the MIDI specification, which allows for custom data to be embedded within standard MIDI streams. iMUSE added six custom system exclusive messages:

```
part-alloc       - Used to assign/unassign an instrument part to a MIDI channel
bulk-dump        - Used to send multiple parameters simultaneously
param-adjust     - Used to change a particular instrument parameter
hook             - Marks conditional branch points in the music
marker           - Marks points where game events should be triggered
loop             - Sets loop points for sequence playback
```


#### MIDI Command Set

The iMUSE system provided an extensive set of MIDI-specific commands that went beyond standard MIDI capabilities. The full set of MIDI commands was:

```c
enum {
    mdGetParam              = 0x0100,  /* MUST AGREE WITH MD.C */
    mdJump                  = 0x0101,
    mdScan                  = 0x0102,
    mdSetSpeed              = 0x0103,
    mdSendMessage           = 0x0104,
    mdSetPartTrim           = 0x0105,
    mdShareParts            = 0x0106,
    mdUndefined             = 0x0107
};
```

These commands were implemented as function calls in the API:

```c
int32 MdGetParam(int32 sound, int32 param, int32 chan);
int32 MdJump(int32 sound, int32 chunk, int32 measure, int32 beat, int32 tick, int32 sustain);
int32 MdScan(int32 sound, int32 chunk, int32 measure, int32 beat, int32 tick);
int32 MdSetSpeed(int32 sound, int32 speed);
int32 MdSendMessage(int32 sound, int32 arg1, int32 arg2, int32 arg3);
int32 MdSetPartTrim(int32 sound, int32 chan, int32 trim);
int32 MdShareParts(int32 sound1, int32 sound2);
```

Particularly noteworthy are:

1. **MdJump/mdJump**: This allowed the sequencer to immediately jump to a specific location in a MIDI sequence, specified by chunk, measure, beat, and tick. Notes that were already playing would continue to sustain.

2. **MdScan/mdScan**: Similar to jump, but would process all MIDI messages between the current position and the destination, ensuring that all configuration changes were properly interpreted.

3. **MdShareParts/mdShareParts**: A powerful feature that allowed two different sequences to share instrument parts, enabling smooth transitions between pieces.

### Parsing Implications

For anyone developing a parser for iMUSE MIDI files or extending a MIDI engine to support iMUSE capabilities, these details have several implications:

1. The parser must handle not only standard MIDI messages but also the custom system exclusive messages.

2. It must recognize the soundbundle format and be able to select the appropriate version based on capability.

3. It must interpret the specialized header chunk to set initial playback parameters.

4. Most critically, it must implement the hook and marker system to enable the interactive branching that defined iMUSE.

5. The parser should support musical time (measures, beats, ticks) rather than just absolute time, to properly implement jumps and scans.

With these extensions, iMUSE transforms standard MIDI files from linear recordings into complex interactive musical structures with conditional logic and real-time response capabilities.

### MIDI Implementation Details

The MIDI engine in iMUSE was particularly sophisticated for its time. According to Figure 3 of the patent, the MIDI module had the following internal structure:

INSERT FIGURE 3 MIDI MODULE DIAGRAM

This detailed diagram shows how the MIDI module was organized:

**Left Side (Command Flow)**:
- **File Manager (108)**: Manages sound file access and retrieval
- **Time Control (106)**: Provides timing services and interrupts
- **Command Interface (104)**: Routes commands from the system
- **MIDI Module Interface (120)**: Entry point for all MIDI-related commands

**Bottom Row (Processing Chain)**:
- **Command Queue (122)**: Stores commands for delayed execution
- **Sequencer (124)**: Extracts musical events from the MIDI file
- **Event Generator (126)**: Generates fade events over time

**Right Side (Sound Production)**:
- **MIDI Parser (128)**: Interprets MIDI messages
- **Players (130)**: Manages player-sequencer pairs
- **Hooks (132)**: Handles conditional branching
- **Parts (134)**: Manages instrument parts and parameters

**Top Area (Output)**:
- **Instrument Interface (136)**: Drives the synthesizer hardware
- Contains **Hardware**, **Tones**, and **Patches** components

This architecture illustrates the complete processing chain from command input to sound output, showing how all the components interact to interpret musical data and respond to game events.

The MIDI engine provided these core functions:

```c
extern int32 MdInitialize(InitDataPtr idp);
extern int32 MdTerminate(void);
extern int32 MdPause(void);
extern int32 MdResume(void);
extern int32 MdSave(char *buf, int32 size);
extern int32 MdRestore(char *buf);
extern int32 MdUpdateVols(void);
extern int32 MdPrepareSound(int32 sound, int32 flag);
extern int32 MdStartSound(int32 sound);
extern int32 MdStopSound(int32 sound);
extern int32 MdStopAllSounds(void);
extern int32 MdGetNextSound(int32 sound);
extern int32 MdSetSoundParam(int32 sound, int32 param, int32 val);
extern int32 MdGetSoundParam(int32 sound, int32 param);
extern int32 MdSetHook(int32 sound, int32 val);
extern int32 MdGetHook(int32 sound);
```

This enabled the game to synchronize events with musical timing by querying the current measure, beat, and tick position. The `MdJump` function allowed jumping to specific musical positions rather than time positions, which was key to maintaining musical coherence during transitions.

### Parameter Controls

The system supported various parameters that could be controlled at runtime.


### Parameter Controls

The system supported various parameters that could be controlled at runtime:

#### Sound Priority (`soundPriority`)
Sound priority determined which sounds would play when the system reached resource limitations. From the source, we can see priority was a critical parameter for managing limited audio resources:

- Higher priority sounds would play instead of lower priority ones
- When a new sound was started, it could pre-empt an existing lower priority sound
- The fade system allowed for gradually changing priorities (via `fadePriority`)
- MIDI channels had part-specific priorities (`midiPartPriority`)

In practical terms, this meant:
- Background music might have lower priority than critical dialogue or sound effects
- Less important ambient sounds could be dropped when more important sounds needed to play
- The system could dynamically reprioritize sounds based on gameplay context

Other key parameters included:

- Volume (`soundVol`): Controls amplitude, 0-127 range
- Pan (`soundPan`): Controls stereo positioning
- Detune (`soundDetune`): Absolute pitch adjustment
- Transpose (`soundTranspose`): Relative pitch adjustment in semitones
- MIDI-specific parameters like playback speed (`midiSpeed`)
- Part-specific parameters (`midiPartTrim`)

### Group Volume Busses

The iMUSE system organized sounds into hierarchical volume groups, or busses:

```c
enum {
    groupMaster   = 0,   /* master */
    groupSfx      = 1,   /* sound effects */
    groupVoice    = 2,   /* voice */
    groupMusic    = 3,   /* music */
    groupMusicDip = 4    /* auto dip music */
};
```

This allowed for master control of entire categories of sounds, such as ducking - automatically lowering music volume when dialogue played.

The implementation reveals how these volumes were calculated:

```c
int32 GrSetGroupVol(int32 group, int32 vol)
{
    int32 x;
    int32 oldVol;

    if ((ulong)group >= GROUP_COUNT)
        return (imArgErr);

    if (vol == -1)                          /* query */
        return (groupVols[group]);

    if ((ulong)vol > 127)
        return (imArgErr);

    oldVol = groupVols[group];

    if (group == 0) {                       /* master vol */
        groupVols[0] = vol;
        groupEffVols[0] = vol;
        for (x = 1; x < GROUP_COUNT; x++)
            groupEffVols[x] = ((groupVols[x] + 1) * vol) >> 7;
    }

    else {                                  /* group vol */
        groupVols[group] = vol;
        groupEffVols[group] = ((vol + 1) * groupVols[0]) >> 7;
    }

    MdUpdateVols();
    WvUpdateVols();

    return (oldVol);
}
```

When setting the master volume (group 0), all other group effective volumes were recalculated. Similarly, when setting a specific group's volume, its effective volume was calculated based on the master volume. This created a hierarchical volume system where changes to higher-level groups affected all sounds within those groups.

### Fades System

The fades system managed smooth transitions for parameters such as volume, pan, and playback speed:

```c
int32 FdFade(int32 sound, int32 param, int32 val, int32 time)
```

This function would calculate the necessary slope and modulo for a parameter transition, ensuring smooth changes even with limited CPU resources. The possible parameters that could be faded included:

```c
enum {
    fadePriority        = 0,  /* priority */
    fadeVol             = 1,  /* volume */
    fadePan             = 2,  /* pan */
    fadeDetune          = 3,  /* detune */
    fadeMidiSpeed       = 4,  /* MIDI speed */
    fadeMidiPartTrim    = 5,  /* MIDI part trim 1 */
    fadeMidiPartTrim16  = 20, /* MIDI part trim 16 */
    fadeUndefined       = 21  /* boundary */
};
```

The implementation used a clever algorithm to approximate fractional changes with integer math:

```c
fdp->slope = height / time;           // Integer division for main slope
fdp->slopeMod = height % time;        // Remainder for fractional part
fdp->modOvfloCounter = 0;             // Running counter for remainder

// In the interrupt handler:
val = fdp->currentVal + fdp->slope;   // Apply main slope
if ((fdp->modOvfloCounter += fdp->slopeMod) >= fdp->length) {
    fdp->modOvfloCounter -= fdp->length;
    val += fdp->nudge;                // Apply fractional adjustment
}
```

This approach allowed for smooth fades even on systems with limited floating-point capabilities, by using a remainder accumulation technique similar to Bresenham's line algorithm.

### Interrupt-Based Processing

iMUSE relied heavily on timer interrupts (running at approximately 60Hz) to update fades, process deferred commands, and handle other time-sensitive operations. From `time.c`:

```c
int32 TcStartInts(void) {
    gIntTimer = WinLibAllocateTimer(USEC_PER_INT, TcIntHandler, NULL);
    return (0);
}
```

### Composition Database Formation and Utilization

Section 5 of the original patent documentation offers valuable insights about how composers actually worked with iMUSE to create interactive soundtracks[¹]. This section explores the practical aspects of creating and implementing music for this system.

### Composing and Conditionalizing MIDI Sequences

The method for forming the composition database followed these steps:

1. **Initial Composition**: A human composer first created one or more sequences using standard MIDI hardware and software. These sequences were structured according to the demands of the particular application (e.g., the plot and action segments of a game).

2. **Conditionalizing Process**: The composer then conditionalized the raw musical material by determining aesthetically appropriate ways for the music to react to actions of the directing system. This involved:

   - **Control Instructions**: Creating clear instructions for the directing system explaining how to issue appropriate commands to the sound driver.
   
   - **Database Modifications**: Making modifications to the composition database itself to accommodate conditional behavior.
   
   - **Decision Point Placement**: Identifying where decision points should be located and what conditional actions might take place there.
   
   - **Hook and Marker Implementation**: Creating and inserting hook or marker messages at decision points with specific IDs.

3. **Conditionalizing Strategies**: Several approaches were possible:

   a. **Direct Commands**: Simple control instructions telling the directing system how to issue commands to start, stop, or modify music.
   
   b. **Complex Direct Commands**: Instructions requiring detailed knowledge of the composition database, such as when to enable/disable specific instrument parts.
   
   c. **Database Modification**: Altering the composition database to accommodate the operation of direct commands, such as adjusting performance data for different combinations of instrument parts.
   
   d. **Condition Setting**: Having the directing system set conditions that the sound driver would evaluate at decision points, using hooks and markers.

The result of this process was an interlinked set of sequences that could branch, converge, and loop in numerous ways as dictated by gameplay.

### Example: Assembling a Sound Track

Let's examine a detailed example of how a composition database would be created for an action scene in a video game[¹]:

1. **Main Sequences**: The composer would create one or more "main" sequences to provide primary background music, designed to be started at the beginning of an action scene and looped until the end.

2. **Conditional Parts**: Some instrument parts would play continuously, while others would be conditionally enabled/disabled depending on specific events in the scene.

3. **Hook Messages**: The composer would insert hook messages between each musical phrase of each instrument part, allowing for conditional changes only at musically appropriate moments.

4. **Jump Hooks**: Jump hook messages would be placed at musically appropriate points in the main sequence, allowing the music to conditionally jump to other sequences.

5. **Transition Sequences**: The composer would create "transition sequences" to provide musically graceful transitions from a source sequence to a destination sequence.

6. **Termination Sequences**: Special sequences would be composed for the end of major scenes or actions.

This approach created an interwoven set of sequences which could be conditionally traversed by the sound driver, resulting in a dynamic, responsive soundtrack that maintained musical coherence regardless of player actions.

### Practical Example: A Fight Scene

The fight scene example illustrates how iMUSE created sophisticated interactive music[¹]:

1. **Layered Composition**: A continuously looped music sequence with four instrument parts - two playing throughout the fight, one playing when a specific fighter is winning, and one when that fighter is losing.

2. **Musical Phrases**: Parts are divided into musical phrases that must be played without interruption to preserve natural flow.

3. **Dynamic Control**: As the fight progresses, hook messages enable or disable parts at musically appropriate points between phrases.

4. **Graceful Transitions**: When the fight ends, the system doesn't abruptly switch to victory or defeat music. Instead, it:
   - Enables the appropriate jump hook message
   - Enqueues a trigger with the appropriate marker ID
   - Enqueues a command to start either victory or defeat music
   - Jumps to a transitional sequence when it reaches an enabled jump hook
   - Plays the transitional music
   - Starts the victory or defeat music when it encounters the marker at the end of the transition


### State Management

iMUSE included robust state saving and restoration:

```c
int32 ImSave(char *buf, int32 size)
int32 ImRestore(char *buf)
```

This allowed game states (including all active sounds, fades, and triggers) to be saved and restored, essential for game save/load functionality.

____
## Building iMuse Today

Creating a modern iMUSE-like system would involve several components that improve on the original authoring workflow:

1. **Interactive Music Composition Tools**:
   - DAW plugins for defining interactive segments and transition points
   - Visual tools for creating music state machines and transition rules
   - Real-time preview of transitions without requiring game integration

2. **Conditionalizing Support**:
   - Tools specifically designed to support the process of conditionalizing music
   - Visual representation of decision points and possible branches
   - Automated validation of transition compatibility

3. **Control Instructions Integration**:
   - Formalized method for composers to document control instructions
   - Integration of these instructions directly into middleware
   - Translation tools to convert musical concepts to programming concepts

4. **Integration with Industry-Standard DAWs**:
   - Support for exporting from Logic, Cubase, Ableton, etc.
   - Preservation of metadata like markers, tempo maps, and section information
   - Middleware plugins that understand interactive music concepts

5. **Game Engine Integration**:
   - Visual node editors for mapping game events to musical responses
   - In-editor auditioning of transitions and musical branches
   - Runtime debugging tools to monitor music state and transitions

6. **Enhanced Audio Capabilities**:
   - High-quality runtime DSP for seamless cross-fades and time-stretching
   - Support for layered stems that can be individually controlled
   - Advanced mixing capabilities with dynamic mixing rules

### Key Components to Implement

1. **Sound Manager**:
   - Handle loading, unloading, and pooling audio resources
   - Coordinate multiple audio engines (similar to iMUSE's MIDI and WAVE engines)

2. **Parameter System**:
   - Provide real-time control over audio parameters (volume, pitch, effects)
   - Support smoothed transitions (fades) with curve types beyond linear

3. **Marker and Trigger System**:
   - Parse metadata in audio files for markers (modern formats support this natively)
   - Connect game events to audio markers, and vice versa
   - Support timelines with precise timing for scripted events

4. **Transition Engine**:
   - Define rules for transitioning between music states
   - Support stingers, crossfades, and other transition types
   - Maintain musical beat/bar synchronization during transitions

5. **State Management**:
   - Support serialization of the entire audio state
   - Allow for hot-reloading during development

### Modern Advantages

1. **Higher processing power**: Support for more complex algorithms and real-time DSP
2. **Better audio formats**: Use of compressed, high-quality formats with embedded metadata
3. **Advanced DSP**: Real-time effects processing and filtering
4. **Middleware integration**: Connect with industry-standard tools
5. **Network capabilities**: Distributed audio processing or streaming

___
## Example Modern Implementation (Pseudo-code)

Here's how a modern hook/trigger system might be implemented:

```typescript
// Music segment with hooks
class MusicSegment {
    audioClip: AudioClip;
    markers: Map<string, number>; // marker name -> time in seconds
    hooks: Map<number, TransitionRule[]>; // hook point -> possible transitions
    
    triggerHook(hookId: number, context: GameContext): boolean {
        const rules = this.hooks.get(hookId) || [];
        for (const rule of rules) {
            if (rule.evaluate(context)) {
                rule.execute(context);
                return true;
            }
        }
        return false;
    }
}

// Game-side trigger system
class MusicTriggerSystem {
    activeSegment: MusicSegment | null = null;
    queuedCommands: Array<{time: number, command: () => void}> = [];
    
    update(deltaTime: number) {
        // Process time-based commands (similar to iMUSE's deferred commands)
        this.queuedCommands.forEach(cmd => {
            cmd.time -= deltaTime;
            if (cmd.time <= 0) {
                cmd.command();
                return false; // remove from queue
            }
            return true; // keep in queue
        });
    }
    
    setTrigger(markerName: string, callback: () => void) {
        if (!this.activeSegment) return;
        
        const markerTime = this.activeSegment.markers.get(markerName);
        if (markerTime !== undefined) {
            const timeUntilMarker = markerTime - this.audioPlayer.currentTime;
            if (timeUntilMarker > 0) {
                this.queuedCommands.push({
                    time: timeUntilMarker,
                    command: callback
                });
            }
        }
    }
}
```

## Conclusion

iMUSE represented a revolutionary approach to interactive music, providing capabilities that many modern game audio systems still strive to match. The clever use of hooks, triggers, and parameter control allowed for dynamic musical experiences despite the limited hardware and software capabilities of the early 1990s.

The core concepts defined in the original system - the directing system, composition database, control instructions, and the conditionalizing process - remain highly relevant today. The clear separation between the composition process (which happens a priori) and the dynamic playback (controlled by the directing system) established a paradigm that continues to influence interactive audio design.

What made iMUSE particularly powerful was its bidirectional communication model:

- The directing system could control the sound driver through direct commands
- The sound driver could notify the directing system through callbacks when musical events occurred

This established an interactive relationship between music and gameplay, where neither was subordinate to the other.

A modern implementation of iMUSE would follow many of the same principles while taking advantage of current technologies to provide sophisticated interactive audio experiences. The original concepts - especially the role of the composer in conditionalizing music and providing control instructions - are key to capturing the full power of the system's approach to interactive music.

## References

[1] Land, M. Z., & McConnell, P. N. (1994). U.S. Patent No. 5,315,057. Washington, DC: U.S. Patent and Trademark Office. "Method and apparatus for dynamically composing music and sound effects using a computer entertainment system."

https://en.wikipedia.org/wiki/IMUSE
https://www.academia.edu/97212316/The_Legacy_of_iMuse_Interactive_Video_Game_Music_in_the_1990s
https://www.youtube.com/watch?v=nRHlPRQPWZ4
https://www.audiogang.org/interview-with-lucasarts-jesse-harlin/

https://theforceengine.github.io/2022/05/17/iMuseAndSoundRelease.html
https://github.com/AndywinXp/DiMUSE-Notes
https://github.com/Jither/iMUSE-Sequencer 
https://github.com/vgmstream/vgmstream/blob/master/src/meta/imuse.c
https://github.com/vgmstream/vgmstream/blob/master/src/coding/imuse_decoder.c
https://github.com/scummvm/scummvm/blob/master/engines/scumm/imuse_digi/dimuse_codecs.cpp (V1)
https://github.com/residualvm/residualvm/tree/master/engines/grim/imuse
https://github.com/residualvm/residualvm/tree/master/engines/grim/movie/codecs (V2)
https://wiki.multimedia.cx/index.php/VIMA (V2)
