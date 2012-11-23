//
//  LabMidiSongPlayer.cpp
//

/*
 Copyright (c) 2012, Nick Porcino
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 * The names of its contributors may not be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabMidiSongPlayer.h"

#include "LabMidiCommand.h"
#include "LabMidiOut.h"
#include "LabMidiSong.h"

#include <stdint.h>

namespace Lab {

    struct MidiRtEvent
    {
        MidiRtEvent(float t, uint8_t b1, uint8_t b2, uint8_t b3)
        : time(t)
        {
            command.command = b1;
            command.byte1 = b2;
            command.byte2 = b3;
        }
        
        MidiRtEvent& operator=(const MidiRtEvent& rhs)
        {
            time = rhs.time;
            command = rhs.command;
            return *this;
        }
        
        float time;
        MidiCommand command;
    };
    
    class MidiSongPlayer::Detail
    {
    public:
        
        Detail(MidiSong* s, MidiOutBase* mo)
        : song(s)
        , startTime(0)
        , midiOut(mo)
        , eventCursor(0)
        {
            beatsPerMinute = s ? s->startingTempo : 120.0f; // 120 is the standard default value
            ticksPerBeat = s ? s->ticksPerBeat : 100.0f;    // 100 is an arbitrary safe value
            events.reserve(10000);  // arbritrarily large to avoid push_back delays
        }
        
        void update(float wallclockTime)
        {
            if (eventCursor >= events.size())
                return;
            
            float newTime = wallclockTime - startTime;
            while (eventCursor < events.size() && events[eventCursor].time <= newTime) {
                if (midiOut) {
                    MidiRtEvent& ev = events[eventCursor];
                    midiOut->command(&ev.command);
                }
                ++eventCursor;
            }
        }
        
        double ticksToSeconds(int ticks)
        {
            double beats = double(ticks) / ticksPerBeat;
            double seconds = beats / (beatsPerMinute / 60);
            return seconds;
        }
        
        void recordEvent(double now, MidiEvent* ev)
        {
            if (ev->eventType == MIDI_EventSetTempo) {
                SetTempoEvent* ste = (SetTempoEvent*) ev;
                beatsPerMinute = 60000000.0f / float(ste->microsecondsPerBeat);
            }
            else if (ev->eventType == MIDI_EventChannel) {
                if (midiOut) {
                    ChannelEvent* ce = (ChannelEvent*) ev;
                    events.push_back(MidiRtEvent(float(now), ce->midiCommand, ce->param1, ce->param2));
                }
            }
        }

        std::vector<MidiRtEvent> events;
        
        MidiSong* song;
        
        float startTime;
        float beatsPerMinute;
        double ticksPerBeat;
        int eventCursor;
        
        MidiOutBase* midiOut;
    };
    
    MidiSongPlayer::MidiSongPlayer(MidiSong* s, MidiOutBase* midiOut)
    : _detail(new Detail(s, midiOut))
    {
        if (s) {
            std::vector<MidiTrack*>& tracks = *(s->tracks);
            int tc = tracks.size();

            // double, because don't want to introduce sync slip during rendering
            double* nextTime = (double*) alloca(sizeof(double) * tc);
            int* nextIndex = (int*) alloca(sizeof(int) * tc);
            
            int i = 0;
            for (auto t = s->tracks->begin(); t != s->tracks->end(); ++t, ++i) {
                MidiTrack& track = *(*t);
                int ec = track.events.size();
                nextTime[i] = ec ? track.events[0]->deltatime : std::numeric_limits<double>::max();
                nextIndex[i] = ec ? 0 : -1;
            }
            
            do {
                double nextEventT = std::numeric_limits<double>::max();
                int nt = -1;
                for (int i = 0; i < tc; ++i) {
                    if (nextIndex[i] >= tracks[i]->events.size())
                        continue;
                    if (nextTime[i] < nextEventT) {
                        nt = i;
                        nextEventT = nextTime[i];
                    }
                }
                if (nt == -1)
                    break;

                MidiEvent* ev = tracks[nt]->events[nextIndex[nt]];
                _detail->recordEvent(nextTime[nt], ev);
                ++nextIndex[nt];
                int n = nextIndex[nt];
                if (n < tracks[nt]->events.size())
                    nextTime[nt] += _detail->ticksToSeconds(tracks[nt]->events[n]->deltatime);
            } while (true);
        }
    }
    
    MidiSongPlayer::~MidiSongPlayer()
    {
        delete _detail;
    }
    
    void MidiSongPlayer::play(float wallclockTime)
    {
        _detail->startTime = wallclockTime;
    }
    
    void MidiSongPlayer::update(float wallclockTime)
    {
        _detail->update(wallclockTime);
    }
    
    float MidiSongPlayer::length() const
    {
        return _detail->events.back().time;
    }

} // Lab
