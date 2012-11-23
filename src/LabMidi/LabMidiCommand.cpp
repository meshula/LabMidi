//
//  LabMidiCommand.cpp
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

#include "LabMidiCommand.h"

namespace Lab {

    const char* commandNames[128] = {
        // 0x80
        "Note off: 1", "Note off: 2", "Note off: 3", "Note off: 4", "Note off: 5", "Note off: 6", "Note off: 7", "Note off: 8",
        "Note off: 9", "Note off: 10", "Note off: 11", "Note off: 12", "Note off: 13", "Note off: 14", "Note off: 15", "Note off: 16",
        // 0x90
        "Note on: 1", "Note on: 2", "Note on: 3", "Note on: 4", "Note on: 5", "Note on: 6", "Note on: 7", "Note on: 8",
        "Note on: 9", "Note on: 10", "Note on: 11", "Note on: 12", "Note on: 13", "Note on: 14", "Note on: 15", "Note on: 16",
        // 0xA0
        "Poly Pressure: 1", "Poly Pressure: 2", "Poly Pressure: 3", "Poly Pressure: 4", "Poly Pressure: 5", "Poly Pressure: 6", "Poly Pressure: 7", "Poly Pressure: 8",
        "Poly Pressure: 9", "Poly Pressure: 10", "Poly Pressure: 11", "Poly Pressure: 12", "Poly Pressure: 13", "Poly Pressure: 14", "Poly Pressure: 15", "Poly Pressure: 16",
        // 0xB0
        "Control Change: 1", "Control Change: 2", "Control Change: 3", "Control Change: 4", "Control Change: 5", "Control Change: 6", "Control Change: 7", "Control Change: 8",
        "Control Change: 9", "Control Change: 10", "Control Change: 11", "Control Change: 12", "Control Change: 13", "Control Change: 14", "Control Change: 15", "Control Change: 16",
        // 0xC0
        "Program Change: 1", "Program Change: 2", "Program Change: 3", "Program Change: 4", "Program Change: 5", "Program Change: 6", "Program Change: 7", "Program Change: 8",
        "Program Change: 9", "Program Change: 10", "Program Change: 11", "Program Change: 12", "Program Change: 13", "Program Change: 14", "Program Change: 15", "Program Change: 16",
        // 0xD0
        "Channel Pressure: 1", "Channel Pressure: 2", "Channel Pressure: 3", "Channel Pressure: 4", "Channel Pressure: 5", "Channel Pressure: 6", "Channel Pressure: 7", "Channel Pressure: 8",
        "Channel Pressure: 9", "Channel Pressure: 10", "Channel Pressure: 11", "Channel Pressure: 12", "Channel Pressure: 13", "Channel Pressure: 14", "Channel Pressure: 15", "Channel Pressure: 16",
        // 0xE0
        "Pitch Bend: 1", "Pitch Bend: 2", "Pitch Bend: 3", "Pitch Bend: 4", "Pitch Bend: 5", "Pitch Bend: 6", "Pitch Bend: 7", "Pitch Bend: 8",
        "Pitch Bend: 9", "Pitch Bend: 10", "Pitch Bend: 11", "Pitch Bend: 12", "Pitch Bend: 13", "Pitch Bend: 14", "Pitch Bend: 15", "Pitch Bend: 16",
        // 0xF0 System Commands
        "System Exclusive",
        "Time Code",
        "Song Position Pointer",
        "Song Select",
        "Reserved 1", "Reserved 2",
        "Tune Request",
        "EOX",
        // 0xF8 System Realtime
        "Time Clock",
        "Reserved 3",
        "Start",
        "Continue",
        "Stop",
        "Reserved 4",
        "Active Sensing",
        "System Reset"
    };
    
    const char* commandName(uint8_t command)
    {
        if (command < 0x80)
            return "Unknown";
        
        return commandNames[command - 0x80];
    }
    
    const char* noteNames[128] = {
        "C0", "C0#", "D0", "D0#", "E0", "F0", "F0#", "G0", "G0#", "A0", "A0#", "B0",
        "C1", "C1#", "D1", "D1#", "E1", "F1", "F1#", "G1", "G1#", "A1", "A1#", "B1",
        "C2", "C2#", "D2", "D2#", "E2", "F2", "F2#", "G2", "G2#", "A2", "A2#", "B2",
        "C3", "C3#", "D3", "D3#", "E3", "F3", "F3#", "G3", "G3#", "A3", "A3#", "B3",
        "C4", "C4#", "D4", "D4#", "E4", "F4", "F4#", "G4", "G4#", "A4", "A4#", "B4",
        "C5", "C5#", "D5", "D5#", "E5", "F5", "F5#", "G5", "G5#", "A5", "A5#", "B5",
        "C6", "C6#", "D6", "D6#", "E6", "F6", "F6#", "G6", "G6#", "A6", "A6#", "B6",
        "C7", "C7#", "D7", "D7#", "E7", "F7", "F7#", "G7", "G7#", "A7", "A7#", "B7",
        "C8", "C8#", "D8", "D8#", "E8", "F8", "F8#", "G8", "G8#", "A8", "A8#", "B8",
        "C9", "C9#", "D9", "D9#", "E9", "F9", "F9#", "G9", "G9#", "A9", "A9#", "B9",
        "C10", "C10#", "D10", "D10#", "E10", "F10", "F10#", "G10"
    };
    
    const char* noteName(uint8_t note)
    {
        return noteNames[note & 0x7f];
    }
    
} // Lab
