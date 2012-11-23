//
//  LabMidiOut.h
//
//  CoreAudio, CoreMidi, and CoreFoundation frameworks are required on OSX/iOS
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

    struct MidiCommand;
    
    class MidiOutBase {
    public:
        virtual ~MidiOutBase() { }
        virtual void command(const MidiCommand*) = 0;
    };
    
    class MidiOut : public MidiOutBase {
    public:
        MidiOut();
        virtual ~MidiOut();
        
        // createVirtualPort is available on MacOSX and ALSA for allowing other software to connect
        void createVirtualPort(const std::string& port);
        
        bool openPort(unsigned int port);
        void closePort();
        unsigned int getPort() const;
        
        // channels are 0-0xF (not 1-16)
        void sendNoteOn       (int channel, int id, int value);
        void sendNoteOff      (int channel, int id, int value);
        void sendControlChange(int channel, int id, int value);
        void sendProgramChange(int channel, int value);
        void sendPitchBend    (int channel, int lsb, int msb);
        
        virtual void command(const MidiCommand*);
        
    private:
        class Detail;
        Detail* _detail;
    };
    
} // Lab
    
