
//  Copyright (c) 2012, Nick Porcino
//  All rights reserved.
//  SPDX-License-Identifier: BSD-2-Clause

#include "MidiPlayerApp.h"
#include "OptionParser.h"

#include <LabMidi/LabMidi.h>

#include <iostream>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#endif



class MidiPlayerApp::Detail
{
public:
    Lab::MidiPorts* midiPorts = nullptr;
    Lab::MidiSongPlayer* midiSongPlayer = nullptr;
    double startTime = 0;

    Detail()
    : midiPorts(new Lab::MidiPorts())
    , midiSongPlayer(0)
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
            std::cout << "No MIDI input ports found\n\n";
        else {
            std::cout << "MIDI input ports:" << std::endl;
            for (int i = 0; i < c; ++i)
                std::cout << "   " << i << ": " << midiPorts->inPort(i) << std::endl;
            std::cout << std::endl;
        }

        c = midiPorts->outPorts();
        if (c == 0)
            std::cout << "No MIDI output ports found\n\n";
        else {
            std::cout << "MIDI output ports:" << std::endl;
            for (int i = 0; i < c; ++i)
                std::cout << "   " << i << ": " << midiPorts->outPort(i) << std::endl;
            std::cout << std::endl;
        }
    }
};

MidiPlayerApp::MidiPlayerApp
()
: _detail(new Detail())
{
    _detail->listPorts();
}

MidiPlayerApp::~MidiPlayerApp
()
{
    delete _detail;
}

double MidiPlayerApp::getElapsedSeconds()
{
#ifdef _MSC_VER
    static double freq;
    static LARGE_INTEGER start;
    static bool init = true;
    if (init) {
        LARGE_INTEGER lFreq;
        QueryPerformanceFrequency(&lFreq);
        freq = double(lFreq.QuadPart);
        QueryPerformanceCounter(&start);
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

void MidiPlayerApp::setup()
{
}

void MidiPlayerApp::update(double t)
{
    _detail->midiSongPlayer->update(static_cast<float>(t));
}

bool MidiPlayerApp::running(double t)
{
    if (!_detail->midiSongPlayer)
        return false;

    return t <= (_detail->midiSongPlayer->length() + 0.5f);
}

int main(int argc, char** argv)
{
    MidiPlayerApp app;

    OptionParser op("MidiPlayer");
    int port = -1;
    std::string path;
    op.AddIntOption("o", "outport", port, "A valid MIDI output port number");
    op.AddStringOption("f", "file", path, "Path to the file to play");
    if (op.Parse(argc, argv)) {
        if (port == -1 || !path.length())
            op.Usage();
        else {
            Lab::MidiOut* midiOut = new Lab::MidiOut();
            midiOut->openPort(port);
            Lab::MidiSong* midiSong = new Lab::MidiSong();
            midiSong->parse(path.c_str(), true);
            app._detail->midiSongPlayer = new Lab::MidiSongPlayer(midiSong);
            app._detail->midiSongPlayer->addCallback(Lab::MidiOut::playerCallback, midiOut);
            app._detail->midiSongPlayer->play(0);

            double startTime = app.getElapsedSeconds();

            while (app.running(app.getElapsedSeconds() - startTime)) {
                #ifdef _MSC_VER
                Sleep(1); // 1ms delay --- to do - shouldn't sleep this long
                #else
                usleep(100); // 0.1ms delay
                #endif
                app.update(app.getElapsedSeconds() - startTime);
            }
            delete midiOut;
        }
    }

    return 1;
}

