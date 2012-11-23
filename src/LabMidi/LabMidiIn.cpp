
//
//  LabMidiIn.cpp
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

#include "LabMidiIn.h"
#include "LabMidiCommand.h"

#include "RtMidi.h"
#include <iostream>

namespace Lab {
    
    
    class MidiIn::Detail {
    public:
        Detail()
        : verbose(false)
        , port(-1)
        {
            try {
                midiIn = new RtMidiIn();
            }
            catch (RtError& rterr) {
                midiIn = 0;
                rterr.printMessage();
            }
        }
        
        ~Detail()
        {
            closePort();
            delete midiIn;
        }
        
        static void midiInCallback(double deltatime, std::vector<unsigned char> *message, void* userData)
        {
            ((MidiIn::Detail*)userData)->manageNewMessage(deltatime, message);
        }
        
        bool openPort(int port)
        {
            if (port != -1)
                closePort();
            
            try {
                midiIn->openPort(port);
                midiIn->setCallback(&midiInCallback, this);
                
                // Don't ignore sysex, timing, or active sensing messages.
                midiIn->ignoreTypes(false, false, false);
                port = port;
            }
            catch(RtError& rterr) {
                port = -1;
                rterr.printMessage();
            }
            
            return port != -1;
        }
        
        void closePort()
        {
            midiIn->closePort();
            port = -1;
        }
        
        void createVirtualPort(const std::string& port)
        {
            midiIn->openVirtualPort(port);
        }
        
        // RtMidi compatible callback function
        void manageNewMessage(double deltatime, std::vector<unsigned char>* message)
        {
            unsigned int nBytes = message->size();
            if (verbose) {
                std::cout << "num bytes: " << nBytes;
                for (unsigned int i = 0; i < nBytes; i++)
                    std::cout << " Byte " << i << " = " << (int)message->at(i) << ", ";
                if (nBytes > 0)
                    std::cout << "stamp = " << deltatime << '\n';
            }
            
            if (nBytes > 0) {
                MidiCommand mc;
                mc.command = message->at(0);
                mc.byte1 = 0;
                mc.byte2 = 0;
                
                if (nBytes > 1)
                    mc.byte1 = int(message->at(1));
                if (nBytes > 2)
                    mc.byte2 = int(message->at(2));
                
                for (auto i = callbacks.begin(); i != callbacks.end(); ++i)
                    (*i).second((*i).first, &mc);
            }
        }

        
        RtMidiIn*    midiIn;
        unsigned int port;
        bool         verbose;
        
        std::vector<std::pair<void*, MidiCallbackFn> > callbacks;
    };
    
    MidiIn::MidiIn()
    : _detail(new Detail())
    {
    }

    MidiIn::~MidiIn()
    {
        delete _detail;
    }

    bool MidiIn::openPort(unsigned int port)
    {
        return _detail->openPort(port);
    }

    void MidiIn::createVirtualPort(const std::string& port)
    {
        _detail->createVirtualPort(port);
    }

    unsigned int MidiIn::getPort() const
    {
        return _detail->port;
    }

    void MidiIn::closePort()
    {
        _detail->closePort();
    }
        
    void MidiIn::addCallback(MidiCallbackFn f, void* userData)
    {
        _detail->callbacks.push_back(std::pair<void*, MidiCallbackFn>(userData, f));
    }
    
    void MidiIn::removeCallback(void* userData)
    {
        std::vector<std::pair<void*, MidiCallbackFn> >::iterator i = _detail->callbacks.begin();
        while (i != _detail->callbacks.end()) {
            if (userData == (*i).first) {
                _detail->callbacks.erase(i);
                i = _detail->callbacks.begin();
            }
            else
                ++i;
        }
    }
    
    void MidiIn::setVerbose(bool verbose)
    {
        _detail->verbose = verbose;
    }
    
} // Lab
