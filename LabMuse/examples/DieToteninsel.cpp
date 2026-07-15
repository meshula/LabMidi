//
//  DieToteninsel.cpp
//
//  A terminal toy for LabMuse: a Rachmaninoff "Isle of the Dead"-inspired tone
//  poem in 5/8.  A continuously looping A-minor ostinato plays; pressing keys
//  lays thematic layers in and out -- always quantized to the next 5/8 bar line
//  via iMUSE-style markers and triggers, so the music never jumps mid-phrase.
//
//  Run with no arguments for the interactive audio demo.  Run with --selftest
//  for a headless (no audio, no TTY) verification of the quantization, fade,
//  pause, and loop-wrap machinery -- safe to run in a sandbox.
//
/*
┌──────────────────────────────────────────────────────────────────────────┐
│ ▓▓▓▒▒▒ PROVENANCE NODE [8] // ZONE: [HYBRID_REMIXABLE]                   │
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

#include <LabMidi/LabMidi.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <sys/time.h>
#include <unistd.h>

#if !defined(_MSC_VER)
#include <termios.h>
#include <signal.h>
#endif

using namespace Lab::Muse;

static double nowSeconds()
{
    timeval t;
    gettimeofday(&t, nullptr);
    return double(t.tv_sec) + double(t.tv_usec) * 1.0e-6;
}

//============================================================================
// Terminal raw-mode handling (interactive mode only).
//============================================================================

#if !defined(_MSC_VER)
static struct termios g_savedTermios;
static bool           g_termiosSaved = false;

static void restoreTerminal()
{
    if (g_termiosSaved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTermios);
        g_termiosSaved = false;
        // Show cursor again.
        printf("\x1b[?25h\n");
        fflush(stdout);
    }
}

static void onSignal(int)
{
    restoreTerminal();
    _exit(0);
}

static void enterRawMode()
{
    if (tcgetattr(STDIN_FILENO, &g_savedTermios) != 0)
        return;
    g_termiosSaved = true;
    atexit(restoreTerminal);
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    struct termios raw = g_savedTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 0;   // non-blocking read
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    printf("\x1b[?25l");   // hide cursor
}

static int readKey()
{
    unsigned char ch;
    ssize_t n = read(STDIN_FILENO, &ch, 1);
    return n == 1 ? int(ch) : -1;
}
#endif

//============================================================================
// Shared helpers
//============================================================================

// The layer trim used when a part is laid in.
static const int kLayInTrim = 100;

// Is the given master channel currently on (or pending on)?
static bool partIsOn(const LabMuse& m, int channel, bool& pending, int& trim)
{
    pending = false;
    trim = 0;
    for (int i = 0; i < m.partCount(kMasterSound); ++i) {
        int ch, t, ptrim; const char* name; bool pend;
        if (m.partInfo(kMasterSound, i, ch, name, t, pend, ptrim) && ch == channel) {
            pending = pend;
            trim = t;
            return pend ? (ptrim > 0) : (t > 0);
        }
    }
    return false;
}

// Queue a lay-in / lay-out of a part, quantized to the next bar marker.
static void togglePart(LabMuse& m, int channel)
{
    bool pending; int trim;
    bool on = partIsOn(m, channel, pending, trim);
    int target = on ? 0 : kLayInTrim;
    m.SoundCall(imSetTrigger, kMasterSound, kBarMarker, mdSetPartTrim,
                channel, target);
}

//============================================================================
// Interactive mode
//============================================================================

#if !defined(_MSC_VER)
static void printStatus(LabMuse& m, bool paused)
{
    int bar, beat, tick;
    m.musicalPosition(kMasterSound, bar, beat, tick);
    int barInLoop = ((bar - 1) % 8) + 1;

    // Clear line, then draw.
    printf("\r\x1b[2K");
    printf("bar %d:%d  tempo %d  speed %d%s | ",
           barInLoop, beat, int(m.tempo(kMasterSound)), m.speed(kMasterSound),
           paused ? "  [PAUSED]" : "");

    for (int i = 0; i < m.partCount(kMasterSound); ++i) {
        int ch, t, ptrim; const char* name; bool pend;
        if (!m.partInfo(kMasterSound, i, ch, name, t, pend, ptrim))
            continue;
        if (ch == partOstinato)
            continue;   // always on; not a toggle target

        const char* state;
        if (pend)          state = (ptrim > 0) ? "PEND-IN " : "PEND-OUT";
        else if (t > 0)    state = "IN      ";
        else               state = "out     ";
        printf("%d:%s[%s] ", ch, name, state);
    }
    if (m.soundActive(kStingerSound))
        printf(" *stinger*");
    fflush(stdout);
}

// Present the output menu, open the chosen sink, and return it (nullptr means
// "use the engine's built-in software synth").  Sets *useSoftSynth and *profile.
static Lab::MidiOut* chooseOutput(Lab::MidiPorts& ports,
                                  bool& useSoftSynth,
                                  const DeviceProfile*& profile,
                                  const char*& outName)
{
    ports.refreshPortList();
    int nOut = ports.outPorts();

    printf("Die Toteninsel -- LabMuse iMUSE demonstrator\n\n");
    printf("Select audio output:\n");
    printf("  0: Built-in software synth (Apple DLS General MIDI)\n");
    for (int i = 0; i < nOut; ++i)
        printf("  %d: %s\n", i + 1, ports.outPortCStr(i));
    printf("Choice [0]: ");
    fflush(stdout);

    int choice = 0;
    char line[128];
    if (fgets(line, sizeof(line), stdin) && line[0] != '\n' && line[0] != '\0')
        choice = std::atoi(line);

    useSoftSynth = (choice <= 0 || choice > nOut);
    profile = &ProfileForPort(nullptr);        // default: General MIDI
    outName = "Built-in software synth";

    if (useSoftSynth)
        return nullptr;

    int portIndex = choice - 1;
    Lab::MidiOut* midiOut = new Lab::MidiOut();
    if (!midiOut->openPort(portIndex)) {
        printf("Could not open that port; falling back to the software synth.\n");
        delete midiOut;
        useSoftSynth = true;
        return nullptr;
    }
    outName = ports.outPortCStr(portIndex);
    profile = &ProfileForPort(outName);
    return midiOut;
}

static int runInteractive()
{
    Lab::MidiPorts ports;
    bool useSoftSynth = true;
    const DeviceProfile* profile = nullptr;
    const char* outName = nullptr;
    Lab::MidiOut* midiOut = chooseOutput(ports, useSoftSynth, profile, outName);

    LabMuse muse(useSoftSynth);
    if (midiOut)
        muse.setOutput(midiOut);

    BuildDieToteninsel(muse);
    ApplyDeviceProfile(muse, *profile);

    printf("\nOutput: %s\nVoices: %s\n\n", outName, profile->name);

    enterRawMode();

    printf("Die Toteninsel -- LabMuse iMUSE demonstrator\n");
    printf("  1 lament   2 viola   3 dies-irae   4 agitato   5 shimmer\n");
    printf("  s stinger   [ / ] tempo(speed)   space pause   q quit\n");
    printf("  (layers enter/exit on the next 5/8 bar line)\n\n");

    double start = nowSeconds();
    muse.update(0.0);   // prime the timebase
    muse.SoundCall(imStartSound, kMasterSound);

    bool paused = false;
    bool run = true;
    double lastDraw = 0.0;

    while (run) {
        usleep(1000);   // ~1 ms
        double t = nowSeconds() - start;
        muse.update(t);

        int key;
        while ((key = readKey()) != -1) {
            switch (key) {
            case '1': case '2': case '3': case '4': case '5':
                togglePart(muse, key - '0');
                break;
            case 's': case 'S':
                muse.SoundCall(imStartSound, kStingerSound);
                break;
            case '[':
                muse.SoundCall(mdSetSpeed, kMasterSound,
                               std::max(16, muse.speed(kMasterSound) - 16));
                break;
            case ']':
                muse.SoundCall(mdSetSpeed, kMasterSound,
                               std::min(256, muse.speed(kMasterSound) + 16));
                break;
            case ' ':
                paused = !paused;
                muse.SoundCall(paused ? imPause : imResume);
                break;
            case 'q': case 'Q': case 27: /* ESC */
                run = false;
                break;
            default: break;
            }
        }

        if (t - lastDraw >= 0.05) {   // redraw at ~20 Hz
            printStatus(muse, paused);
            lastDraw = t;
        }
    }

    muse.SoundCall(imStopAllSounds);
    restoreTerminal();
    delete midiOut;   // safe: ~LabMuse does not emit to an external sink
    printf("Auf Wiedersehen.\n");
    return 0;
}
#endif

//============================================================================
// Headless self-test
//============================================================================

namespace {
    std::vector<Notification> g_log;

    void logNotify(void*, const Notification& n) { g_log.push_back(n); }

    int  g_failures = 0;

    void check(bool cond, const char* what)
    {
        printf("  [%s] %s\n", cond ? "PASS" : "FAIL", what);
        if (!cond) ++g_failures;
    }

    const char* notifyName(Notification::Type t)
    {
        switch (t) {
        case Notification::Marker:       return "Marker";
        case Notification::TriggerFired: return "TriggerFired";
        case Notification::DeferFired:   return "DeferFired";
        case Notification::PartTrim:     return "PartTrim";
        case Notification::LoopWrap:     return "LoopWrap";
        case Notification::SoundStarted: return "SoundStarted";
        case Notification::SoundStopped: return "SoundStopped";
        }
        return "?";
    }
}

static int runSelfTest()
{
    printf("LabMuse self-test (headless, no audio)\n");
    printf("======================================\n");

    LabMuse muse(false);   // no synth
    BuildDieToteninsel(muse);
    muse.setNotify(logNotify, nullptr);

    // Virtual clock.
    double t = 0.0;
    const double dt = 0.005;
    auto advanceTo = [&](double target) {
        while (t < target) {
            t += dt;
            muse.update(t);
        }
    };

    muse.update(0.0);            // prime
    muse.SoundCall(imStartSound, kMasterSound);

    // A 5/8 bar at 52 BPM quarter is 2.5 quarters == ~2.885 s.
    const double barSeconds = (2.5 / 52.0) * 60.0;
    printf("bar length ~= %.3f s\n\n", barSeconds);

    //-- Test 1: lay a theme IN, quantized to the next bar ------------------
    printf("Test 1: quantized lay-in of dies-irae\n");
    advanceTo(t + 1.0);                    // sit inside a bar
    g_log.clear();
    muse.SoundCall(imSetTrigger, kMasterSound, kBarMarker, mdSetPartTrim,
                   partDiesIrae, kLayInTrim);
    advanceTo(t + barSeconds + 0.5);       // cross at least one bar line

    bool firedOnBar = false;
    for (const Notification& n : g_log) {
        if (n.type == Notification::TriggerFired && n.part == mdSetPartTrim) {
            printf("    trigger fired at bar-in-piece %d, beat %d, tick %d\n",
                   n.bar, n.beat, n.tick);
            firedOnBar = (n.beat == 1 && n.tick == 0);
            break;
        }
    }
    check(firedOnBar, "lay-in trigger fires exactly on a 5/8 bar line");
    check(muse.SoundCall(mdGetParam, kMasterSound, midiPartTrim, partDiesIrae)
              == kLayInTrim,
          "dies-irae part trim is now laid in");

    //-- Test 2: lay the theme OUT, also quantized --------------------------
    printf("Test 2: quantized lay-out of dies-irae\n");
    advanceTo(t + 0.4);
    g_log.clear();
    muse.SoundCall(imSetTrigger, kMasterSound, kBarMarker, mdSetPartTrim,
                   partDiesIrae, 0);
    advanceTo(t + barSeconds + 0.5);

    bool outOnBar = false;
    for (const Notification& n : g_log) {
        if (n.type == Notification::TriggerFired && n.part == mdSetPartTrim) {
            printf("    trigger fired at bar-in-piece %d, beat %d, tick %d\n",
                   n.bar, n.beat, n.tick);
            outOnBar = (n.beat == 1 && n.tick == 0);
            break;
        }
    }
    check(outOnBar, "lay-out trigger fires exactly on a 5/8 bar line");
    check(muse.SoundCall(mdGetParam, kMasterSound, midiPartTrim, partDiesIrae) == 0,
          "dies-irae part trim is now laid out");

    //-- Test 3: every bar marker lands on beat 1, tick 0 -------------------
    printf("Test 3: all bar markers are on the bar line\n");
    bool allMarkersOnBar = true;
    int markerCount = 0;
    for (const Notification& n : g_log) {
        if (n.type == Notification::Marker) {
            ++markerCount;
            if (!(n.beat == 1 && n.tick == 0))
                allMarkersOnBar = false;
        }
    }
    check(markerCount > 0 && allMarkersOnBar,
          "bar markers all report beat 1, tick 0");

    //-- Test 4: nested pause freezes musical time --------------------------
    printf("Test 4: pause / resume freezes and restores musical time\n");
    int b0, be0, ti0, b1, be1, ti1;
    muse.musicalPosition(kMasterSound, b0, be0, ti0);
    muse.SoundCall(imPause);
    muse.SoundCall(imPause);               // nested: two pauses
    advanceTo(t + 2.0);
    muse.musicalPosition(kMasterSound, b1, be1, ti1);
    check(b0 == b1 && be0 == be1 && ti0 == ti1,
          "position does not advance while paused");
    muse.SoundCall(imResume);              // still paused (counter == 1)
    advanceTo(t + 1.0);
    int b2, be2, ti2;
    muse.musicalPosition(kMasterSound, b2, be2, ti2);
    check(b2 == b1 && be2 == be1 && ti2 == ti1,
          "still paused after one of two resumes (nested counter)");
    muse.SoundCall(imResume);              // fully resumed
    advanceTo(t + 1.0);
    int b3, be3, ti3;
    muse.musicalPosition(kMasterSound, b3, be3, ti3);
    check(!(b3 == b2 && be3 == be2 && ti3 == ti2),
          "position advances again after full resume");

    //-- Test 5: fade drives a part trim smoothly over time -----------------
    printf("Test 5: imFade ramps a part trim\n");
    muse.SoundCall(imFade, kMasterSound, fadeMidiPartTrim + partLament,
                   kLayInTrim, 500 /*ms*/);
    advanceTo(t + 0.25);
    int mid = muse.SoundCall(mdGetParam, kMasterSound, midiPartTrim, partLament);
    advanceTo(t + 0.5);
    int end = muse.SoundCall(mdGetParam, kMasterSound, midiPartTrim, partLament);
    printf("    trim mid-fade=%d  end-fade=%d\n", mid, end);
    check(mid > 0 && mid < kLayInTrim, "trim is partway through mid-fade");
    check(end == kLayInTrim, "trim reaches target at end of fade");

    //-- Test 6: loop wrap re-arms markers ----------------------------------
    printf("Test 6: loop wraps and re-arms its bar markers\n");
    g_log.clear();
    advanceTo(t + 8 * barSeconds + 1.0);   // run past the 8-bar loop
    int wraps = 0, markersTotal = 0, markersAfterWrap = 0;
    bool wrapped = false;
    for (const Notification& n : g_log) {
        if (n.type == Notification::LoopWrap) { ++wraps; wrapped = true; }
        if (n.type == Notification::Marker) {
            ++markersTotal;
            if (wrapped) ++markersAfterWrap;
        }
    }
    printf("    loop wraps=%d  markers total=%d  markers after wrap=%d\n",
           wraps, markersTotal, markersAfterWrap);
    check(wraps >= 1, "the master loop wrapped at least once");
    check(markersAfterWrap >= 1,
          "bar markers fire again after the wrap (re-armed)");

    //-- Test 7: concurrent stinger sequencer -------------------------------
    printf("Test 7: independent one-shot stinger sequencer\n");
    muse.SoundCall(imStartSound, kStingerSound);
    check(muse.soundActive(kStingerSound), "stinger is active after start");
    check(muse.soundActive(kMasterSound),  "master still playing alongside stinger");
    g_log.clear();
    advanceTo(t + 8 * barSeconds);         // let the (non-looping) stinger finish
    bool stingerStopped = false;
    for (const Notification& n : g_log)
        if (n.type == Notification::SoundStopped && n.sound == kStingerSound)
            stingerStopped = true;
    check(stingerStopped && !muse.soundActive(kStingerSound),
          "non-looping stinger stops on its own; master keeps looping");
    check(muse.soundActive(kMasterSound), "master unaffected by stinger ending");

    printf("\n--- notification tail (last 12) ---\n");
    int from = int(g_log.size()) > 12 ? int(g_log.size()) - 12 : 0;
    for (int i = from; i < int(g_log.size()); ++i) {
        const Notification& n = g_log[i];
        printf("  %-13s sound=%d marker=%d part=%d val=%d @ bar %d:%d.%d\n",
               notifyName(n.type), n.sound, n.marker, n.part, n.value,
               n.bar, n.beat, n.tick);
    }

    printf("\n======================================\n");
    printf("%s (%d failure%s)\n",
           g_failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
           g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}

//============================================================================

int main(int argc, char** argv)
{
    bool selftest = false;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--selftest") == 0)
            selftest = true;

    if (selftest)
        return runSelfTest();

#if defined(_MSC_VER)
    printf("Interactive mode is not supported on this platform; "
           "use --selftest.\n");
    return 0;
#else
    return runInteractive();
#endif
}
