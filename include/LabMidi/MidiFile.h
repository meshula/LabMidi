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

#include <algorithm>
#include <memory>
#include <ostream>
#include <vector>

namespace Lab {

    class MidiOutBase;

    //////////////////////////
    // (Meta) Message Type  //
    //////////////////////////

    enum class Midi_MetaEventType : uint8_t
    {
        SEQUENCE_NUMBER = 0x00,
        TEXT = 0x01,
        COPYRIGHT = 0x02,
        TRACK_NAME = 0x03,
        INSTRUMENT = 0x04,
        LYRIC = 0x05,
        MARKER = 0x06,
        CUE = 0x07,
        PATCH_NAME = 0x08,
        DEVICE_NAME = 0x09,
        MIDI_CHANNEL_PREFIX = 0x20,
        WHAT_is_THIS = 0x21,
        END_OF_TRACK = 0x2F,
        TEMPO_CHANGE = 0x51,
        SMPTE_OFFSET = 0x54,
        TIME_SIGNATURE = 0x58,
        KEY_SIGNATURE = 0x59,
        PROPRIETARY = 0x7F,
        SYSTEM_EXCLUSIVE = 0xF0,
        END_OF_EXCLUSIVE = 0xF7,
        LABMIDI_CHANNEL_EVENT = 0xFE,
        UNKNOWN = 0xFF
    };
    
    constexpr inline uint8_t clamp(uint8_t val, uint8_t min, uint8_t max)
    {
        return std::max(std::min(val, max), min);
    }

    struct MidiEvent {
        MidiEvent(Midi_MetaEventType s) : eventType(s), tick(0) { }
        ~MidiEvent() = default;

        Midi_MetaEventType eventType;
        int tick;
        std::vector<uint8_t> data;
    };
    
    struct Event_SequenceNumber : public MidiEvent {
        Event_SequenceNumber() : MidiEvent(Midi_MetaEventType::SEQUENCE_NUMBER) {} uint16_t number = 0; };
    struct Event_Text : public MidiEvent {
        Event_Text() : MidiEvent(Midi_MetaEventType::TEXT) {} };
    struct Event_CopyrightNotice : public MidiEvent {
        Event_CopyrightNotice() : MidiEvent(Midi_MetaEventType::COPYRIGHT) {} };
    struct Event_TrackName : public MidiEvent {
        Event_TrackName() : MidiEvent(Midi_MetaEventType::TRACK_NAME) {} };
    struct Event_InstrumentName : public MidiEvent {
        Event_InstrumentName() : MidiEvent(Midi_MetaEventType::INSTRUMENT) {} };
    struct Event_Lyrics : public MidiEvent {
        Event_Lyrics() : MidiEvent(Midi_MetaEventType::LYRIC) {} };
    struct Event_Marker : public MidiEvent {
        Event_Marker() : MidiEvent(Midi_MetaEventType::MARKER) {} };
    struct Event_Cue : public MidiEvent {
        Event_Cue() : MidiEvent(Midi_MetaEventType::CUE) {} };
    struct Event_MidiChannelPrefix : public MidiEvent {
        Event_MidiChannelPrefix() : MidiEvent(Midi_MetaEventType::MIDI_CHANNEL_PREFIX) {} uint8_t channel = 0; };
    struct Event_EndOfTrack : public MidiEvent {
        Event_EndOfTrack() : MidiEvent(Midi_MetaEventType::END_OF_TRACK) {} };
    struct Event_SetTempo : public MidiEvent {
        Event_SetTempo() : MidiEvent(Midi_MetaEventType::TEMPO_CHANGE) {} int microsecondsPerBeat = 125000; };
    struct Event_SmpteOffset : public MidiEvent {
        Event_SmpteOffset() : MidiEvent(Midi_MetaEventType::SMPTE_OFFSET) {} uint8_t framerate = 0; uint8_t hour = 0; uint8_t min = 0; uint8_t sec = 0; uint8_t frame = 0; uint8_t subframe = 0; };
    struct Event_TimeSignature : public MidiEvent {
        Event_TimeSignature() : MidiEvent(Midi_MetaEventType::TIME_SIGNATURE) {}  double timeSignature = 120.; uint8_t metronome = 0; uint8_t thirtyseconds = 0; };
    struct Event_KeySignature : public MidiEvent {
        Event_KeySignature() : MidiEvent(Midi_MetaEventType::KEY_SIGNATURE) {} uint8_t key = 0; uint8_t scale = 0; };
    struct Event_SequencerSpecific : public MidiEvent {
        Event_SequencerSpecific() : MidiEvent(Midi_MetaEventType::PROPRIETARY) {} };
    struct Event_Unknown : public MidiEvent {
        Event_Unknown() : MidiEvent(Midi_MetaEventType::UNKNOWN) {} };
    struct Event_SysEx : public MidiEvent {
        Event_SysEx() : MidiEvent(Midi_MetaEventType::SYSTEM_EXCLUSIVE) {} };
    struct Event_DividedSysEx : public MidiEvent {
        Event_DividedSysEx() : MidiEvent(Midi_MetaEventType::END_OF_EXCLUSIVE) {} };

    struct Event_Channel : public MidiEvent {
        Event_Channel() : MidiEvent(Midi_MetaEventType::LABMIDI_CHANNEL_EVENT) {} };

    class MidiTrack {
    public:
        ~MidiTrack()
        {
            for (auto e : events)
                delete e;
        }
        std::vector<MidiEvent*> events;
    };

    class MidiSong {
    public:
        MidiSong();
        ~MidiSong();
        
        void parse(uint8_t const*const midifiledata, size_t length, bool verbose);
        void parse(char const*const midifilePath, bool verbose);

        void writeMidi(std::ostream& out);

        // Converts MML to Midi
        void parseMML(char const*const mmlStr, size_t length, bool verbose);
        void parseMML(char const*const midifilePath, bool verbose);

        void clearTracks();
        
        float ticksPerBeat = 1;   // precision (number of ticks distinguishable per second)
        float startingTempo = 120;
        std::vector<std::shared_ptr<MidiTrack>> tracks;
    };

} // Lab

