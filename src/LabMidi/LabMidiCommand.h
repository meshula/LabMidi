//
//  LabMidiIn.h
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
    
    
    /*
     http://www.midi.org/techspecs/midimessages.php
     Command	 Meaning	 # parameters	 param 1	 param 2
     0x80	 Note-off	 2	 key	 velocity
     0x90	 Note-on	 2	 key	 velocity
     0xA0	 Aftertouch	 2	 key	 touch
     0xB0	 Continuous controller (pedals, levers, etc)	 2	 controller #	 controller value
     0xC0	 Patch change	 2	 instrument #
     0xD0	 Channel Pressure	 1	 pressure
     0xE0	 Pitch bend (center is 0x2000)	 2	 lsb (7 bits)	 msb (7 bits)
     
     0xF0     0iiiiiii (7 bit manufacturer ID) 0ddddddd (message, listened to if iiiiiii corresponds to receiving device)
     0xF1     0nnndddd Time code quarter frame. nnn = message type, dddd = values
     0xF2     0iiiiiii LSB of 14 bit song position pointer, 0mmmmmmm MSB of 14 bit song position pointer
     0xF3     0sssssss Song to be played
     0xF6     upon reception, all analog synthesizers should tune their oscillators
     0xF7     end of excclusive, used to terminate system exclusive dump.
     */
    
    // channel info (MSN=command LSN=channel)
    #define MIDI_NOTE_OFF           0x80
    #define MIDI_NOTE_ON            0x90
    #define MIDI_POLY_PRESSURE      0xA0
    #define MIDI_CONTROL_CHANGE     0xB0
    #define MIDI_PROGRAM_CHANGE     0xC0
    #define MIDI_CHANNEL_PRESSURE   0xD0
    #define MIDI_PITCH_BEND         0xE0
    
    // system common
    #define MIDI_SYSTEM_EXCLUSIVE   0xF0
    #define MIDI_TIME_CODE          0xF1
    #define MIDI_SONG_POS_POINTER   0xF2
    #define MIDI_SONG_SELECT        0xF3
    #define MIDI_RESERVED1          0xF4
    #define MIDI_RESERVED2          0xF5
    #define MIDI_TUNE_REQUEST       0xF6
    #define MIDI_EOX                0xF7
    
    // system realtime
    #define MIDI_TIME_CLOCK         0xF8
    #define MIDI_RESERVED3          0xF9
    #define MIDI_START              0xFA
    #define MIDI_CONTINUE           0xFB
    #define MIDI_STOP               0xFC
    #define MIDI_RESERVED4          0xFD
    #define MIDI_ACTIVE_SENSING     0xFE
    #define MIDI_SYSTEM_RESET       0xFF

    struct MidiCommand {
        
        MidiCommand& operator=(const MidiCommand& rhs)
        {
            command = rhs.command;
            byte1 = rhs.byte1;
            byte2 = rhs.byte2;
            return *this;
        }

        uint8_t command;
        uint8_t byte1;
        uint8_t byte2;
   };
    
} // Lab
    
