//
//  LabMidiSong.cpp
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

#include "LabMidiSong.h"

#include "LabMidiCommand.h"
#include "LabMidiOut.h"

#include <iostream>
#include <math.h>
#include <stdint.h>
#include <vector>

namespace Lab {
    
    /* read a MIDI-style variable-length integer
     (big-endian value in groups of 7 bits,
     with top bit set to signify that another byte follows)
     */
	int readVarInt(uint8_t*& dataStart)
    {
        int result = 0;
		while (true) {
			uint8_t b = *dataStart++;
			if (b & 0x80) {
				result += (b & 0x7f);
				result <<= 7;
			} else {
				return result + b; // b is the last byte
			}
		}
	}
    
    
    int readInt16(uint8_t*& dataStart)
    {
        int result = int(*dataStart++) << 8;
        result += int(*dataStart++);
        return result;
    }
    
    int readInt24(uint8_t*& dataStart)
    {
        int result = int(*dataStart++) << 16;
        result += int(*dataStart++) << 8;
        result += int(*dataStart++);
        return result;
    }
    
    int readInt32(uint8_t*& dataStart)
    {
        int result = int(*dataStart++) << 24;
        result += int(*dataStart++) << 16;
        result += int(*dataStart++) << 8;
        result += int(*dataStart++);
        return result;
    }
    

    MidiEvent* parseEvent(uint8_t*& dataStart, uint8_t lastEventTypeByte)
    {
        uint8_t eventTypeByte = *dataStart++;
        
		if ((eventTypeByte & 0xf0) == 0xf0) {
			/* system / meta event */
			if (eventTypeByte == 0xff) {
				/* meta event */
                uint8_t subtypeByte = *dataStart++;
				int length = readVarInt(dataStart);
				switch(subtypeByte) {
					case 0x00: {
						if (length != 2) throw "Expected length for sequenceNumber event is 2";
                        SequenceNumberEvent* event = new SequenceNumberEvent();
						event->number = * (uint16_t*) dataStart;
                        dataStart += 2;
						return event;
                    }
					case 0x01: {
                        TextEvent* event = new TextEvent();
						event->text.assign((char*) dataStart, length);
                        dataStart += length;
						return event;
                    }
					case 0x02: {
                        CopyrightNoticeEvent* event = new CopyrightNoticeEvent();
						event->text.assign((char*) dataStart, length);
                        dataStart += length;
						return event;
                    }
					case 0x03: {
                        TrackNameEvent* event = new TrackNameEvent();
						event->text.assign((char*) dataStart, length);
                        dataStart += length;
						return event;
                    }
					case 0x04: {
                        InstrumentNameEvent* event = new InstrumentNameEvent();
						event->text.assign((char*) dataStart, length);
                        dataStart += length;
						return event;
                    }
					case 0x05: {
                        LyricsEvent* event = new LyricsEvent();
						event->text.assign((char*) dataStart, length);
                        dataStart += length;
						return event;
                    }
					case 0x06: {
                        MarkerEvent* event = new MarkerEvent();
						event->text.assign((char*) dataStart, length);
                        dataStart += length;
						return event;
                    }
					case 0x07: {
                        CuePointEvent* event = new CuePointEvent();
						event->text.assign((char*) dataStart, length);
                        dataStart += length;
						return event;
                    }
					case 0x20: {
						if (length != 1) throw "Expected length for midiChannelPrefix event is 1";
                        MidiChannelPrefixEvent* event = new MidiChannelPrefixEvent();
						event->channel = *(uint8_t*) dataStart;
                        ++dataStart;
						return event;
                    }
					case 0x2f: {
						if (length != 0) throw "Expected length for endOfTrack event is 0";
                        EndOfTrackEvent* event = new EndOfTrackEvent();
						return event;
                    }
					case 0x51: {
						if (length != 3) throw "Expected length for setTempo event is 3";
                        SetTempoEvent* event = new SetTempoEvent();
						event->microsecondsPerBeat = readInt24(dataStart);
						return event;
                    }
					case 0x54: {
						if (length != 5) throw "Expected length for smpteOffset event is 5";
                        SmpteOffsetEvent* event = new SmpteOffsetEvent();
						uint8_t hourByte = *dataStart++;
                        switch (hourByte & 0x60) {
                            case 0x00: event->framerate = 24; break;
                            case 0x20: event->framerate = 25; break;
                            case 0x40: event->framerate = 29; break;
                            case 0x60: event->framerate = 30; break;
                        }
						event->hour = hourByte & 0x1f;
						event->min = int(*dataStart++);
						event->sec = int(*dataStart++);
						event->frame = int(*dataStart++);
						event->subframe = int(*dataStart++);
						return event;
                    }
					case 0x58: {
						if (length != 4) throw "Expected length for timeSignature event is 4";
						TimeSignatureEvent* event = new TimeSignatureEvent();
						event->numerator = int(*dataStart++);
						event->denominator = int(powf(2.0f, float(*dataStart++)));
						event->metronome = int(*dataStart++);
						event->thirtyseconds = int(*dataStart++);
						return event;
                    }
					case 0x59: {
						if (length != 2) throw "Expected length for keySignature event is 2";
                        KeySignatureEvent* event = new KeySignatureEvent();
						event->key = int(*dataStart++);
						event->scale = int(*dataStart++);
						return event;
                    }
					case 0x7f: {
                        SequencerSpecificEvent* event = new SequencerSpecificEvent();
                        event->data = new uint8_t[length];
                        memcpy(event->data, dataStart, length);
                        dataStart += length;
						return event;
                    }
				}
                // console.log("Unrecognised meta event subtype: " + subtypeByte);
                UnknownEvent* event = new UnknownEvent();
                event->data = new uint8_t[length];
                memcpy(event->data, dataStart, length);
                dataStart += length;
                return event;
			}
            else if (eventTypeByte == 0xf0) {
                int length = readVarInt(dataStart);
                SysExEvent* event = new SysExEvent();
                event->data = new uint8_t[length];
                memcpy(event->data, dataStart, length);
                dataStart += length;
                return event;
			}
			else if (eventTypeByte == 0xf7) {
                int length = readVarInt(dataStart);
                DividedSysExEvent* event = new DividedSysExEvent();
                event->data = new uint8_t[length];
                memcpy(event->data, dataStart, length);
                dataStart += length;
                return event;
			}
            else {
				throw "Unrecognised MIDI event type byte"; // eventTypeByte;
			}
		}
        else {
			/* channel event */
            ChannelEvent* event = new ChannelEvent();
			int param1;
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
				param1 = int(*dataStart++);
				lastEventTypeByte = eventTypeByte;
			}
            event->midiCommand = eventTypeByte;
            event->param1 = param1;
            event->param2 = 0xff;   // don't transmit this value

			switch (eventTypeByte & 0xf0) {
				case 0x80:  // note off
                    event->param2 = int(*dataStart++);
					return event;
				case 0x90: // note on
                    event->param2 = int(*dataStart++); // velocity
					return event;
				case 0xa0: // after touch
                    event->param2 = int(*dataStart++); // amount
					return event;
				case 0xb0: // controller
                    event->param2 = int(*dataStart++); // amount
					return event;
				case 0xc0: // program change
					return event;
				case 0xd0: // channel after touch
					return event;
				case 0xe0: // pitch bend
                    event->param2 = int(*dataStart++);
					return event;
				default:
					throw "Unrecognised MIDI event type";
			}
		}
        throw "Unparsed event";
	}

    MidiSong::MidiSong()
    : tracks(0)
    {
    }

    MidiSong::~MidiSong()
    {
        clearTracks();
    }
    
    void MidiSong::clearTracks()
    {
        if (tracks) {
            for (auto i = tracks->begin(); i != tracks->end(); ++i)
                delete *i;
            delete tracks;
        }
    }
    
    void MidiSong::parse(uint8_t* file, int length, bool verbose)
    {
        clearTracks();
        tracks = new std::vector<MidiTrack*>();
        
        uint8_t* dataStart = file;
        
        int headerId = readInt32(dataStart);
        int headerLength = readInt32(dataStart);
        if (headerId != 'MThd' || headerLength != 6) {
            if (verbose)
                std::cerr << "Bad .mid file - couldn't parse header" << std::endl;
            return;
        }
        
        /*int formatType = */ readInt16(dataStart);
        int trackCount = readInt16(dataStart);
        int timeDivision = readInt16(dataStart);
        
        // CBB: deal with the SMPTE style time coding
        // timeDivision is described here http://www.sonicspot.com/guide/midifiles.html
        if (timeDivision & 0x8000) {
            if (verbose)
                std::cerr << "Found SMPTE time frames" << std::endl;
            //int fps = (timeDivision >> 16) & 0x7f;
            //int ticksPerFrame = timeDivision & 0xff;
            // given beats per second, timeDivision should be derivable.
            return;
        }
        
        startingTempo = 120.0f;
        ticksPerBeat = float(timeDivision); // ticks per beat (a beat is defined as a quarter note)
                                                  // commonly 48 to 960.
        
        try {
            for (int i = 0; i < trackCount; ++i) {
                headerId = readInt32(dataStart);
                headerLength = readInt32(dataStart);
                if (headerId != 'MTrk') {
                    if (verbose)
                        std::cerr << "Bad .mid file - couldn't find track" << std::endl;
                    return;
                }

                tracks->push_back(new MidiTrack());
                MidiTrack* track = tracks->back();
                uint8_t* dataEnd = dataStart + headerLength;
                uint8_t runningEvent = 0;
                while (dataStart < dataEnd) {
                    int duration = readVarInt(dataStart);
                    MidiEvent* ev = parseEvent(dataStart, runningEvent);
                    ev->deltatime = duration;
                    ChannelEvent* ce = dynamic_cast<ChannelEvent*>(ev);
                    if (ce)
                        runningEvent = ce->midiCommand;
                    track->events.push_back(ev);
                }
            }
        }
        catch(...)
        {
        }
    }
    
    void MidiSong::parse(char const*const path, bool verbose)
    {
        FILE* f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            int l = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t* a = new uint8_t[l];
            fread(a, 1, l, f);
            fclose(f);
            parse(a, l, verbose);
            delete [] a;
        }
    }

} // Lab
