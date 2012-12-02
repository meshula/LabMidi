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

#include "MidiApp.h"

#include "LabMidiCommand.h"
#include "LabMidiIn.h"
#include "LabMidiOut.h"
#include "LabMidiPorts.h"
#include "LabMidiSoftSynth.h"
#include "LabMidiSong.h"
#include "LabMidiSongPlayer.h"
#include "LabMidiUtil.h"

#include <iostream>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

void midiPrintCallback(void* userData, Lab::MidiCommand* c)
{
    if (c) {
        std::cout << Lab::commandName(c->command) << " "
                  << Lab::noteName(c->byte1) << " ";
        
        if (c->command <= 0x9f)
            std::cout << "vel: ";

        uint8_t ct = c->command >> 4;
        if ((ct != 0xc) && (ct != 0xd))
            std::cout << int(c->byte2) << std::endl;
    }
}


class TestMidi
{
public:
    virtual ~TestMidi() { }
    
    virtual void update(float t) = 0;
    virtual bool running(float t) = 0;
};

/*
    TestInOut sets up the following scenario:
 
    song.mid -> midiOut(0)
    midiIn(1) -> softSynth
 
    If you set up an external program to listen to 0
    and echo through to 1, the softsynth will play the song.mid file
 
 */

class TestInOut : public TestMidi
{
public:
    TestInOut()
    : midiIn(new Lab::MidiIn())
    , midiOut(new Lab::MidiOut())
    , midiSoftSynth(new Lab::MidiSoftSynth())
    {
        midiIn->addCallback(midiPrintCallback, 0);
        midiIn->addCallback(midiSoftSynthPlayCallback, this);
        midiOut->createVirtualPort("MidiApp Demo Out Port");
        midiIn->openPort(0);
        midiSoftSynth->initialize(1, 0);
        
        midiSong = new Lab::MidiSong();
        midiSong->parse("resources/minute_waltz.mid", true);
        midiSongPlayer = new Lab::MidiSongPlayer(midiSong, midiOut);
        midiSongPlayer->play(0);
    }
    
    virtual ~TestInOut()
    {
        delete midiIn;
        delete midiOut;
        delete midiSongPlayer;
        delete midiSong;
        delete midiSoftSynth;
    }
    
    virtual void update(float t)
    {
        midiSongPlayer->update(t);
    }
    
    virtual bool running(float t)
    {
        return t <= (midiSongPlayer->length() + 0.5f);
    }
    
    static void midiSoftSynthPlayCallback(void* userData, Lab::MidiCommand* c)
    {
        if (userData && c) {
            TestInOut* t = (TestInOut*) userData;
            Lab::MidiSoftSynth* s = t->midiSoftSynth;
            s->command(c);
        }
    }
    
    Lab::MidiIn*         midiIn;
    Lab::MidiOut*        midiOut;
    Lab::MidiSoftSynth*  midiSoftSynth;
    Lab::MidiSong*       midiSong;
    Lab::MidiSongPlayer* midiSongPlayer;
};


/*
 TestSoftSynth sets up the following scenario:
 
 song.mid -> softSynth
 
 */

class TestSoftSynth : public TestMidi
{
public:
    TestSoftSynth(char const*const path, bool MML = false)
    : midiSoftSynth(new Lab::MidiSoftSynth())
    , midiSong(0)
    , midiSongPlayer(0)
    {
        midiSoftSynth->initialize(1, 0);
        midiSong = new Lab::MidiSong();
        if (MML)
            midiSong->parseMML(path, strlen(path), true);
        else
            midiSong->parse(path, true);
        
        midiSongPlayer = new Lab::MidiSongPlayer(midiSong, midiSoftSynth);
        midiSongPlayer->play(0);
    }
    
    virtual ~TestSoftSynth()
    {
        delete midiSongPlayer;
        delete midiSong;
        delete midiSoftSynth;
    }
    
    virtual void update(float t)
    {
        if (midiSongPlayer)
            midiSongPlayer->update(t);
    }
    
    virtual bool running(float t)
    {
        if (!midiSongPlayer)
            return false;
        
        return t <= (midiSongPlayer->length() + 0.5f);
    }
    
    Lab::MidiSoftSynth*  midiSoftSynth;
    Lab::MidiSong*       midiSong;
    Lab::MidiSongPlayer* midiSongPlayer;
};

class TestFrequencyCalc : public TestMidi
{
public:
    TestFrequencyCalc()
    {
        std::cout << "Compare to the table on http://soundprogramming.net/file_formats/midi_note_frequencies" << std::endl;
        std::cout << "--------------------------------------------------------------------------------------" << std::endl;
        for (uint8_t i = 0; i <= 127; ++i) {
            float f = Lab::noteToFrequency(i);
            int fi = Lab::frequencyToNote(f);
            std::cout << (int) i << " (" << fi << ") " << Lab::noteName(i)
                      << " " << f
                      << std::endl;
        }
    }
    
    virtual void update(float) { }
    virtual bool running(float) { return false; }
};


class MidiApp::Detail
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
    
    TestMidi* testMidi;
};

// sample MML from http://www.g200kg.com/en/docs/webmodular/

namespace {
    const char* MMLtune = "t150 e-d-<g-4>g-rg-4e-d-<g-4>g-rg-4e-d-<g-4>g-4<e-4>g-4<d-4>frf4e-d-<d-4>frf4e-d-<d-4>frf4e-d-<d-4>f4<e-4>f4<g-4>g-rg-4";
}

MidiApp::MidiApp()
: _detail(new Detail())
{
#define TEST 2
    switch (TEST) {
        case 0: _detail->testMidi = new TestSoftSynth("resources/rachmaninov3.mid"); break;
        case 1: _detail->testMidi = new TestSoftSynth("resources/209-Tchaikovsky - Russian Dance (Nutcracker)"); break;
        case 2: _detail->testMidi = new TestSoftSynth(MMLtune, true); break;
        case 3: _detail->testMidi = new TestInOut(); break;
        case 4: _detail->testMidi = new TestFrequencyCalc(); break;
    }
    
    _detail->startTime = getElapsedSeconds();
    _detail->listPorts();
}

MidiApp::~MidiApp()
{
    delete _detail->testMidi;
    delete _detail;
}

double MidiApp::getElapsedSeconds()
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

void MidiApp::setup()
{
}

void MidiApp::update()
{
    _detail->testMidi->update(getElapsedSeconds() - _detail->startTime);
}

bool MidiApp::running()
{
    return _detail->testMidi->running(getElapsedSeconds() - _detail->startTime);
}

int main(int argc, char** argv)
{
    MidiApp* app = new MidiApp();
    
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

