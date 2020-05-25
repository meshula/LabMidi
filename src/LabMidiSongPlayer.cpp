;//
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

#include "LabMidi/MidiFile.h"
#include "LabMidi/MidiFilePlayer.h"
#include "LabMidi/MidiInOut.h"
#include "LabMidi/Util.h"

#include <limits>
#include <vector>
#include <cstdint>

namespace Lab {
    
    class MidiSongPlayer::Detail
    {
    public:
        
        Detail(MidiSong* s)
        : song(s)
        , startTime(0)
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

                MidiRtEvent& ev = events[eventCursor];

                for (auto i = callbacks.begin(); i != callbacks.end(); ++i)
                    (*i).second((*i).first, &ev);
                
                ++eventCursor;
            }
        }
        
        double ticksToSeconds(int ticks)
        {
            double beats = double(ticks) / ticksPerBeat;
            double seconds = beats / (beatsPerMinute / 60.f);
            return seconds;
        }
        
        double secondsToTicks(float seconds)
        {
            double ticksPerSecond = (beatsPerMinute * ticksPerBeat) / 60.f;
            double ticks = ticksPerSecond * seconds;
            return (int)ticks;
        }
        
        void recordEvent(double now, MidiEvent* ev)
        {
            if (ev->eventType == Midi_MetaEventType::TEMPO_CHANGE) {
                Event_SetTempo* ste = (Event_SetTempo*) ev;
                beatsPerMinute = 60000000.0f / float(ste->microsecondsPerBeat);
            }
            else if (ev->eventType == Midi_MetaEventType::LABMIDI_CHANNEL_EVENT && ev->data.size() >= 2) {
                events.push_back(MidiRtEvent(float(now), ev->data[0], ev->data[1], ev->data[2]));
            }
        }
        
        std::vector<MidiRtEvent> events;
        
        MidiSong* song;
        
        float startTime;
        float beatsPerMinute;
        double ticksPerBeat;
        int eventCursor;
        
        std::vector<std::pair<void*, MidiEventCallbackFn> > callbacks;
    };
    
    MidiSongPlayer::MidiSongPlayer(MidiSong* s)
    : _detail(new Detail(s))
    {
        if (s) {
            size_t tc = s->tracks.size();
            
            // double, because don't want to introduce sync slip during rendering
            std::vector<double> nextTime;
            nextTime.resize(tc);
            std::vector<int> nextIndex;
            nextIndex.resize(tc);
            
            int i = 0;
            for (auto t = s->tracks.begin(); t != s->tracks.end(); ++t, ++i) {
                size_t ec = (*t)->events.size();
                nextTime[i] = ec ? (*t)->events[0]->tick : std::numeric_limits<double>::max();
                nextIndex[i] = ec ? 0 : -1;
            }
            
            do {
                double nextEventT = std::numeric_limits<double>::max();
                int nt = -1;
                for (int i = 0; i < tc; ++i) {
                    if (nextIndex[i] >= s->tracks[i]->events.size())
                        continue;
                    if (nextTime[i] < nextEventT) {
                        nt = i;
                        nextEventT = nextTime[i];
                    }
                }
                if (nt == -1)
                    break;
                
                MidiEvent* ev = s->tracks[nt]->events[nextIndex[nt]];
                _detail->recordEvent(nextTime[nt], ev);
                ++nextIndex[nt];
                int n = nextIndex[nt];
                if (n < s->tracks[nt]->events.size())
                    nextTime[nt] += _detail->ticksToSeconds(s->tracks[nt]->events[n]->tick);
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
    
    void MidiSongPlayer::addCallback(MidiEventCallbackFn f, void* userData)
    {
        _detail->callbacks.push_back(std::pair<void*, MidiEventCallbackFn>(userData, f));
    }
    
    void MidiSongPlayer::removeCallback(void* userData)
    {
        std::vector<std::pair<void*, MidiEventCallbackFn> >::iterator i = _detail->callbacks.begin();
        while (i != _detail->callbacks.end()) {
            if (userData == (*i).first) {
                _detail->callbacks.erase(i);
                i = _detail->callbacks.begin();
            }
            else
                ++i;
        }
    }
    
} // Lab
