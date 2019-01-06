
//
//  LabMidiUtil.cpp
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

#include "LabMidiPorts.h"
#include "RtMidi.h"

namespace Lab {
    
    class MidiPorts::Detail
    {
    public:
        Detail()
        : inProbe(new RtMidiIn())
        , outProbe(new RtMidiOut())
        {
            refreshPortList();
        }
        
        ~Detail()
        {
            delete inProbe;
            delete outProbe;
        }
        
        void refreshPortList()
        {
            inPorts = inProbe->getPortCount();
            inPortNames.clear();
            for (unsigned int i = 0; i < inPorts; ++i)
                inPortNames.push_back(inProbe->getPortName(i));
            
            outPorts = outProbe->getPortCount();
            outPortNames.clear();
            for (unsigned int i = 0; i < outPorts; ++i)
                outPortNames.push_back(outProbe->getPortName(i));
        }
        
        RtMidiIn* inProbe;
        RtMidiOut* outProbe;
        unsigned int inPorts;
        unsigned int outPorts;
        std::vector<std::string> inPortNames;
        std::vector<std::string> outPortNames;
    };
    
    MidiPorts::MidiPorts()
    : _detail(new Detail())
    {
    }
    
    MidiPorts::~MidiPorts()
    {
        delete _detail;
    }
    
    void MidiPorts::refreshPortList()
    {
        _detail->refreshPortList();
    }

    unsigned int MidiPorts::inPorts() const
    {
        return _detail->inPorts;
    }
    
    unsigned int MidiPorts::outPorts() const
    {
        return _detail->outPorts;
    }

    const std::string& MidiPorts::inPort(int i) const
    {
        static std::string emptyString;
        if (i < 0 || i >= _detail->inPortNames.size())
            return emptyString;
        return _detail->inPortNames[i];
    }
    
    const std::string& MidiPorts::outPort(int i) const
    {
        static std::string emptyString;
        if (i < 0 || i >= _detail->outPortNames.size())
            return emptyString;
        return _detail->outPortNames[i];
    }
    
    char const*const MidiPorts::inPortCStr(int i) const
    {
        static const char emptyString[1] = {'\0'};
        if (i < 0 || i >= _detail->inPortNames.size())
            return emptyString;
        return _detail->inPortNames[i].c_str();
    }
    
    char const*const MidiPorts::outPortCStr(int i) const
    {
        static const char emptyString[1] = {'\0'};
        if (i < 0 || i >= _detail->outPortNames.size())
            return emptyString;
        return _detail->outPortNames[i].c_str();
    }
    
    
} // Lab
