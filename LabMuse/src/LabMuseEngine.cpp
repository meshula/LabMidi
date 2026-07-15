//
//  LabMuseEngine.cpp
//
//  The LabMuse engine: the SoundCall() opcode dispatcher, a pause-aware musical
//  timebase, hierarchical group-volume busses, a 60 Hz fade table, the trigger
//  table and deferred-command queue, and the Sound struct (sequencer + params +
//  per-channel part map + active-note table + hooks).  One shared MidiSoftSynth
//  drives all 16 channels.
//
/*
┌──────────────────────────────────────────────────────────────────────────┐
│ ▓▓▓▒▒▒ PROVENANCE NODE [5] // ZONE: [HYBRID_REMIXABLE]                   │
├──────────────────────────────────────────────────────────────────────────┤
│ AUTH_ID  : [Nick Porcino]         | GEN_TOOL : [Claude Opus 4.8]         │
│ SRC_AUTH : [HMN]                  │ SIG_STAT : [UNSIGNED]                │
| INCEPT © : [Tues July 14 08:58:06 2026 -0800]                            |
├──────────────────────────────────────────────────────────────────────────┤
│ INDEX_ALLOW : [YES]               │ CORP_TRAIN: [NO]                     │
│ DERIV_ALLOW : [YES]               │ GOV_SPDX  : [SPDX: BSD 2-Clause]     │
└──────────────────────────────────────────────────────────────────────────┘
*/

#include "LabMuse/LabMuse.h"
#include "LabMuseInternal.h"

#include "LabMidi/SoftSynth.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace Lab {
namespace Muse {

//----------------------------------------------------------------------------
// A live sound: a sequencer plus the runtime part/parameter state.
//----------------------------------------------------------------------------

struct Sound {
    int       id    = 0;
    int       group = groupMusic;
    Sequencer seq;

    int volume    = 127;   // soundVol
    int pan       = 64;    // soundPan
    int transpose = 0;     // soundTranspose (semitones)
    int detune    = 0;     // soundDetune
    int priority  = 64;    // soundPriority

    std::array<int, 16>         partTrim{};      // 0..127
    std::array<int, 16>         partProgram{};
    std::array<int, 16>         partBankMSB;     // -1 == don't send
    std::array<int, 16>         partBankLSB;     // -1 == don't send
    std::array<std::string, 16> partName;
    std::array<bool, 16>        partDefined{};
    std::array<int, 16>         pendingTrim;     // -1 == no pending trigger
    std::set<int>               usedChannels;    // channels this sound emits on

    std::array<std::set<int>, 16> heldNotes;     // active notes per channel

    std::map<int, int> hooks;                    // hookId -> value

    bool active = false;

    Sound()
    {
        partTrim.fill(0);
        partProgram.fill(0);
        partBankMSB.fill(-1);
        partBankLSB.fill(-1);
        pendingTrim.fill(-1);
    }

    // Ordered list of channels for stable UI iteration.
    std::vector<int> orderedChannels() const
    {
        std::vector<int> v(usedChannels.begin(), usedChannels.end());
        return v;
    }
};

//----------------------------------------------------------------------------
// Trigger / deferred command records.
//----------------------------------------------------------------------------

struct Trigger {
    int sound  = 0;
    int marker = 0;
    int opcode = 0;
    int a = 0, b = 0, c = 0;   // args forwarded to the fired command
};

struct Deferred {
    double fireAt = 0.0;       // absolute wall-clock seconds
    int    opcode = 0;
    int    a = 0, b = 0, c = 0;
};

struct Fade {
    int    sound  = 0;
    int    param  = 0;         // FadeParam
    double start  = 0.0;
    double target = 0.0;
    double duration = 0.0;     // seconds
    double elapsed  = 0.0;
    bool   active   = false;
};

//----------------------------------------------------------------------------

class LabMuse::Detail {
public:
    explicit Detail(bool enableAudio)
    {
        groupVols.fill(127);
        recomputeGroupVols();

        if (enableAudio) {
            ownedSynth = new MidiSoftSynth();
            ownedSynth->initialize(0, nullptr);   // one instance, all 16 channels
            out = ownedSynth;
        }
    }

    ~Detail()
    {
        delete ownedSynth;
    }

    void setOutput(MidiOutBase* sink)
    {
        // Routing to an external sink retires the built-in synth so it doesn't
        // also grab the audio device.
        if (ownedSynth && sink != ownedSynth) {
            delete ownedSynth;
            ownedSynth = nullptr;
        }
        out = sink;
    }

    //-- output ------------------------------------------------------------
    void send(uint8_t status, uint8_t d1, uint8_t d2)
    {
        if (!out)
            return;
        MidiCommand c(status, d1, d2);
        out->command(&c);
    }

    //-- group volumes (essay's GrSetGroupVol) -----------------------------
    void recomputeGroupVols()
    {
        groupEffVols[groupMaster] = groupVols[groupMaster];
        for (int g = 1; g < GROUP_COUNT; ++g)
            groupEffVols[g] = ((groupVols[g] + 1) * groupVols[groupMaster]) >> 7;
    }

    int setGroupVol(int group, int vol)
    {
        if (group < 0 || group >= GROUP_COUNT)
            return imArgErr;
        if (vol == -1)
            return groupVols[group];       // query
        if (vol < 0 || vol > 127)
            return imArgErr;

        int old = groupVols[group];
        groupVols[group] = vol;
        recomputeGroupVols();
        return old;
    }

    //-- sound lookup ------------------------------------------------------
    Sound* find(int id)
    {
        auto it = sounds.find(id);
        return it == sounds.end() ? nullptr : &it->second;
    }

    //-- note routing ------------------------------------------------------
    int effectiveVelocity(const Sound& s, int channel, int vel) const
    {
        double g  = groupEffVols[s.group] / 127.0;
        double sv = s.volume / 127.0;
        double pt = s.partTrim[channel] / 127.0;
        double v  = double(vel) * g * sv * pt;
        int iv = int(std::lround(v));
        if (pt > 0.0 && vel > 0 && iv < 1)
            iv = 1;
        if (iv > 127) iv = 127;
        if (iv < 0)   iv = 0;
        return iv;
    }

    void routeEvent(Sound& s, const MuseEvent& e)
    {
        switch (e.cls) {
        case EvClass::Config:
            send(e.status, e.d1, e.d2);
            break;

        case EvClass::Marker:
            // Report the marker's own tick: during emission the sequencer's
            // posTicks has not yet advanced to the boundary.
            notifyAt(Notification::Marker, s, e.markerId, 0, 0, e.tick);
            fireTriggersForMarker(s, e.markerId, e.tick);
            break;

        case EvClass::Note: {
            int ch = e.channel;
            bool isOff = (e.status & 0xF0) == 0x80 || e.d2 == 0;
            if (isOff) {
                if (s.heldNotes[ch].erase(e.d1)) {
                    int n = e.d1 + s.transpose;
                    if (n < 0) n = 0;
                    if (n > 127) n = 127;
                    send(uint8_t(0x80 | ch), uint8_t(n), 0);
                }
            }
            else {
                int note = e.d1 + s.transpose;
                if (note < 0)   note = 0;
                if (note > 127) note = 127;
                int v = effectiveVelocity(s, ch, e.d2);
                if (v > 0) {
                    send(uint8_t(0x90 | ch), uint8_t(note), uint8_t(v));
                    s.heldNotes[ch].insert(e.d1);   // key by scored note id
                }
            }
            break;
        }
        }
    }

    // Send note-offs for every held note on a channel (used on trim->0, stop,
    // jump-without-sustain, loop-wrap).
    void flushChannel(Sound& s, int ch)
    {
        for (int note : s.heldNotes[ch]) {
            int n = note + s.transpose;
            if (n < 0) n = 0; if (n > 127) n = 127;
            send(uint8_t(0x80 | ch), uint8_t(n), 0);
        }
        s.heldNotes[ch].clear();
    }

    void flushAllNotes(Sound& s)
    {
        for (int ch = 0; ch < 16; ++ch)
            if (!s.heldNotes[ch].empty())
                flushChannel(s, ch);
    }

    // On any stop: all-sound-off (CC120) and all-notes-off (CC123) on the
    // channels this sound uses, so the DLS synth never strands a note.
    void allNotesOff(Sound& s)
    {
        flushAllNotes(s);
        for (int ch : s.usedChannels) {
            send(uint8_t(0xB0 | ch), 120, 0);   // all sound off
            send(uint8_t(0xB0 | ch), 123, 0);   // all notes off
        }
    }

    //-- triggers ----------------------------------------------------------
    void fireTriggersForMarker(Sound& s, int markerId, int atTick)
    {
        // Collect matches first; firing may itself queue/clear triggers.
        std::vector<Trigger> matched;
        for (auto it = triggers.begin(); it != triggers.end(); ) {
            if (it->sound == s.id && it->marker == markerId) {
                matched.push_back(*it);
                it = triggers.erase(it);      // one-shot
            }
            else
                ++it;
        }
        for (const Trigger& t : matched) {
            // Clear the UI "pending" flag for a part-trim trigger.
            if (t.opcode == mdSetPartTrim && t.a >= 0 && t.a < 16)
                s.pendingTrim[t.a] = -1;

            notifyAt(Notification::TriggerFired, s, markerId, t.opcode, t.a, atTick);
            self->SoundCall(t.opcode, s.id, t.a, t.b, t.c);
        }
    }

    //-- fades (stepped at 60 Hz) ------------------------------------------
    void applyFadeValue(Sound& s, int param, double value)
    {
        int iv = int(std::lround(value));
        if (param == fadeVol) {
            s.volume = std::clamp(iv, 0, 127);
        }
        else if (param == fadeMidiSpeed) {
            s.seq.speed = std::max(1, iv);
        }
        else if (param == fadePan) {
            s.pan = std::clamp(iv, 0, 127);
        }
        else if (param >= fadeMidiPartTrim && param <= fadeMidiPartTrim16) {
            int ch = param - fadeMidiPartTrim;
            s.partTrim[ch] = std::clamp(iv, 0, 127);
            if (s.partTrim[ch] == 0)
                flushChannel(s, ch);
        }
    }

    double currentFadeValue(const Sound& s, int param) const
    {
        if (param == fadeVol)        return s.volume;
        if (param == fadeMidiSpeed)  return s.seq.speed;
        if (param == fadePan)        return s.pan;
        if (param >= fadeMidiPartTrim && param <= fadeMidiPartTrim16)
            return s.partTrim[param - fadeMidiPartTrim];
        return 0.0;
    }

    void stepFades()
    {
        const double dt = 1.0 / 60.0;
        for (auto it = fades.begin(); it != fades.end(); ) {
            Sound* s = find(it->sound);
            if (!s) { it = fades.erase(it); continue; }

            it->elapsed += dt;
            double u = it->duration > 0.0 ? it->elapsed / it->duration : 1.0;
            if (u >= 1.0) u = 1.0;
            double v = it->start + (it->target - it->start) * u;
            applyFadeValue(*s, it->param, v);

            if (u >= 1.0)
                it = fades.erase(it);
            else
                ++it;
        }
    }

    void startFade(Sound& s, int param, double target, double durationSec)
    {
        // Replace any existing fade of the same param on this sound.
        fades.erase(std::remove_if(fades.begin(), fades.end(),
            [&](const Fade& f) { return f.sound == s.id && f.param == param; }),
            fades.end());

        if (durationSec <= 0.0) {
            applyFadeValue(s, param, target);
            return;
        }
        Fade f;
        f.sound    = s.id;
        f.param    = param;
        f.start    = currentFadeValue(s, param);
        f.target   = target;
        f.duration = durationSec;
        f.elapsed  = 0.0;
        f.active   = true;
        fades.push_back(f);
    }

    //-- notifications -----------------------------------------------------
    // atTick >= 0 reports the musical position of that explicit tick (used for
    // markers/triggers, which fire before posTicks advances to the boundary);
    // otherwise the sound's current position is used.
    void notifyAt(Notification::Type type, const Sound& s,
                  int marker, int part, int value, int atTick = -1)
    {
        if (!notifyFn)
            return;
        Notification n;
        n.type   = type;
        n.sound  = s.id;
        n.marker = marker;
        n.part   = part;
        n.value  = value;
        if (atTick >= 0) {
            int bt = s.seq.barTicks   > 0 ? s.seq.barTicks   : 1;
            int et = s.seq.beatTicks  > 0 ? s.seq.beatTicks  : 1;
            int within = atTick % bt;
            n.bar  = atTick / bt + 1;
            n.beat = within / et + 1;
            n.tick = within % et;
        }
        else {
            s.seq.musicalPosition(n.bar, n.beat, n.tick);
        }
        notifyFn(notifyUser, n);
    }

    //-- lifecycle ---------------------------------------------------------
    void startSound(Sound& s)
    {
        s.seq.active = true;
        s.active     = true;
        s.seq.reset();
        for (auto& h : s.heldNotes)
            h.clear();
        // Configure each part's instrument on the sink (bank select, then
        // program change).  Done here rather than as scored events so a device
        // profile can remap voices right up until the sound starts.
        for (int ch : s.usedChannels) {
            if (!s.partDefined[ch])
                continue;
            if (s.partBankMSB[ch] >= 0)
                send(uint8_t(0xB0 | ch), 0,  uint8_t(s.partBankMSB[ch]));
            if (s.partBankLSB[ch] >= 0)
                send(uint8_t(0xB0 | ch), 32, uint8_t(s.partBankLSB[ch]));
            send(uint8_t(0xC0 | ch), uint8_t(s.partProgram[ch]), 0);
        }
        notifyAt(Notification::SoundStarted, s, 0, 0, 0);
    }

    void stopSound(Sound& s)
    {
        allNotesOff(s);
        s.seq.active = false;
        s.active     = false;
        // Drop this sound's triggers and fades.
        triggers.erase(std::remove_if(triggers.begin(), triggers.end(),
            [&](const Trigger& t) { return t.sound == s.id; }), triggers.end());
        fades.erase(std::remove_if(fades.begin(), fades.end(),
            [&](const Fade& f) { return f.sound == s.id; }), fades.end());
        for (auto& p : s.pendingTrim) p = -1;
        notifyAt(Notification::SoundStopped, s, 0, 0, 0);
    }

    //-- the main update ---------------------------------------------------
    void update(double now)
    {
        if (!started) {
            lastNow = now;
            started = true;
            return;
        }
        double dt = now - lastNow;
        lastNow = now;
        if (dt < 0.0) dt = 0.0;

        if (pauseCount > 0)
            return;   // frozen: no musical time, no fades, no deferred commands

        // Fixed 60 Hz fade / deferred sub-counter, decoupled from note density.
        fadeAccum += dt;
        while (fadeAccum >= 1.0 / 60.0) {
            stepFades();
            fadeAccum -= 1.0 / 60.0;
        }

        // Deferred commands (wall-clock).
        for (auto it = deferred.begin(); it != deferred.end(); ) {
            if (now >= it->fireAt) {
                Deferred d = *it;
                it = deferred.erase(it);
                self->SoundCall(d.opcode, d.a, d.b, d.c);
            }
            else
                ++it;
        }

        // Advance every active sequencer.  Snapshot ids first: firing a trigger
        // during advance may start/stop sounds and mutate the map.
        std::vector<int> ids;
        ids.reserve(sounds.size());
        for (auto& kv : sounds)
            if (kv.second.active)
                ids.push_back(kv.first);

        for (int id : ids) {
            Sound* s = find(id);
            if (!s || !s->active)
                continue;
            bool wasActive = s->seq.active;
            s->seq.advance(dt);
            // Re-find: advance -> onEvent -> triggers -> SoundCall could rehash.
            s = find(id);
            if (!s)
                continue;
            if (wasActive && !s->seq.active) {
                // Non-looping sound reached its end.
                allNotesOff(*s);
                s->active = false;
                notifyAt(Notification::SoundStopped, *s, 0, 0, 0);
            }
        }
    }

    //-- state -------------------------------------------------------------
    LabMuse*  self  = nullptr;
    MidiOutBase*   out        = nullptr;   // active output sink
    MidiSoftSynth* ownedSynth = nullptr;   // built-in synth, when we made one

    std::map<int, Sound> sounds;
    std::vector<Trigger> triggers;
    std::vector<Deferred> deferred;
    std::vector<Fade>    fades;

    std::array<int, GROUP_COUNT> groupVols{};
    std::array<int, GROUP_COUNT> groupEffVols{};

    int    pauseCount = 0;
    bool   started    = false;
    double lastNow    = 0.0;
    double fadeAccum  = 0.0;

    NotifyFn notifyFn   = nullptr;
    void*    notifyUser = nullptr;
};

//============================================================================
// Public wrapper
//============================================================================

LabMuse::LabMuse(bool enableAudio)
: _detail(new Detail(enableAudio))
{
    _detail->self = this;
}

LabMuse::~LabMuse()
{
    delete _detail;
}

void LabMuse::setOutput(MidiOutBase* sink)
{
    _detail->setOutput(sink);
}

void LabMuse::setNotify(NotifyFn fn, void* user)
{
    _detail->notifyFn   = fn;
    _detail->notifyUser = user;
}

void LabMuse::update(double now)
{
    _detail->update(now);
}

//----------------------------------------------------------------------------
// Musical-position helper used by mdJump / mdScan: measure/beat/tick -> tick.
//----------------------------------------------------------------------------

static int positionToTick(const Sound& s, int measure, int beat, int tick)
{
    if (measure < 1) measure = 1;
    if (beat < 1)    beat = 1;
    return (measure - 1) * s.seq.barTicks
         + (beat - 1) * s.seq.beatTicks
         + tick;
}

//----------------------------------------------------------------------------
// The dispatcher.
//----------------------------------------------------------------------------

int LabMuse::SoundCall(int opcode, int a, int b, int c, int d, int e, int f)
{
    Detail& D = *_detail;

    switch (opcode) {

    //-- System ------------------------------------------------------------
    case imInitialize:
        return imSuccess;

    case imTerminate:
        for (auto& kv : D.sounds)
            if (kv.second.active)
                D.stopSound(kv.second);
        return imSuccess;

    case imPause:
        ++D.pauseCount;
        return imSuccess;

    case imResume:
        if (D.pauseCount > 0) --D.pauseCount;
        return imSuccess;

    case imSave:
    case imRestore:
        return imNotImplemented;

    case imPrepareSound:
        return D.find(a) ? imSuccess : imNoSound;   // data already resident

    case imSetGroupVol:
        return D.setGroupVol(a, b);

    case imStartSound: {
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        D.startSound(*s);
        return imSuccess;
    }

    case imStopSound: {
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        D.stopSound(*s);
        return imSuccess;
    }

    case imStopAllSounds:
        for (auto& kv : D.sounds)
            if (kv.second.active)
                D.stopSound(kv.second);
        return imSuccess;

    case imGetNextSound: {
        // Smallest active sound id strictly greater than 'a'; 0 if none.
        int best = 0;
        for (auto& kv : D.sounds) {
            if (kv.second.active && kv.first > a)
                if (best == 0 || kv.first < best)
                    best = kv.first;
        }
        return best;
    }

    case imSetParam: {
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        switch (b) {
        case soundVol:       s->volume    = std::clamp(c, 0, 127); break;
        case soundPan:       s->pan       = std::clamp(c, 0, 127); break;
        case soundTranspose: s->transpose = c; break;
        case soundDetune:    s->detune    = c; break;
        case soundPriority:  s->priority  = c; break;
        default: return imArgErr;
        }
        return imSuccess;
    }

    case imGetParam: {
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        switch (b) {
        case soundVol:       return s->volume;
        case soundPan:       return s->pan;
        case soundTranspose: return s->transpose;
        case soundDetune:    return s->detune;
        case soundPriority:  return s->priority;
        default: return imArgErr;
        }
    }

    case imFade: {
        // a=sound b=param c=target d=timeMilliseconds
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        D.startFade(*s, b, double(c), double(d) / 1000.0);
        return imSuccess;
    }

    case imSetHook: {
        // a=sound b=hookId c=value
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        s->hooks[b] = c;
        return imSuccess;
    }

    case imGetHook: {
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        auto it = s->hooks.find(b);
        return it == s->hooks.end() ? 0 : it->second;
    }

    case imSetTrigger: {
        // a=sound b=marker c=opcode d,e,f=args
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        Trigger t;
        t.sound = a; t.marker = b; t.opcode = c;
        t.a = d; t.b = e; t.c = f;
        D.triggers.push_back(t);
        // Reflect a pending part-trim change in the UI immediately.
        if (c == mdSetPartTrim && d >= 0 && d < 16)
            s->pendingTrim[d] = e;
        return imSuccess;
    }

    case imCheckTrigger: {
        // a=sound b=marker c=opcode ; count matching (either can be -1 wildcard)
        int count = 0;
        for (const Trigger& t : D.triggers) {
            if ((a < 0 || t.sound == a) &&
                (b < 0 || t.marker == b) &&
                (c < 0 || t.opcode == c))
                ++count;
        }
        return count;
    }

    case imClearTrigger: {
        Sound* s = D.find(a);
        size_t before = D.triggers.size();
        D.triggers.erase(std::remove_if(D.triggers.begin(), D.triggers.end(),
            [&](const Trigger& t) {
                bool match = (a < 0 || t.sound == a) &&
                             (b < 0 || t.marker == b) &&
                             (c < 0 || t.opcode == c);
                if (match && s && t.opcode == mdSetPartTrim &&
                    t.a >= 0 && t.a < 16)
                    s->pendingTrim[t.a] = -1;
                return match;
            }), D.triggers.end());
        return int(before - D.triggers.size());
    }

    case imDeferCommand: {
        // a=timeMilliseconds b=opcode c,d,e=args
        Deferred df;
        df.fireAt = D.lastNow + double(a) / 1000.0;
        df.opcode = b;
        df.a = c; df.b = d; df.c = e;
        D.deferred.push_back(df);
        return imSuccess;
    }

    //-- MIDI --------------------------------------------------------------
    case mdGetParam: {
        // a=sound b=param c=chan
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        int bar, beat, tick;
        s->seq.musicalPosition(bar, beat, tick);
        switch (b) {
        case midiChunk:    return 0;
        case midiMeasure:  return bar;
        case midiBeat:     return beat;
        case midiTick:     return tick;
        case midiSpeed:    return s->seq.speed;
        case midiTempo:    return int(std::lround(s->seq.tempoBPM));
        case midiPartTrim: return (c >= 0 && c < 16) ? s->partTrim[c] : imArgErr;
        default: return imArgErr;
        }
    }

    case mdJump: {
        // a=sound b=chunk c=measure d=beat e=tick f=sustain
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        if (!f)                       // no sustain: kill playing notes
            D.flushAllNotes(*s);
        int target = positionToTick(*s, c, d, e);
        s->seq.seekTo(target, /*emitConfig*/ false);
        return imSuccess;
    }

    case mdScan: {
        // a=sound b=chunk c=measure d=beat e=tick
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        int target = positionToTick(*s, c, d, e);
        s->seq.seekTo(target, /*emitConfig*/ true);
        return imSuccess;
    }

    case mdSetSpeed: {
        // a=sound b=speed
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        s->seq.speed = std::max(1, b);
        return imSuccess;
    }

    case mdSendMessage: {
        // a=sound b=status c=data1 d=data2  (raw MIDI passthrough)
        D.send(uint8_t(b), uint8_t(c), uint8_t(d));
        return imSuccess;
    }

    case mdSetPartTrim: {
        // a=sound b=chan c=trim
        Sound* s = D.find(a);
        if (!s) return imNoSound;
        if (b < 0 || b >= 16) return imArgErr;
        int trim = std::clamp(c, 0, 127);
        s->partTrim[b] = trim;
        s->pendingTrim[b] = -1;
        if (trim == 0)
            D.flushChannel(*s, b);       // stop ringing notes when laid out
        D.notifyAt(Notification::PartTrim, *s, 0, b, trim);
        return imSuccess;
    }

    case mdShareParts:
        return imNotImplemented;

    //-- WAVE (stubs) ------------------------------------------------------
    case wvGetParam:
    case wvStartDiskSound:
        return imNotImplemented;

    default:
        return imArgErr;
    }
}

//============================================================================
// Composition-database builder API
//============================================================================

int LabMuse::createSound(int soundId, int group,
                         int ticksPerQuarter, double tempoBPM,
                         int meter, int beatTicks)
{
    Detail& D = *_detail;
    if (D.find(soundId))
        return imArgErr;

    Sound s;
    s.id    = soundId;
    s.group = group;
    s.seq.ticksPerQuarter = ticksPerQuarter;
    s.seq.tempoBPM        = tempoBPM;
    s.seq.meter           = meter;
    s.seq.beatTicks       = beatTicks;
    s.seq.barTicks        = meter * beatTicks;
    s.seq.speed           = 128;

    // Wire the sequencer callbacks back into the engine.
    int id = soundId;
    LabMuse::Detail* Dp = &D;
    D.sounds[soundId] = std::move(s);

    Sound& stored = D.sounds[soundId];
    stored.seq.onEvent = [Dp, id](const MuseEvent& e) {
        Sound* snd = Dp->find(id);
        if (snd)
            Dp->routeEvent(*snd, e);
    };
    stored.seq.onLoopWrap = [Dp, id]() {
        Sound* snd = Dp->find(id);
        if (!snd)
            return;
        Dp->flushAllNotes(*snd);
        Dp->notifyAt(Notification::LoopWrap, *snd, 0, 0, 0);
    };
    return imSuccess;
}

void LabMuse::setLoop(int soundId, int startTick, int endTick)
{
    Sound* s = _detail->find(soundId);
    if (!s) return;
    s->seq.looping   = true;
    s->seq.loopStart = startTick;
    s->seq.loopEnd   = endTick;
}

void LabMuse::definePart(int soundId, int channel, const char* name,
                         int gmProgram, int initialTrim)
{
    Sound* s = _detail->find(soundId);
    if (!s || channel < 0 || channel >= 16) return;
    s->partDefined[channel] = true;
    s->partName[channel]    = name ? name : "";
    s->partProgram[channel] = gmProgram;
    s->partBankMSB[channel] = -1;
    s->partBankLSB[channel] = -1;
    s->partTrim[channel]    = std::clamp(initialTrim, 0, 127);
    s->usedChannels.insert(channel);
    // The bank-select + program-change are emitted when the sound starts
    // (see startSound), so a device profile can remap the voice until then.
}

void LabMuse::setPartInstrument(int soundId, int channel, int program,
                                int bankMSB, int bankLSB)
{
    Sound* s = _detail->find(soundId);
    if (!s || channel < 0 || channel >= 16) return;
    s->partProgram[channel] = program;
    s->partBankMSB[channel] = bankMSB;
    s->partBankLSB[channel] = bankLSB;
    if (!s->partDefined[channel]) {
        s->partDefined[channel] = true;
        s->usedChannels.insert(channel);
    }
}

void LabMuse::addNote(int soundId, int channel, int startTick, int durTicks,
                      int note, int velocity)
{
    Sound* s = _detail->find(soundId);
    if (!s || channel < 0 || channel >= 16) return;
    s->usedChannels.insert(channel);

    MuseEvent on;
    on.tick = startTick; on.cls = EvClass::Note;
    on.status = uint8_t(0x90 | channel);
    on.d1 = uint8_t(note); on.d2 = uint8_t(velocity);
    on.channel = channel;
    s->seq.addEvent(on);

    MuseEvent off;
    off.tick = startTick + durTicks; off.cls = EvClass::Note;
    off.status = uint8_t(0x80 | channel);
    off.d1 = uint8_t(note); off.d2 = 0;
    off.channel = channel;
    s->seq.addEvent(off);

    s->seq.sortEvents();
}

void LabMuse::addControlChange(int soundId, int channel, int tick, int cc, int value)
{
    Sound* s = _detail->find(soundId);
    if (!s || channel < 0 || channel >= 16) return;
    s->usedChannels.insert(channel);
    MuseEvent ev;
    ev.tick = tick; ev.cls = EvClass::Config;
    ev.status = uint8_t(0xB0 | channel);
    ev.d1 = uint8_t(cc); ev.d2 = uint8_t(value);
    ev.channel = channel;
    s->seq.addEvent(ev);
    s->seq.sortEvents();
}

void LabMuse::addProgramChange(int soundId, int channel, int tick, int program)
{
    Sound* s = _detail->find(soundId);
    if (!s || channel < 0 || channel >= 16) return;
    s->usedChannels.insert(channel);
    MuseEvent ev;
    ev.tick = tick; ev.cls = EvClass::Config;
    ev.status = uint8_t(0xC0 | channel);
    ev.d1 = uint8_t(program); ev.d2 = 0;
    ev.channel = channel;
    s->seq.addEvent(ev);
    s->seq.sortEvents();
}

void LabMuse::addMarker(int soundId, int tick, int markerId)
{
    Sound* s = _detail->find(soundId);
    if (!s) return;
    MuseEvent ev;
    ev.tick = tick; ev.cls = EvClass::Marker;
    ev.markerId = markerId;
    s->seq.addEvent(ev);
    s->seq.sortEvents();
}

//============================================================================
// Introspection
//============================================================================

bool LabMuse::soundActive(int soundId) const
{
    Sound* s = _detail->find(soundId);
    return s && s->active;
}

bool LabMuse::musicalPosition(int soundId, int& bar, int& beat, int& tick) const
{
    Sound* s = _detail->find(soundId);
    if (!s) return false;
    s->seq.musicalPosition(bar, beat, tick);
    return true;
}

double LabMuse::tempo(int soundId) const
{
    Sound* s = _detail->find(soundId);
    return s ? s->seq.tempoBPM : 0.0;
}

int LabMuse::speed(int soundId) const
{
    Sound* s = _detail->find(soundId);
    return s ? s->seq.speed : 0;
}

int LabMuse::partCount(int soundId) const
{
    Sound* s = _detail->find(soundId);
    return s ? int(s->usedChannels.size()) : 0;
}

bool LabMuse::partInfo(int soundId, int index, int& channel, const char*& name,
                       int& trim, bool& pending, int& pendingTrim) const
{
    Sound* s = _detail->find(soundId);
    if (!s) return false;
    std::vector<int> chans = s->orderedChannels();
    if (index < 0 || index >= int(chans.size()))
        return false;
    int ch = chans[index];
    channel = ch;
    name    = s->partName[ch].c_str();
    trim    = s->partTrim[ch];
    pending = s->pendingTrim[ch] >= 0;
    pendingTrim = s->pendingTrim[ch];
    return true;
}

} // namespace Muse
} // namespace Lab
