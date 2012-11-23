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
    
    const char* commandName(uint8_t command);
    const char* noteName(uint8_t note);
    const char* noteName(uint8_t note, uint8_t channel);      // percussion is on channel 10
    const char* instrumentGroupName(uint8_t instrument);      // 0 based
    const char* instrumentName(uint8_t instrument);           // 0 based
    const char* percussionName(uint8_t channel10noteNumber);  // 0 based

    // Convert a note name to a MIDI note number
    //
    // name has the form nmo, and does not need to be null terminated.
    // where n is note name in upper or lower case (a-g)
    //       m is modifier - b for flat # for sharp
    //       o is octave from -1 to 9
    //
    // len is the number of characters consumed by the note name
    uint8_t     noteNameToNoteNum(char const*const name);
    uint8_t     noteNameToNoteNum(char const*const name, int& len);
    
} // Lab
    
