//
//  LabMidiSong.h
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

#pragma once

#include <string>
#include <vector>

namespace Lab {

    class MidiOutBase;
    
    enum MIDI_EventType {
        MIDI_EventSequenceNumber, MIDI_EventText, MIDI_EventCopyrightNotice,
        MIDI_EventTrackName, MIDI_EventInstrumentName, MIDI_EventLyrics,
        MIDI_EventMarker, MIDI_EventCuePoint, MIDI_EventMidiChannelPrefix,
        MIDI_EventEndOfTrack, MIDI_EventSetTempo, MIDI_EventSmpteOffset,
        MIDI_EventTimeSignature, MIDI_EventKeySignature, MIDI_EventSequencerSpecific,
        MIDI_EventUnknown, MIDI_EventSysEx, MIDI_EventDividedSysEx, MIDI_EventChannel
    };
    
    struct MidiEvent {
        MidiEvent(MIDI_EventType s) : eventType(s), deltatime(0) { }
        virtual ~MidiEvent() { }
        MIDI_EventType eventType;
        int deltatime;
    };
    
    #define DECLARE_EVENT(ev) struct ev ## Event : public MidiEvent { ev ## Event() : MidiEvent(MIDI_Event ## ev) { }
    DECLARE_EVENT(SequenceNumber) int number; };
    DECLARE_EVENT(Text) std::string text; };
    DECLARE_EVENT(CopyrightNotice) std::string text; };
    DECLARE_EVENT(TrackName) std::string text; };
    DECLARE_EVENT(InstrumentName) std::string text; };
    DECLARE_EVENT(Lyrics) std::string text; };
    DECLARE_EVENT(Marker) std::string text; };
    DECLARE_EVENT(CuePoint) std::string text; };
    DECLARE_EVENT(MidiChannelPrefix) int channel; };
    DECLARE_EVENT(EndOfTrack) };
    DECLARE_EVENT(SetTempo) int microsecondsPerBeat; };
    DECLARE_EVENT(SmpteOffset) int framerate; int hour; int min; int sec; int frame; int subframe; };
    DECLARE_EVENT(TimeSignature) int numerator; int denominator; int metronome; int thirtyseconds; };
    DECLARE_EVENT(KeySignature) int key; int scale; };
    DECLARE_EVENT(SequencerSpecific) ~SequencerSpecificEvent() { delete[] data; } uint8_t* data; };
    DECLARE_EVENT(Unknown) ~UnknownEvent() { delete[] data; } uint8_t* data; };
    DECLARE_EVENT(SysEx) ~SysExEvent() { delete[] data; } uint8_t* data; };
    DECLARE_EVENT(DividedSysEx) ~DividedSysExEvent() { delete[] data; } uint8_t* data; };
    DECLARE_EVENT(Channel) uint8_t midiCommand; uint8_t param1; uint8_t param2; };

    class MidiTrack {
    public:
        std::vector<MidiEvent*> events;
    };

    class MidiSong {
    public:
        MidiSong();
        ~MidiSong();
        
        void parse(uint8_t const*const midifiledata, int length, bool verbose);
        void parse(char const*const midifilePath, bool verbose);
        
        void parseMML(char const*const mmlStr, int length, bool verbose);
        void parseMML(char const*const midifilePath, bool verbose);

        void clearTracks();
        
        float ticksPerBeat;   // precision (number of ticks distinguishable per second)
        float startingTempo;
        std::vector<MidiTrack*>* tracks;
    };

} // Lab

