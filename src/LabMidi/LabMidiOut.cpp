
//
//  LabMidiOut.cpp
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
#include "LabMidiOut.h"

#include "RtMidi.h"

namespace Lab {

    class MidiOut::Detail
    {
    public:
        Detail()
        : port(-1)
        {
            try {
                midiOut = new RtMidiOut();
            }
            catch(RtError& error) {
                midiOut = 0;
                error.printMessage();
            }        
        }
        
        ~Detail()
        {
            closePort();
        }
        
        void closePort()
        {
            if (midiOut) {
                midiOut->closePort();
                delete midiOut;
            }
        }
        
        bool openPort(unsigned int port)
        {
            if (port != -1)
                closePort();
            
            try {
                midiOut->openPort(port);
                port = port;
            }
            catch(RtError& rterr) {
                port = -1;
                rterr.printMessage();
            }
            
            return port != -1;
        }
        
        RtMidiOut* midiOut;
        unsigned int port;
    };
    
    MidiOut::MidiOut()
    : _detail(new Detail())
    {
    }
    
    MidiOut::~MidiOut()
    {
        delete _detail;
    }
    
    bool MidiOut::openPort(unsigned int port)
    {
        return _detail->openPort(port);
    }

    void MidiOut::createVirtualPort(const std::string& _port)
    {
        _detail->midiOut->openVirtualPort(_port);
    }

    void MidiOut::closePort()
    {
        _detail->closePort();
    }

    unsigned int MidiOut::getPort() const
    {
        return _detail->port;
    }

    void MidiOut::sendNoteOn(int channel, int id, int value)
    {
        std::vector<unsigned char> message;
        message.push_back(MIDI_NOTE_ON | channel);
        message.push_back(id);
        message.push_back(value);
        _detail->midiOut->sendMessage(&message);
    }

    void MidiOut::sendNoteOff(int channel, int id, int value)
    {
        std::vector<unsigned char> message;
        message.push_back(MIDI_NOTE_OFF | channel);
        message.push_back(id);
        message.push_back(value);
        _detail->midiOut->sendMessage(&message);
    }

    void MidiOut::sendControlChange(int channel, int id, int value)
    {
        std::vector<unsigned char> message;
        message.push_back(MIDI_CONTROL_CHANGE | channel);
        message.push_back(id);
        message.push_back(value);
        _detail->midiOut->sendMessage(&message);
    }

    void MidiOut::sendProgramChange(int channel, int value)
    {
        std::vector<unsigned char> message;
        message.push_back(MIDI_PROGRAM_CHANGE | channel);
        message.push_back(value);
        _detail->midiOut->sendMessage(&message);
    }

    void MidiOut::sendPitchBend(int channel, int lsb, int msb)
    {
        std::vector<unsigned char> message;
        message.push_back(MIDI_PITCH_BEND | channel);
        message.push_back(lsb);
        message.push_back(msb);
        _detail->midiOut->sendMessage(&message);
    }
    
    void MidiOut::command(const MidiCommand* mc)
    {
        std::vector<unsigned char> message;
        message.push_back(mc->command);
        message.push_back(mc->byte1);
        uint8_t c = mc->command >> 4;
        if ((c != 0xc) && (c != 0xd))
            message.push_back(mc->byte2);
        _detail->midiOut->sendMessage(&message);
    }

} // Lab
