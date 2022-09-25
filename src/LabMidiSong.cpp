//
//  LabMidiSong.cpp
//

//  Copyright (c) 2012, Nick Porcino
//  All rights reserved.
//  SPDX-License-Identifier: BSD-2-Clause

// Includes midi file writer and other improvements from modern-midi
// which itself included portions of LabMidi.
// Those improvements:
// Copyright (c) 2015, Dimitri Diakopoulos All rights reserved.
// SPDX-License-Identifier: BSD-2-Clause

//
// The parser is loosely ported to C++ from jasmid/midifile.js obtained from github 12/11/17
// https://github.com/gasman/jasmid
//
// Thanks to the jasmid team for posting such clean and useful code on github.
//
// This is the only jasmid ported source in this library. Jasmid carried the
// following license when obtained from github:
/*
Copyright (c) 2010, Matt Westcott & Ben Firshman
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

// The Base64 decoder is based on
//  http://base64.sourceforge.net/b64.c
// It carries the copyright notice:
/*
 MODULE NAME:    b64.c
 
 AUTHOR:         Bob Trower 08/04/01
 
 PROJECT:        Crypt Data Packaging
 
 COPYRIGHT:      Copyright (c) Trantor Standard Systems Inc., 2001
 
 NOTE:           This source code may be used as you wish, subject to
 the MIT license.  See the LICENCE section below.
 
 LICENCE:        Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated
 documentation files (the "Software"), to deal in the
 Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall
 be included in all copies or substantial portions of the
 Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "LabMidi/MidiFile.h"

#include "LabMidi/MidiInOut.h"

#include <stdexcept>
#include <iostream>
#include <cmath>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <vector>



/*
    ** decodeblock
    **
    ** decode 4 '6-bit' characters into 3 8-bit binary bytes
    */
void decodeblock(unsigned char in[4], unsigned char out[3])
{
    out[0] = (unsigned char)(in[0] << 2 | in[1] >> 4);
    out[1] = (unsigned char)(in[1] << 4 | in[2] >> 2);
    out[2] = (unsigned char)(((in[2] << 6) & 0xc0) | in[3]);
}

/*
    ** Translation Table to decode (created by author)
    */
static const char cd64[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
    ** decode
    **
    ** decode a base64 encoded stream discarding padding, line breaks and noise
    */
void decode64(uint8_t const* infile, uint8_t* outfile, size_t filelen)
{
    unsigned char in[4], out[3], v;
    int i, len;

    while (filelen > 0) {
        for (len = 0, i = 0; i < 4 && (filelen > 0); i++) {
            v = 0;
            while ((filelen > 0) && v == 0) {
                v = (unsigned char)*infile++;
                --filelen;
                v = (unsigned char)((v < 43 || v > 122) ? 0 : cd64[v - 43]);
                if (v) {
                    v = (unsigned char)((v == '$') ? 0 : v - 61);
                }
            }
            if (filelen > 0) {
                len++;
                if (v) {
                    in[i] = (unsigned char)(v - 1);
                }
            }
            else {
                in[i] = 0;
            }
        }
        if (len) {
            decodeblock(in, out);
            for (i = 0; i < len - 1; i++) {
                *outfile = out[i];
                ++outfile;
            }
        }
    }
}

namespace mm
{
    
    /////////////////////////////
    // (Channel) Message Type  //
    /////////////////////////////
    
    enum class MessageType : uint8_t
    {
        INVALID             = 0x0,
        // Standard Message
        NOTE_OFF           = 0x80,
        NOTE_ON            = 0x90,
        POLY_PRESSURE      = 0xA0, // after touch
        CONTROL_CHANGE     = 0xB0,
        PROGRAM_CHANGE     = 0xC0,
        AFTERTOUCH         = 0xD0, // channel pressure
        PITCH_BEND         = 0xE0,
        
        // System Common Messages
        SYSTEM_EXCLUSIVE   = 0xF0,
        TIME_CODE          = 0xF1,
        SONG_POS_POINTER   = 0xF2,
        SONG_SELECT        = 0xF3,
        RESERVED1          = 0xF4,
        RESERVED2          = 0xF5,
        TUNE_REQUEST       = 0xF6,
        EOX                = 0xF7,
        
        // System Realtime Messages
        TIME_CLOCK         = 0xF8,
        RESERVED3          = 0xF9,
        START              = 0xFA,
        CONTINUE           = 0xFB,
        STOP               = 0xFC,
        RESERVED4          = 0xFD,
        ACTIVE_SENSING     = 0xFE,
        SYSTEM_RESET       = 0xFF
    };


    std::ostream & write_uint16_be(std::ostream & out, uint16_t value)
    {
        union { uint8_t bytes[2]; uint16_t v; } data;
        data.v = value;
        out << data.bytes[1]; out << data.bytes[0];
        return out;
    }

    std::ostream & write_int16_be(std::ostream & out, int16_t value)
    {
        union { uint8_t bytes[2]; int16_t v; } data;
        data.v = value;
        out << data.bytes[1]; out << data.bytes[0];
        return out;
    }

    std::ostream & write_uint32_be(std::ostream & out, uint32_t value)
    {
        union { uint8_t bytes[4]; uint32_t v; } data;
        data.v = value;
        out << data.bytes[3]; out << data.bytes[2];
        out << data.bytes[1]; out << data.bytes[0];
        return out;
    }

    std::ostream & write_int32_be(std::ostream & out, int32_t value)
    {
        union { uint8_t bytes[4]; int32_t v; } data;
        data.v = value;
        out << data.bytes[3]; out << data.bytes[2];
        out << data.bytes[1]; out << data.bytes[0];
        return out;
    }

    std::ostream & write_float_be(std::ostream & out, float value)
    {
        union { uint8_t bytes[4]; float v; } data;
        data.v = value;
        out << data.bytes[3]; out << data.bytes[2];
        out << data.bytes[1]; out << data.bytes[0];
        return out;
    }

    std::ostream & write_double_be(std::ostream & out, double value)
    {
        union { uint8_t bytes[8]; double v; } data;
        data.v = value;
        out << data.bytes[7]; out << data.bytes[6];
        out << data.bytes[5]; out << data.bytes[4];
        out << data.bytes[3]; out << data.bytes[2];
        out << data.bytes[1]; out << data.bytes[0];
        return out;
    }

    // Write a number to the midifile
    // as a variable length value which segments a file into 7-bit
    // values.  Maximum size of aValue is 0x7fffffff
    void write_variable_length(uint32_t aValue, std::vector<uint8_t> & outdata)
    {
        uint8_t bytes[5] = {0};
        
        bytes[0] = (uint8_t) (((uint32_t) aValue >> 28) & 0x7F);  // most significant 5 bits
        bytes[1] = (uint8_t) (((uint32_t) aValue >> 21) & 0x7F);  // next largest 7 bits
        bytes[2] = (uint8_t) (((uint32_t) aValue >> 14) & 0x7F);
        bytes[3] = (uint8_t) (((uint32_t) aValue >> 7)  & 0x7F);
        bytes[4] = (uint8_t) (((uint32_t) aValue)       & 0x7F);  // least significant 7 bits
        
        int start = 0;
        while (start < 5 && bytes[start] == 0)
            start++;
        
        for (int i = start; i < 4; i++)
        {
            bytes[i] = bytes[i] | 0x80;
            outdata.push_back(bytes[i]);
        }
        outdata.push_back(bytes[4]);
    }
}


namespace Lab {
    
// Read a MIDI-style variable-length integer (big-endian value in groups of 7 bits,
// with top bit set to signify that another byte follows). 
inline uint32_t read_variable_length(uint8_t const *& data)
{
    uint32_t result = 0;
    while (true) 
    {
        uint8_t b = *data++;
        if (b & 0x80) 
        {
            result += (b & 0x7F);
            result <<= 7;
        } 
        else 
        {
            return result + b; // b is the last byte
        }
    }
}

// in the names that follow, be = big endian

inline uint16_t read_uint16_be(uint8_t const *& data)
{
    uint16_t result = uint16_t(*data++) << 8;
    result += int(*data++);
    return result;
}
    
inline uint32_t read_uint24_be(uint8_t const *& data)
{
    uint32_t result = uint32_t(*data++) << 16;
    result += uint32_t(*data++) << 8;
    result += uint32_t(*data++);
    return result;
}
    
inline uint32_t read_uint32_be(uint8_t const *& data)
{
    uint32_t result = uint32_t(*data++) << 24;
    result += uint32_t(*data++) << 16;
    result += uint32_t(*data++) << 8;
    result += uint32_t(*data++);
    return result;
}


MidiEvent* parseEvent(uint8_t const*& dataStart, uint8_t lastEventTypeByte)
{
    uint8_t eventTypeByte = *dataStart++;
    
    if ((eventTypeByte & 0xf0) == 0xf0) 
    {
        mm::MessageType message_type = static_cast<mm::MessageType>(eventTypeByte);

        /* system / meta event */
        if (eventTypeByte == 0xff) {
            /* meta event */
            Midi_MetaEventType subtype = static_cast<Midi_MetaEventType>(*dataStart++);
            int length = read_variable_length(dataStart);
            switch(subtype) {

            case Midi_MetaEventType::WHAT_is_THIS: {
                dataStart += length;
                auto event = new Event_Unknown();
                return event;
            }

            default:
                throw std::invalid_argument("Unhandled meta event");

            case Midi_MetaEventType::SEQUENCE_NUMBER: {
                if (length > 2) throw std::invalid_argument("Expected length for sequenceNumber event is 1 or 2");
                auto event = new Event_SequenceNumber();
                if (length == 1)
                    event->number = *dataStart;
                else
                    event->number = *(uint16_t*)dataStart;
                dataStart += 2;
                return event;
            }
            case Midi_MetaEventType::TEXT: {
                auto event = new Event_Text();
                event->data.assign(dataStart, dataStart + length);
                dataStart += length;
                return event;
            }
            case Midi_MetaEventType::COPYRIGHT: {
                auto event = new Event_CopyrightNotice();
                event->data.assign(dataStart, dataStart + length);
                dataStart += length;
                return event;
            }
            case Midi_MetaEventType::TRACK_NAME: {
                auto event = new Event_TrackName();
                event->data.assign(dataStart, dataStart + length);
                dataStart += length;
                return event;
            }
            case Midi_MetaEventType::INSTRUMENT: {
                auto event = new Event_InstrumentName();
                event->data.assign(dataStart, dataStart + length);
                dataStart += length;
                return event;
            }
            case Midi_MetaEventType::LYRIC: {
                auto event = new Event_Lyrics();
                event->data.assign(dataStart, dataStart + length);
                dataStart += length;
                return event;
            }
            case Midi_MetaEventType::MARKER: {
                auto event = new Event_Marker();
                event->data.assign(dataStart, dataStart + length);
                dataStart += length;
                return event;
            }
            case Midi_MetaEventType::CUE: {
                auto event = new Event_Cue();
                event->data.assign(dataStart, dataStart + length);
                dataStart += length;
                return event;
            }
            case Midi_MetaEventType::MIDI_CHANNEL_PREFIX: {
                if (length != 1) throw std::invalid_argument("Expected length for midiChannelPrefix event is 1");
                auto event = new Event_MidiChannelPrefix();
                event->channel = *dataStart;
                ++dataStart;
                return event;
            }
            case Midi_MetaEventType::END_OF_TRACK: {
                if (length != 0) throw std::invalid_argument("Expected length for END_OF_TRACK event is 0");
                auto event = new Event_EndOfTrack();
                return event;
            }
            case Midi_MetaEventType::TEMPO_CHANGE: {
                if (length != 3) throw std::invalid_argument("Expected length for TEMPO_CHANGE event is 3");
                auto event = new Event_SetTempo();
                event->microsecondsPerBeat = read_uint24_be(dataStart);
                return event;
            }
            case Midi_MetaEventType::SMPTE_OFFSET: {
                if (length != 5) throw std::invalid_argument("Expected length for SMPTE_OFFSET event is 5");
                auto event = new Event_SmpteOffset();
                uint8_t hourByte = *dataStart++;
                switch (hourByte & 0x60) {
                case 0x00: event->framerate = 24; break;
                case 0x20: event->framerate = 25; break;
                case 0x40: event->framerate = 29; break;
                case 0x60: event->framerate = 30; break;
                }
                event->hour = hourByte & 0x1f;
                event->min = *dataStart++;
                event->sec = *dataStart++;
                event->frame = *dataStart++;
                event->subframe = *dataStart++;
                return event;
            }
            case Midi_MetaEventType::TIME_SIGNATURE: {
                if (length != 4) throw std::invalid_argument("Expected length for TIME_SIGNATURE event is 4");
                auto event = new Event_TimeSignature();
                double num = double(*dataStart++);
                double denom = double(*dataStart++);
                event->timeSignature = num / std::pow(2., denom);
                event->metronome = *dataStart++;
                event->thirtyseconds = *dataStart++;
                return event;
            }
            case Midi_MetaEventType::KEY_SIGNATURE: {
                if (length != 2) throw std::invalid_argument("Expected length for KEY_SIGNATURE event is 2");
                auto event = new Event_KeySignature();
                event->key = *dataStart++;    // key shift
                event->scale = *dataStart++;  // if not zero, key is minor
                return event;
            }
            case Midi_MetaEventType::PROPRIETARY: {
                auto event = new Event_SequencerSpecific();
                event->data.resize(length);
                memcpy(event->data.data(), dataStart, length);
                dataStart += length;
                return event;
            }
            }
            // console.log("Unrecognised meta event subtype: " + subtypeByte);
            auto event = new Event_Unknown();
            event->data.resize(length);
            memcpy(event->data.data(), dataStart, length);
            dataStart += length;
            return event;
        }
        else if (message_type == mm::MessageType::SYSTEM_EXCLUSIVE) {
            int length = read_variable_length(dataStart);
            auto event = new Event_SysEx();
            event->data.resize(length);
            memcpy(event->data.data(), dataStart, length);
            dataStart += length;
            return event;
        }
        else if (message_type == mm::MessageType::EOX) {
            int length = read_variable_length(dataStart);
            auto event = new Event_DividedSysEx();
            event->data.resize(length);
            memcpy(event->data.data(), dataStart, length);
            dataStart += length;
            return event;
        }
        else {
            throw std::runtime_error("Unrecognised MIDI event type byte");
        }
    }
    else {
        /* channel event */
        auto event = new Event_Channel();
        uint8_t param1;
        if ((eventTypeByte & 0x80) == 0) {
            // Running status is described here:
            // http://home.roadrunner.com/~jgglatt/tech/midispec/run.htm
            // running status - reuse lastEventTypeByte as the event type.
            // eventTypeByte is actually the first parameter
            //
            param1 = eventTypeByte;
            eventTypeByte = lastEventTypeByte;
        }
        else {
            param1 = *dataStart++;
            lastEventTypeByte = eventTypeByte;
        }

        // 0xff will likely be overwritten in the next switch
        event->data = { eventTypeByte, param1, 0xff };

        mm::MessageType message_type = static_cast<mm::MessageType>(eventTypeByte & 0xf0);

        switch (message_type) {
        case mm::MessageType::NOTE_OFF:
            event->data[2] = int(*dataStart++);
            return event;
        case mm::MessageType::NOTE_ON:
            event->data[2] = int(*dataStart++); // velocity
            return event;
        case mm::MessageType::POLY_PRESSURE: // after touch
            event->data[2] = int(*dataStart++); // amount
            return event;
        case mm::MessageType::CONTROL_CHANGE:
            event->data[2] = int(*dataStart++); // amount
            return event;
        case mm::MessageType::PROGRAM_CHANGE:
            return event;
        case mm::MessageType::AFTERTOUCH: // channel after touch
            return event;
        case mm::MessageType::PITCH_BEND:
            event->data[2] = int(*dataStart++);
            return event;
        default:
            throw std::runtime_error("Unrecognised MIDI event type");
        }
    }
    throw "Unparsed event";
}



MidiSong::MidiSong()
: tracks(0)
, ticksPerBeat(240)     // precision (number of ticks distinguishable per second)
, startingTempo(120)
{
}

MidiSong::~MidiSong()
{
    clearTracks();
}

void MidiSong::clearTracks()
{
    tracks.clear();
}


void MidiSong::parse(uint8_t const*const input_data, size_t length, bool verbose)
{
    uint8_t const* file = input_data;
    std::vector<uint8_t> parse_buffer;
    
    // Check if the MIDI file has been base64 encoded using the same scheme
    // Euphony has in its tracks files.
    // https://github.com/qiao/euphony
    //
    const char* base64Test = "data:audio/midi;base64,";
    size_t base64TestLen = strlen(base64Test);
    if (!strncmp((const char*) file, base64Test, base64TestLen)) {
        parse_buffer.resize(length); // the decoded data will be smaller than length.
        decode64(input_data + base64TestLen, parse_buffer.data(), length - base64TestLen);
        file = parse_buffer.data();
    }

    clearTracks();
    
    uint8_t const* dataStart = file;
    
    uint32_t headerId = read_uint32_be(dataStart);
    uint32_t headerLength = read_uint32_be(dataStart);
    if (headerId != 'MThd' || headerLength != 6) {
        if (verbose)
            std::cerr << "Bad .mid file - couldn't parse header" << std::endl;
        return;
    }

    // Midi Format 0 is a single track
    // Midi Format 1 has multiple same length tracks
    // Midi Format 2 has multiple tracks of arbitrary lengths and starts, typically used as clips, or multiple songs
    int formatType = read_uint16_be(dataStart);

    if (formatType == 2) {
        if (verbose)
            std::cerr << "Multiple songs format not supported" << std::endl;
        return;
    }

    uint16_t trackCount = read_uint16_be(dataStart);
    uint16_t timeDivision = read_uint16_be(dataStart);

    int ticksPerFrame;
    float framesPerSecond;
    int unitsPerQuarterNote;
    
    // CBB: deal with the SMPTE style time coding
    // timeDivision is described here http://www.sonicspot.com/guide/midifiles.html
    if (timeDivision & 0x8000) {
        ticksPerFrame = timeDivision & 0xFF;
        uint16_t framesPerSecondsIndicator = ((timeDivision >> 8) & 0b1100000) >> 5;
        framesPerSecond = (framesPerSecondsIndicator == 0 ? 24.0f : (framesPerSecondsIndicator == 1 ? 25.0f : (framesPerSecondsIndicator == 2 ? 29.97f : 30.0f)));
        std::cout << "[INFO]: " << ticksPerFrame << " units per frame, " << framesPerSecond << " frames per second." << std::endl;
        unitsPerQuarterNote = 0;
    }
    else
    {
        // Remove 15th bit.
        unitsPerQuarterNote = timeDivision ^ (timeDivision & (0x1 << 15));
        std::cout << "[INFO]: " << unitsPerQuarterNote << " units per quarter note ." << std::endl;

        ticksPerFrame = 0;
        framesPerSecond = 0.0f;
    }
    
    startingTempo = 0.0f;
    ticksPerBeat = float(timeDivision); // ticks per beat (a beat is defined as a quarter note)
                                        // commonly 48 to 960.
    
    try {
        for (int i = 0; i < trackCount; ++i) {
            headerId = read_uint32_be(dataStart);
            headerLength = read_uint32_be(dataStart);
            if (headerId != 'MTrk') {
                if (verbose)
                    std::cerr << "Bad .mid file - couldn't find track" << std::endl;
                return;
            }

            tracks.emplace_back(std::make_shared<MidiTrack>());
            auto track = tracks.back();
            uint8_t const* dataEnd = dataStart + headerLength;
            uint8_t runningEvent = 0;
            float tempo = 0.f;
            while (dataStart < dataEnd) {
                int duration = read_variable_length(dataStart);
                MidiEvent* ev = parseEvent(dataStart, runningEvent);
                ev->tick = duration;
                if (ev->data.size() > 0)
                    runningEvent = ev->data[0];
                track->events.push_back(ev);
                if (ev->eventType == Midi_MetaEventType::TEMPO_CHANGE)
                {
                    Event_SetTempo* set_tempo = reinterpret_cast<Event_SetTempo*>(ev);
                    startingTempo = 60000000.0f / float(set_tempo->microsecondsPerBeat);
                }
            }
        }
    }
    catch(...)
    {
    }

    if (startingTempo <= 0.f)
        startingTempo = 120.f;
}

void MidiSong::parse(char const*const path, bool verbose)
{
    FILE* f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        int l = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> a(l);
        fread(a.data(), 1, l, f);
        fclose(f);

        parse(a.data(), l, verbose);
    }
}


void MidiSong::writeMidi(std::ostream& out)
{
    // MIDI File Header
    out << 'M'; out << 'T'; out << 'h'; out << 'd';

    uint16_t num_tracks = static_cast<uint16_t>(tracks.size());
    uint16_t ticks_per_quarter_note = 120; /// @TODO default value, should store something in the loader, how does it relate to ticksPerBeat?

    mm::write_uint32_be(out, 6);
    mm::write_uint16_be(out, num_tracks == 1 ? 0 : 1);
    mm::write_uint16_be(out, num_tracks);
    mm::write_uint16_be(out, ticks_per_quarter_note);

    std::vector<uint8_t> trackRawData;

    for (auto midi_track : tracks)
    {
        for (MidiEvent* event : midi_track->events)
        {
            const Midi_MetaEventType msg = event->eventType;

            // Suppress end-of-track meta messages (one will be added
            // automatically after all track data has been written).
            if (msg == Midi_MetaEventType::END_OF_TRACK)
                continue;

            mm::write_variable_length(event->tick, trackRawData);
#if 0
            if ((msg == Midi_MetaEventType::SYSTEM_EXCLUSIVE) || (msg == Midi_MetaEventType::END_OF_SYSTEM_EXCLUSIVE))
            {
                // 0xf0 == Complete sysex message (0xf0 is part of the raw MIDI).
                // 0xf7 == Raw byte message (0xf7 not part of the raw MIDI).
                // Print the first byte of the message (0xf0 or 0xf7), then
                // print a VLV length for the rest of the bytes in the message.
                // In other words, when creating a 0xf0 or 0xf7 MIDI message,
                // do not insert the VLV byte length yourself, as this code will
                // do it for you automatically.
                trackRawData.emplace_back(msg->data[0]); // 0xf0 or 0xf7;

                mm::write_variable_length(uint32_t(msg->messageSize() - 1), trackRawData);

                for (size_t k = 1; k < msg->messageSize(); k++)
                {
                    trackRawData.emplace_back((*msg)[k]);
                }
            }
            else
            {
                // Non-sysex type of message, so just output the bytes of the message:
                for (size_t k = 0; k < msg->messageSize(); k++)
                {
                    trackRawData.emplace_back((*msg)[k]);
                }
            }
#endif
        }
    }

    auto size = trackRawData.size();

    // if the track is too small to have an end, or the final data is not an end of track, then write an end of track.
    if ((size < 3) || !((trackRawData[size - 3] == 0xFF) && (trackRawData[size - 2] == 0x2F)))
    {
        trackRawData.emplace_back(0x0); // tick
        trackRawData.emplace_back(0xFF);
        trackRawData.emplace_back(0x2F);
        trackRawData.emplace_back(0x00);
    }

    // Write the track ID marker "MTrk":
    out << 'M'; out << 'T'; out << 'r'; out << 'k';
    mm::write_uint32_be(out, uint32_t(trackRawData.size()));
    out.write((char*)trackRawData.data(), trackRawData.size());
}

//------------------------------------------------------------
// MML support
//

int secondsToTicks(double seconds, double bpm, int ticksPerBeat)
{
    double beats = seconds * (bpm / 60.0);
    double ticks = beats * ticksPerBeat;
    return int(ticks);
}

int wholeNoteToTicks(double fraction, double bpm, int ticksPerBeat)
{
    double seconds = (bpm / 60.0f) / fraction;
    return secondsToTicks(seconds, bpm, ticksPerBeat);
}

int getMMLInt(char const*& curr)
{
    int v = 0;
    char c = *curr;
    int sign = 1;
    while (c == '-' || (c >= '0' && c <= '9')) {
        if (c == '-')
            sign = -1;
        else
            v = v * 10 + (c - '0');
        ++curr;
        c = *curr;
    }
    return v * sign;
}

int sharpFlat(char const*& midifiledata)
{
    char c = *midifiledata;
    if (c == '-') {
        ++midifiledata;
        return -1;
    }
    if (c == '+' || c == '#') {
        ++midifiledata;
        return 1;
    }
    return 0;
}

int dotted(char const*& midifiledata, int bpm, int ticksPerBeat, int l)
{
    int ret = l;
    
    // denominator is a fraction of a whole note
    int denominator = getMMLInt(midifiledata);
    if (denominator)
        ret = denominator;
    
    int l2 = ret * 2;
    char c = *midifiledata;
    while (c == '.') {
        ++midifiledata;
        ret += l2;
        l2 = l2 * 2;
    }
    return wholeNoteToTicks(ret, bpm, ticksPerBeat);
}

// duration is in ticks
//
void storeMMLEvent(MidiTrack* track, uint8_t eventTypeByte, uint8_t note, uint8_t amount, int duration)
{
    /* channel event */
    Event_Channel* event = new Event_Channel();
    event->tick = duration;
    event->data = { eventTypeByte, note, amount };
    track->events.emplace_back(event);
}

// The variant of MML parsed here was produced by studying http://www.g200kg.com/en/docs/webmodular/,
// mml2mid by Arle (unfortunately Arle's pages and the mml2mid sources are no longer online) and
// the wikipedia article http://en.wikipedia.org/wiki/Music_Macro_Language
// 

// sample MML from http://www.g200kg.com/en/docs/webmodular/
// t150 e-d-<g-4>g-rg-4e-d-<g-4>g-rg-4e-d-<g-4>g-4<e-4>g-4<d-4>frf4e-d-<d-4>frf4e-d-<d-4>frf4e-d-<d-4>f4<e-4>f4<g-4>g-rg-4

void MidiSong::parseMML(char const*const mmlStr, size_t length, bool verbose)
{
    char const* curr = mmlStr;
    char const* end = mmlStr + length;
    
    int octave = 4;
    int tr = 0;
    int err = 0;
    bool tied = false;
    int tempo = 120;
    int len = 8;        // an 1/8th note
    
    clearTracks();
    tracks.emplace_back(std::make_shared<MidiTrack>());
    auto track = tracks.back();
    
    do {
        char c = *curr++;
        
        switch(c) {
        case 'l':   // length NN
        case 'L':
            len = getMMLInt(curr);
            break;
                
        case 'o':
        case 'O':
            octave = getMMLInt(curr);
            if (octave < 0)
                octave = 0;
            if (octave > 7)
                octave = 7;
            break;
                
        case '<':
            ++octave;
            if (octave > 7)
                octave = 7;
            break;
        case '>':
            --octave;
            if (octave < 0)
                octave = 0;
            break;
                
        case '@': { // tone selection
            int i = getMMLInt(curr);
            if (i < 0)
                i = 0;
            if (i > 127)
                i = 127;
            storeMMLEvent(track.get(), MIDI_PROGRAM_CHANGE | tr, i, 0xff, 0);
            break;
        }
            
        case 't':       // tempo in bpm
        case 'T': {
            tempo = getMMLInt(curr);
            if (tempo < 0)
                tempo = 0;
            if (tempo > 500)
                tempo = 500;
            Event_SetTempo* event = new Event_SetTempo();
            event->microsecondsPerBeat = 60000000 / tempo;
            event->tick = 0;
            track->events.push_back(event);
            break;
        }
            
        case '/':
            tr++;
            if (tr > 15)
                tr = 15;
            while (tr >= tracks.size())
                tracks.emplace_back(std::make_shared<MidiTrack>());
            track = tracks[tr];
            break;
        
        case 'c':
        case 'C': {
            int note = 0 + sharpFlat(curr) + octave * 12;
            int duration = dotted(curr, tempo, static_cast<int>(ticksPerBeat), len);
            // note output { tr, cnt, size 3, 0x90|tr note & 0x7f vol=0x7f }
            if (!tied) {
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0, duration);
            }
            else {
                // note output { tr, cnt-1, size 3, 0x90|tr note & 0x7f vol=0x00 }
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, duration);
                tied = false;
            }
            break;
        }
        case 'd':
        case 'D': {
            int note = 2 + sharpFlat(curr) + octave * 12;
            int duration = dotted(curr, tempo, static_cast<int>(ticksPerBeat), len);
            // note output { tr, cnt, size 3, 0x90|tr note & 0x7f vol=0x7f }
            if (!tied) {
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0, duration);
            }
            else {
                // note output { tr, cnt-1, size 3, 0x90|tr note & 0x7f vol=0x00 }
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, duration);
                tied = false;
            }
            break;
        }
        case 'e':
        case 'E': {
            int note = 4 + sharpFlat(curr) + octave * 12;
            int duration = dotted(curr, tempo, static_cast<int>(ticksPerBeat), len);
            // note output { tr, cnt, size 3, 0x90|tr note & 0x7f vol=0x7f }
            if (!tied) {
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0, duration);
            }
            else {
                // note output { tr, cnt-1, size 3, 0x90|tr note & 0x7f vol=0x00 }
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, duration);
                tied = false;
            }
            break;
        }
        case 'f':
        case 'F': {
            int note = 5 + sharpFlat(curr) + octave * 12;
            int duration = dotted(curr, tempo, static_cast<int>(ticksPerBeat), len);
            // note output { tr, cnt, size 3, 0x90|tr note & 0x7f vol=0x7f }
            if (!tied) {
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0, duration);
            }
            else {
                // note output { tr, cnt-1, size 3, 0x90|tr note & 0x7f vol=0x00 }
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, duration);
                tied = false;
            }
            break;
        }
        case 'g':
        case 'G': {
            int note = 7 + sharpFlat(curr) + octave * 12;
            int duration = dotted(curr, tempo, static_cast<int>(ticksPerBeat), len);
            // note output { tr, cnt, size 3, 0x90|tr note & 0x7f vol=0x7f }
            if (!tied) {
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0, duration);
            }
            else {
                // note output { tr, cnt-1, size 3, 0x90|tr note & 0x7f vol=0x00 }
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, duration);
                tied = false;
            }
            break;
        }
        case 'a':
        case 'A': {
            int note = 9 + sharpFlat(curr) + octave * 12;
            int duration = dotted(curr, tempo, static_cast<int>(ticksPerBeat), len);
            // note output { tr, cnt, size 3, 0x90|tr note & 0x7f vol=0x7f }
            if (!tied) {
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0, duration);
            }
            else {
                // note output { tr, cnt-1, size 3, 0x90|tr note & 0x7f vol=0x00 }
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, duration);
                tied = false;
            }
            break;
        }
        case 'b':
        case 'B': {
            int note = 11 + sharpFlat(curr) + octave * 12;
            int duration = dotted(curr, tempo, static_cast<int>(ticksPerBeat), len);
            // note output { tr, cnt, size 3, 0x90|tr note & 0x7f vol=0x7f }
            if (!tied) {
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0, duration);
            }
            else {
                // note output { tr, cnt-1, size 3, 0x90|tr note & 0x7f vol=0x00 }
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, 0);
                storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, note & 0x7f, 0x7f, duration);
                tied = false;
            }
            break;
        }
            
        case '&': // tie (how to handle?)
            tied = true;
            break;
            
        case 'r': // rest
        case 'R':
            storeMMLEvent(track.get(), MIDI_NOTE_ON | tr, 0, 0, dotted(curr, tempo, static_cast<int>(ticksPerBeat), len));
            tied = false;
            break;
            
        case ' ': // ignore
        case '	':
        case 10:
        case 13:
            break;
        
        default:
            err = 1;
            break;
        }
    } while (curr < end);
}

void MidiSong::parseMML(char const*const path, bool verbose)
{
    FILE* f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        int l = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* a = new char[l];
        fread(a, 1, l, f);
        fclose(f);
        
        parseMML(a, l, verbose);
        delete [] a;
    }
}
    
} // Lab
