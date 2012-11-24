//
//  LabMidiUtil.h
//
//  CoreAudio, CoreMidi, and CoreFoundation frameworks are required on OSX/iOS
//
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

#include <stdint.h>

namespace Lab {

    // Convert a MIDI command byte into a descriptive string
    //
    const char* commandName(uint8_t command);

    // Convert an instrument number, 0 based, into a group name. The names
    // correspond to those in the General MIDI specification.
    //
    const char* instrumentGroupName(uint8_t instrument);
    
    // Convert an instrument number, 0 based, into a name. The names
    // correspond to those in the General MIDI specification.
    //
    const char* instrumentName(uint8_t instrument);
    
    // Convert a channel 10 note number into a percussive instrument name.
    // The names correspond to those in the General MIDI specification.
    //
    const char* percussionName(uint8_t channel10noteNumber);

    // Convert a MIDI note to a name in standard format.
    // The result can be round tripped through noteNameToNoteNum.
    //
    const char* noteName(uint8_t note);
    
    // Convert a MIDI note to a name in standard format, unless the note
    // is on channel 10. On Channel 10, the name of the percussive instrument
    // will be returned.
    //
    // The result cannot be round tripped through noteNameToNoteNum.
    //
    const char* noteName(uint8_t note, uint8_t channel);

    // Convert a note name to a MIDI note number
    //
    // name has the form nmo, and does not need to be null terminated.
    // where n is note name in upper or lower case (a-g)
    //       m is modifier - b for flat # for sharp
    //       o is octave from -1 to 9
    //
    // len is the number of characters consumed by the note name
    //
    // If the note number cannot be converted, 0xff is returned.
    // Does not convert percussion names from channel 10, so 0xff will be
    // returned in that case.
    //
    uint8_t noteNameToNoteNum(char const*const name);
    uint8_t noteNameToNoteNum(char const*const name, int& len);

    // Convert a note number to frequency
    //
    // The default value for A is 440.0. A different tuning value
    // can be provided.
    //
    float noteToFrequency(uint8_t note);
    float noteToFrequency(uint8_t, float A);
    
    // Convert a frequency to a note number
    //
    // The default value for A is 440.0. A different tuning value
    // can be provided.
    //
    uint8_t frequencyToNote(float freq);
    uint8_t frequencyToNote(float freq, float A);
    
    // Convert a bpm value to a named tempo such as Largo
    //
    char const*const bpmToTempoName(int bpm);
    
} // Lab
    
