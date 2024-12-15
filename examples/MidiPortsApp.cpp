/*
 *  MidiApp.cpp
 *
 */

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

// Note that the sample midi files were obtained from the jasmid
// distribution on github, and they contain their own internal
// copyright notices.

#include "MidiPortsApp.h"
#include "LabMidi/Ports.h"

#include <iostream>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#endif

class MidiPortsApp::Detail
{
public:
    Detail()
    : midiPorts(new Lab::MidiPorts())
    {
    }

    ~Detail()
    {
        delete midiPorts;
    }

    void listPorts()
    {
        midiPorts->refreshPortList();
        int c = midiPorts->inPorts();
        if (c == 0)
            std::cout << "No MIDI input ports found" << std::endl;
        else {
            std::cout << "MIDI input ports:" << std::endl;
            for (int i = 0; i < c; ++i)
                std::cout << "   " << i << ": " << midiPorts->inPort(i) << std::endl;
            std::cout << std::endl;
        }

        c = midiPorts->outPorts();
        if (c == 0)
            std::cout << "No MIDI output ports found" << std::endl;
        else {
            std::cout << "MIDI output ports:" << std::endl;
            for (int i = 0; i < c; ++i)
                std::cout << "   " << i << ": " << midiPorts->outPort(i) << std::endl;
            std::cout << std::endl;
        }
    }

    Lab::MidiPorts* midiPorts;
    double startTime;
};

MidiPortsApp::MidiPortsApp()
: _detail(new Detail())
{
    _detail->listPorts();
}

MidiPortsApp::~MidiPortsApp()
{
    delete _detail;
}

double MidiPortsApp::getElapsedSeconds()
{
#ifdef _MSC_VER
    static double freq;
    static LARGE_INTEGER start;
    static bool init = true;
    if (init) {
        LARGE_INTEGER lFreq;
        QueryPerformanceFrequency(&lFreq);
        freq = double(lFreq.QuadPart);
        QueryPerformanceCounter(&lStart);
        init = false;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return double(now.QuadPart - start.QuadPart) / freq;
#else
    timeval t;
    gettimeofday(&t, 0);
    return double(t.tv_sec) + double(t.tv_usec) * 1.0e-6f;
#endif
}

void MidiPortsApp::setup()
{
}

void MidiPortsApp::update()
{
}

bool MidiPortsApp::running()
{
    return false;
}

int main(int argc, char** argv)
{
    MidiPortsApp* app = new MidiPortsApp();

    while (app->running()) {
#ifdef _MSC_VER
        Sleep(1); // 1ms delay --- to do - shouldn't sleep this long
#else
        usleep(100); // 0.1ms delay
#endif
        app->update();
    }

    delete app;
    return 1;
}

