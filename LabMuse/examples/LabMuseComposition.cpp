//
//  LabMuseComposition.cpp
//
//  Builds the "Die Toteninsel" composition database programmatically -- an
//  A-minor, Lento, 5/8 tone poem after Rachmaninoff's "Isle of the Dead".
//
//  The whole piece is a single looping master sound (kMasterSound).  Each
//  thematic layer is a *part* -- a MIDI channel -- of that one sequence, so
//  every layer shares one musical clock.  Laying a theme in or out is just a
//  part-trim change; because the parts are phase-locked to the master, a theme
//  laid in at bar 8 is already rhythmically aligned rather than starting its
//  own bar 1.  Bar markers on every 5/8 bar line give the director a musical
//  boundary to quantize transitions against.
//
//  A separate one-shot stinger (kStingerSound) is a second, independent
//  sequencer started with imStartSound -- demonstrating true concurrent
//  player-sequencer pairs sharing the one soft synth.
//
/*
┌──────────────────────────────────────────────────────────────────────────┐
│ ▓▓▓▒▒▒ PROVENANCE NODE [6] // ZONE: [HYBRID_REMIXABLE]                   │
├──────────────────────────────────────────────────────────────────────────┤
│ AUTH_ID  : [Nick Porcino]         | GEN_TOOL : [Claude Opus 4.8]         │
│ SRC_AUTH : [HMN]                  │ SIG_STAT : [UNSIGNED]                │
| INCEPT © : [Tues July 14 08:58:06 2026 -0800]                            |
├──────────────────────────────────────────────────────────────────────────┤
│ INDEX_ALLOW : [YES]               │ CORP_TRAIN: [NO]                     │
│ DERIV_ALLOW : [YES]               │ GOV_SPDX  : [SPDX: BSD 2-Clause]     │
└──────────────────────────────────────────────────────────────────────────┘
*/

#include "DieToteninsel.h"

#include <cctype>
#include <cstring>

namespace Lab {
namespace Muse {

namespace {

// --- Musical grid ---------------------------------------------------------
const int kTicksPerQuarter = 240;
const int kEighth          = 120;   // one beat in 5/8
const int kMeter           = 5;     // eighths per bar
const int kBarTicks        = kMeter * kEighth;   // 600
const int kLoopBars        = 8;
const int kLoopLen         = kLoopBars * kBarTicks;   // 4800
const double kTempoBPM     = 52.0;  // Lento (quarter-note BPM)

// --- General MIDI programs (0-based) --------------------------------------
enum {
    gmContrabass      = 43,
    gmCello           = 42,
    gmViola           = 41,
    gmChoirAahs       = 52,
    gmTremoloStrings  = 44,
    gmCelesta         = 8,
    gmTubularBells    = 14
};

struct Note { int tick; int note; int dur; int vel; };

void addNotes(LabMuse& m, int sound, int ch, const Note* n, int count)
{
    for (int i = 0; i < count; ++i)
        m.addNote(sound, ch, n[i].tick, n[i].dur, n[i].note, n[i].vel);
}

// --- Ostinato (channel 0): the undulating 5/8 rocking figure, always on ----
void buildOstinato(LabMuse& m)
{
    // i i iv iv VI VI V V, in a low register.
    const int roots[kLoopBars]  = { 45, 45, 38, 38, 41, 41, 40, 40 };
    const int thirds[kLoopBars] = {  3,  3,  3,  3,  4,  4,  4,  4 };
    // eighth pattern: root, fifth, octave, third, fifth
    for (int bar = 0; bar < kLoopBars; ++bar) {
        int base = bar * kBarTicks;
        int r = roots[bar];
        int offs[5] = { 0, 7, 12, thirds[bar], 7 };
        for (int e = 0; e < 5; ++e)
            m.addNote(kMasterSound, partOstinato,
                      base + e * kEighth, kEighth - 10, r + offs[e], 60);
    }
}

// --- Lament (channel 1): a mournful cello line ----------------------------
void buildLament(LabMuse& m)
{
    static const Note lament[] = {
        {   0, 64, 360, 60 }, { 360, 62, 240, 58 }, { 600, 60, 480, 62 },
        {1080, 59, 120, 55 }, {1200, 57, 600, 60 }, {1800, 55, 300, 56 },
        {2100, 53, 300, 54 }, {2400, 53, 480, 60 }, {2880, 52, 120, 55 },
        {3000, 52, 600, 58 }, {3600, 55, 360, 58 }, {3960, 57, 240, 60 },
        {4200, 59, 300, 60 }, {4500, 64, 300, 64 },
    };
    addNotes(m, kMasterSound, partLament, lament, int(sizeof(lament)/sizeof(Note)));
}

// --- Viola (channel 2): sustained inner harmony ---------------------------
void buildViola(LabMuse& m)
{
    const int pitch[kLoopBars] = { 52, 52, 53, 53, 57, 57, 56, 56 };
    for (int bar = 0; bar < kLoopBars; ++bar)
        m.addNote(kMasterSound, partViola, bar * kBarTicks, kBarTicks, pitch[bar], 44);
}

// --- Dies Irae (channel 3): the chant motif in the choir ------------------
void buildDiesIrae(LabMuse& m)
{
    static const Note dies[] = {
        {   0, 57, 600, 66 }, { 600, 57, 600, 66 }, {1200, 55, 600, 68 },
        {1800, 57, 600, 68 }, {2400, 53, 600, 70 }, {3000, 55, 600, 70 },
        {3600, 52, 900, 72 }, {4500, 52, 300, 68 },
    };
    addNotes(m, kMasterSound, partDiesIrae, dies, int(sizeof(dies)/sizeof(Note)));
}

// --- Agitato (channel 4): driving tremolo eighths -------------------------
void buildAgitato(LabMuse& m)
{
    const int pat[5] = { 64, 67, 64, 69, 67 };   // E G E A G
    for (int bar = 0; bar < kLoopBars; ++bar) {
        int base = bar * kBarTicks;
        for (int e = 0; e < 5; ++e)
            m.addNote(kMasterSound, partAgitato,
                      base + e * kEighth, kEighth - 8, pat[e], 72);
    }
}

// --- Shimmer (channel 5): sparse high celesta glints ----------------------
void buildShimmer(LabMuse& m)
{
    const int hi[kLoopBars] = { 76, 79, 81, 79, 84, 81, 79, 76 };
    for (int bar = 0; bar < kLoopBars; ++bar) {
        int base = bar * kBarTicks;
        m.addNote(kMasterSound, partShimmer, base + 2 * kEighth, kEighth, hi[bar], 40);
        m.addNote(kMasterSound, partShimmer, base + 4 * kEighth, kEighth,
                  hi[bar] + 3, 36);
    }
}

// --- The one-shot stinger: a tolling bell flourish ------------------------
void buildStinger(LabMuse& m)
{
    m.createSound(kStingerSound, groupSfx,
                  kTicksPerQuarter, kTempoBPM, kMeter, kEighth);
    m.definePart(kStingerSound, 6, "stinger:bells", gmTubularBells, 127);

    static const Note bells[] = {
        {   0, 81, 720, 95 }, { 480, 76, 720, 88 },
        { 960, 69, 960, 82 }, { 960, 57, 1440, 75 },
    };
    addNotes(m, kStingerSound, 6, bells, int(sizeof(bells)/sizeof(Note)));
    // Non-looping: plays once and stops (engine flushes held notes at the end).
}

} // anonymous namespace

void BuildDieToteninsel(LabMuse& m)
{
    m.createSound(kMasterSound, groupMusic,
                  kTicksPerQuarter, kTempoBPM, kMeter, kEighth);

    // Define parts (channel, name, GM program, initial trim).  Only the
    // ostinato is on at the start; the thematic layers wait to be laid in.
    m.definePart(kMasterSound, partOstinato, "ostinato",  gmContrabass,     100);
    m.definePart(kMasterSound, partLament,   "lament",    gmCello,            0);
    m.definePart(kMasterSound, partViola,    "viola",     gmViola,            0);
    m.definePart(kMasterSound, partDiesIrae, "dies-irae", gmChoirAahs,        0);
    m.definePart(kMasterSound, partAgitato,  "agitato",   gmTremoloStrings,   0);
    m.definePart(kMasterSound, partShimmer,  "shimmer",   gmCelesta,          0);

    buildOstinato(m);
    buildLament(m);
    buildViola(m);
    buildDiesIrae(m);
    buildAgitato(m);
    buildShimmer(m);

    // Bar markers on every 5/8 bar line -- the quantization grid.
    for (int bar = 0; bar < kLoopBars; ++bar)
        m.addMarker(kMasterSound, bar * kBarTicks, kBarMarker);

    // Loop the whole master forever.
    m.setLoop(kMasterSound, 0, kLoopLen);

    buildStinger(m);
}

//----------------------------------------------------------------------------
// Device voice profiles
//----------------------------------------------------------------------------

namespace {

// Yamaha DX-7, stock ROM1A cartridge voice assignments (program numbers are
// 0-based).  The original DX-7 ignores bank select, so bank fields are -1.
// These are best-effort factory-slot picks per role; edit freely to taste.
const VoiceAssignment kDX7Voices[] = {
    { "ostinato",      15, -1, -1 },  // BASS 1
    { "lament",         3, -1, -1 },  // STRINGS 1
    { "viola",          4, -1, -1 },  // STRINGS 2
    { "dies-irae",     30, -1, -1 },  // VOICE 1  (choir-like)
    { "agitato",        5, -1, -1 },  // STRINGS 3
    { "shimmer",       26, -1, -1 },  // TUB BELLS
    { "stinger:bells", 26, -1, -1 },  // TUB BELLS
};

// Port-name aliases.  Both Yamahas report "Yamaha", so match specific tokens.
const char* const kDX7Match[]    = { "dx7", "dx-7" };
const char* const kCLP840Match[] = { "clp-840", "clp840", "clp" };

// Registry of known non-default devices.  Add an entry to cover another synth.
const DeviceProfile kProfiles[] = {
    { "Yamaha DX-7 (stock ROM1A)",
      kDX7Match, int(sizeof(kDX7Match) / sizeof(char*)),
      kDX7Voices, int(sizeof(kDX7Voices) / sizeof(VoiceAssignment)) },

    // The CLP-840 Clavinova is General-MIDI compatible, so the score's own
    // voices (contrabass, cello, choir, ...) are already right -- no remap
    // needed.  Recognized here for a friendly label; drop a voice table in if
    // you'd rather drive its native panel voices.
    { "Yamaha Clavinova CLP-840 (General MIDI)",
      kCLP840Match, int(sizeof(kCLP840Match) / sizeof(char*)),
      nullptr, 0 },
};

// The fallback: General MIDI, i.e. keep the voices the score already declared.
const DeviceProfile kDefaultProfile = {
    "General MIDI (as composed)", nullptr, 0, nullptr, 0
};

bool containsNoCase(const char* haystack, const char* needle)
{
    if (!haystack || !needle || !*needle)
        return false;
    size_t nlen = std::strlen(needle);
    for (const char* p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < nlen && p[i] &&
               std::tolower((unsigned char)p[i]) ==
               std::tolower((unsigned char)needle[i]))
            ++i;
        if (i == nlen)
            return true;
    }
    return false;
}

void applyProfileToSound(LabMuse& m, int sound, const DeviceProfile& profile)
{
    for (int i = 0; i < m.partCount(sound); ++i) {
        int ch, trim, ptrim; const char* name; bool pend;
        if (!m.partInfo(sound, i, ch, name, trim, pend, ptrim))
            continue;
        for (int v = 0; v < profile.voiceCount; ++v) {
            if (std::strcmp(name, profile.voices[v].role) == 0) {
                m.setPartInstrument(sound, ch, profile.voices[v].program,
                                    profile.voices[v].bankMSB,
                                    profile.voices[v].bankLSB);
                break;
            }
        }
    }
}

} // anonymous namespace

const DeviceProfile& ProfileForPort(const char* portName)
{
    for (const DeviceProfile& p : kProfiles)
        for (int i = 0; i < p.matchCount; ++i)
            if (containsNoCase(portName, p.match[i]))
                return p;
    return kDefaultProfile;
}

void ApplyDeviceProfile(LabMuse& m, const DeviceProfile& profile)
{
    if (profile.voiceCount == 0 || !profile.voices)
        return;   // General MIDI: leave the score's voices untouched
    applyProfileToSound(m, kMasterSound,  profile);
    applyProfileToSound(m, kStingerSound, profile);
}

} // namespace Muse
} // namespace Lab
