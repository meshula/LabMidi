
#include "OptionParser.h"
#include <LabMidi/LabMidi.h>
#include <iostream>
#include <memory>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific includes
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

// Function to get a single character from the terminal
#ifdef _WIN32
int getch() {
    return _getch();
}

void sleep(int ms) {
    Sleep(ms);
}
#else
int getch() {
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

void sleep(int ms) {
    usleep(ms * 1000);
}
#endif

// Function to check if a key has been pressed
int kbhit() {
#ifdef _WIN32
    return _kbhit();
#else
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch != EOF;
#endif
}

/* APC Mini Mk2
 
 MIDI input ports:
    0: APC mini mk2 Control
    1: APC mini mk2 Notes

 All signals come from port 0

 MIDI output ports:
    0: APC mini mk2 Control
    1: APC mini mk2 Notes
 
 
grid MIDI map

Row / Column	1	2	3	4	5	6	7	8
                1	38	39	3a	3b	3c	3d	3e	3f
                2	30	31	32	33	34	35	36	37
                3	28	29	2a	2b	2c	2d	2e	2f
                4	20	21	22	23	24	25	26	27
                5	18	19	1a	1b	1c	1d	1e	1f
                6	10	11	12	13	14	15	16	17
                7	08	09	0a	0b	0c	0d	0e	0f
                8	00	01	02	03	04	05	06	07

7f is the only on velocity, 00 is the only off velocity
they may be illuminated with note on/off messages
90 key 0 off
90 key 1 green on
90 key 2 green blink
90 key 3 red on
90 key 4 red blink
90 key 5 yellow on
90 key 6 yellow blink



horizontal buttons run from 64 to 6b ~ red
vertical buttons run from 70 to 7a ~ green
these buttons may be illuminated with note on/off messages
90 key 0 off
90 key 1 on
90 key 2 blink

button 7a, the shift button, has no illumination

sliders return values from 0-7f
sliders "note" runs from 30-38

*/

void listPorts(Lab::MidiPorts* midiPorts) {
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

void* printHex(uint8_t byte, char* a, char* b) {
    uint8_t up = byte >> 4;
    uint8_t dn = byte & 0xf;
    *a = up > 9 ? ('A' + up - 10) : ('0' + up);
    *b = dn > 9 ? ('A' + dn - 10) : ('0' + dn);
}

typedef struct {
    int port;
    int mpcInPort;
    int mpcOutPort;
    std::vector<Lab::MidiIn*> midiIns;
    std::vector<Lab::MidiOut*> midiOuts;
} MIDIcb;

void setGrid(MIDIcb* cbData, const uint8_t* array) {
    if (cbData->midiOuts.size() > 0) {
        for (int i = 0; i < 64; ++i) {
            Lab::MidiCommand c;
            c.command = MIDI_NOTE_ON;
            c.byte1 = i;
            c.byte2 = array[i];
            cbData->midiOuts[0]->command(&c);
        }
    }
}

void midiPrintCallback(void* userData, Lab::MidiCommand* c) {
    if (c) {
        MIDIcb* cb = (MIDIcb*) userData;
        
        char a, b;
        printHex(c->byte1, &a, &b);
        std::cout << "Port " << cb->port << " "
                  << Lab::commandName(c->command) << " "
                  << Lab::noteName(c->byte1) << " (" << a << b << ") ";

        if (c->command <= 0x9f)
            std::cout << "vel: ";

        uint8_t ct = c->command >> 4;
        if ((ct != 0xc) && (ct != 0xd)) {
            printHex(c->byte2, &a, &b);
            std::cout << a << b << "\n";
        }
        else {
            std::cout << "\n";
        }
        
        if (cb->mpcInPort >= 0 && cb->mpcOutPort >= 0) {
            if (c->command == MIDI_NOTE_ON) {
                Lab::MidiCommand o = *c;
                o.command = MIDI_NOTE_ON;
                
                static uint8_t foo = 0;
                char a,b;
                printHex(foo, &a, &b);
                std::cout << "val " << a << " " << b << "\n";
                o.byte2 = foo++;
                cb->midiOuts[cb->mpcOutPort]->command(&o);
            }
        }
    }
}

int main(int argc, char** argv) {
    auto ports = new Lab::MidiPorts();
    MIDIcb cbData;
    cbData.port = -1;
    cbData.mpcInPort = -1;
    cbData.mpcOutPort = -1;
    bool userExit = false;
    do {
        if (!ports)
            break;

        listPorts(ports);
        int inPortCount = ports->inPorts();
        if (!inPortCount)
            break;
        
        for (int i = 0; i < ports->inPorts(); ++i)
            if (ports->inPort(i) == "APC mini mk2 Control")
                cbData.mpcInPort = i;

        if (cbData.mpcInPort >=0)
            std::cout << "Found an APC mini mk2 at input port " << cbData.mpcInPort << "\n";

        for (int i = 0; i < ports->outPorts(); ++i)
            if (ports->outPort(i) == "APC mini mk2 Control")
                cbData.mpcOutPort = i;

        if (cbData.mpcOutPort >=0)
            std::cout << "Found an APC mini mk2 at output port " << cbData.mpcOutPort << "\n";
        
        for (int i = 0; i < inPortCount; ++i) {
            auto midiIn = new Lab::MidiIn();
            if (!midiIn)
                break;
            if (!midiIn->openPort(i)) {
                std::cerr << "Unable to open input port " << i << std::endl;
                break;
            }
            midiIn->addCallback(midiPrintCallback, (void*) &cbData);
            cbData.midiIns.push_back(midiIn);
        }
        if (!cbData.midiIns.size())
            break;

        std::cout << "Opened " << cbData.midiIns.size() << " MIDI in ports\n";

        int outPortCount = ports->outPorts();
        for (int i = 0; i < outPortCount; ++i) {
            auto midiOut = new Lab::MidiOut();
            if (!midiOut)
                break;
            if (!midiOut->openPort(i)) {
                std::cerr << "Unable to open output port " << i << std::endl;
                break;
            }
            cbData.midiOuts.push_back(midiOut);
        }
        std::cout << "Opened " << cbData.midiOuts.size() << " MIDI out ports\n";

        /* light up the grid, then wipe it clean */
        std::vector<uint8_t> grid(64, 0);
        for (int i = 0; i < 64; ++i)
            grid[i] = i;
        setGrid(&cbData, grid.data());
        sleep(1000);
        
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                if (i > 0) {
                    grid[(i-1)* 8 + j] = 0;
                }
                grid[i * 8 + j] = 3;
            }
            setGrid(&cbData, grid.data());
            sleep(40);
        }
        for (int j = 0; j < 8; ++j) {
            grid[56 + j] = 0;
        }
        setGrid(&cbData, grid.data());

        
        const int sleepTimeMs = 10;
        int key;
        while (1) {
            sleep(sleepTimeMs);

            // Check if a key has been pressed
            if (kbhit()) {
                // If a key is pressed, retrieve and display the key code
                key = getch();
                printf("Key pressed: %c (ASCII code: %d)\n", key, key);
                userExit = true;
                break; // Exit the loop when a key is pressed
            }
            // If no key is pressed, continue sleeping and checking
        }
    }
    while (false);
    
    if (!userExit) {
        std::cerr << "exit due to error" << std::endl;
    }
    
    for (auto midiIn : cbData.midiIns)
        delete midiIn;
    delete ports;
    return EXIT_SUCCESS;
}
