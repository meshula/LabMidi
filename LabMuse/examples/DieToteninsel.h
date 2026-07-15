//
//  DieToteninsel.h
//
//  Declarations for the "Die Toteninsel" example composition.  These are
//  specific to this one piece, not to the LabMuse engine, so they live with
//  the example rather than in the public library header.
//
/*
┌──────────────────────────────────────────────────────────────────────────┐
│ ▓▓▓▒▒▒ PROVENANCE NODE [7] // ZONE: [HYBRID_REMIXABLE]                   │
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

#include <LabMuse/LabMuse.h>

namespace Lab {
namespace Muse {

// Populates 'muse' with the "Die Toteninsel" composition database:
// a looping A-minor Lento 5/8 master sound (id kMasterSound) whose parts are
// the ostinato + thematic layers, plus a one-shot stinger (id kStingerSound).
void BuildDieToteninsel(LabMuse& muse);

// Well-known ids and the master's bar-marker id used by the demo/self-test.
static const int kMasterSound  = 1;
static const int kStingerSound = 2;
static const int kBarMarker    = 1;   // placed on every 5/8 bar line

// Master part channels (channel 0 is the always-on ostinato).
enum MasterPart {
    partOstinato = 0,  // always on
    partLament   = 1,  // key '1'
    partViola    = 2,  // key '2'
    partDiesIrae = 3,  // key '3'
    partAgitato  = 4,  // key '4'
    partShimmer  = 5   // key '5'
};

//----------------------------------------------------------------------------
// Device voice profiles.
//
// The composition is authored in General MIDI (see definePart calls).  A
// non-GM instrument -- e.g. a Yamaha DX-7 with its stock ROM cartridge -- uses
// different program numbers, so we remap voices per output device rather than
// re-authoring the score.
//
// A profile maps a part *role* (matched against the part name from the score)
// to a device-specific bank/program.  Profiles are selected by matching any of
// a list of case-insensitive substrings against the chosen output port's name.
// This table is intentionally data-driven so covering another synth is just
// another entry -- no engine or composition changes required.
//
// Aliases matter once you own more than one device from the same maker: both a
// DX-7 and a CLP-840 report "Yamaha", so each profile matches a *specific*
// token ("dx7", "clp") rather than the shared brand.
//----------------------------------------------------------------------------

struct VoiceAssignment {
    const char* role;      // matches a LabMuse part name, e.g. "dies-irae"
    int         program;   // device program-change number (0-based)
    int         bankMSB;   // CC0  ; -1 = don't send
    int         bankLSB;   // CC32 ; -1 = don't send
};

struct DeviceProfile {
    const char*            name;        // human-readable
    const char* const*     match;       // case-insensitive port-name substrings
    int                    matchCount;  // 0 => default (matches nothing)
    const VoiceAssignment* voices;      // null/empty => keep the GM score voices
    int                    voiceCount;
};

// The first profile any of whose match substrings occurs (case-insensitively)
// in portName, or the default General-MIDI profile if none match (or portName
// is null).
const DeviceProfile& ProfileForPort(const char* portName);

// Apply a profile to the already-built composition: for every part whose role
// matches an entry, override its instrument (across all sounds that use it).
void ApplyDeviceProfile(LabMuse& muse, const DeviceProfile& profile);

} // namespace Muse
} // namespace Lab
