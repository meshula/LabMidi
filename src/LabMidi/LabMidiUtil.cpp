
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

#include "LabMidiUtil.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

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
        "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",
        "C0",  "C#0",  "D0",  "D#0",  "E0",  "F0",  "F#0",  "G0",  "G#0",  "A0",  "A#0",  "B0",
        "C1",  "C#1",  "D1",  "D#1",  "E1",  "F1",  "F#1",  "G1",  "G#1",  "A1",  "A#1",  "B1",
        "C2",  "C#2",  "D2",  "D#2",  "E2",  "F2",  "F#2",  "G2",  "G#2",  "A2",  "A#2",  "B2",
        "C3",  "C#3",  "D3",  "D#3",  "E3",  "F3",  "F#3",  "G3",  "G#3",  "A3",  "A#3",  "B3",
        "C4",  "C#4",  "D4",  "D#4",  "E4",  "F4",  "F#4",  "G4",  "G#4",  "A4",  "A#4",  "B4",
        "C5",  "C#5",  "D5",  "D#5",  "E5",  "F5",  "F#5",  "G5",  "G#5",  "A5",  "A#5",  "B5",
        "C6",  "C#6",  "D6",  "D#6",  "E6",  "F6",  "F#6",  "G6",  "G#6",  "A6",  "A#6",  "B6",
        "C7",  "C#7",  "D7",  "D#7",  "E7",  "F7",  "F#7",  "G7",  "G#7",  "A7",  "A#7",  "B7",
        "C8",  "C#8",  "D8",  "D#8",  "E8",  "F8",  "F#8",  "G8",  "G#8",  "A8",  "A#8",  "B8",
        "C9",  "C#9",  "D9",  "D#9",  "E9",  "F9",  "F#9",  "G9"
    };
    
    const char* flatNoteNames[128] = {
        "C-1", "Db-1", "D-1", "Eb-1", "E-1", "F-1", "Gb-1", "G-1", "Ab-1", "A-1", "Bb-1", "B-1",
        "C0",  "Db0",  "D0",  "Eb0",  "E0",  "F0",  "Gb0",  "G0",  "Ab0",  "A0",  "Bb0",  "B0",
        "C1",  "Db1",  "D1",  "Eb1",  "E1",  "F1",  "Gb1",  "G1",  "Ab1",  "A1",  "Bb1",  "B1",
        "C2",  "Db2",  "D2",  "Eb2",  "E2",  "F2",  "Gb2",  "G2",  "Ab2",  "A2",  "Bb2",  "B2",
        "C3",  "Db3",  "D3",  "Eb3",  "E3",  "F3",  "Gb3",  "G3",  "Ab3",  "A3",  "Bb3",  "B3",
        "C4",  "Db4",  "D4",  "Eb4",  "E4",  "F4",  "Gb4",  "G4",  "Ab4",  "A4",  "Bb4",  "B4",
        "C5",  "Db5",  "D5",  "Eb5",  "E5",  "F5",  "Gb5",  "G5",  "Ab5",  "A5",  "Bb5",  "B5",
        "C6",  "Db6",  "D6",  "Eb6",  "E6",  "F6",  "Gb6",  "G6",  "Ab6",  "A6",  "Bb6",  "B6",
        "C7",  "Db7",  "D7",  "Eb7",  "E7",  "F7",  "Gb7",  "G7",  "Ab7",  "A7",  "Bb7",  "B7",
        "C8",  "Db8",  "D8",  "Eb8",  "E8",  "F8",  "Gb8",  "G8",  "Ab8",  "A8",  "Bb8",  "B8",
        "C9",  "Db9",  "D9",  "Eb9",  "E9",  "F9",  "Gb9",  "G9"
    };

    const char* noteName(uint8_t note)
    {
        return noteNames[note & 0x7f];
    }
    
    const char* noteName(uint8_t note, uint8_t channel)
    {
        if (channel == 9)  // MIDI channel 10
            return percussionName(note);
        else
            return noteNames[note & 0x7f];
    }

    //                      A   B   C  D  E  F  G
    int baseIndices[7]  = {  9, 11, 0, 2, 4, 5, 7 };
    int flatIndices[7]  = {  8, 10, 0, 1, 3, 5, 6 };
    int sharpIndices[7] = { 10, 11, 1, 3, 4, 6, 8 };
    
    uint8_t noteNameToNoteNum(char const*const name, int& len)
    {
        if (strlen(name) < 2) {
            len = 0;
            return 0xff;
        }
        
        char note = toupper(name[0]);
        char mod = tolower(name[1]);
        bool flat = mod == 'b';
        bool sharp = mod == '#';
        char octaveName = (flat || sharp) ? name[2] : name[1];
        int octave = octaveName - 'A';
        if (octave >= 0 && octave <= 9)
            octave += 1;
        else if (octaveName == '-')
            octave = 0;
        else {
            len = 0;                // malformed octave name
            return 0xff;
        }
        
        len = 2 + ((flat || sharp) ? 1 : 0) + ((octave < 0) ? 1 : 0);
        
        int baseC = (octave + 1) * 12;
        if (flat)
            return baseC + flatIndices[note - 'A'] + 1;
        else if (sharp)
            return baseC + sharpIndices[note - 'A'] - 1;

        return baseC = baseIndices[note - 'A'];
    }
    
    uint8_t noteNameToNoteNum(char const*const name)
    {
        int len;
        return noteNameToNoteNum(name, len);
    }
    
    
    const char* groupNames[16] = {
        "Piano",        "Chromatic Percussion", "Organ",      "Guitar",
        "Bass",         "Strings",              "Ensemble",   "Brass",
        "Reed",         "Pipe",                 "Syth Lead",  "Synth Pad",
        "Syth Effects", "Ethnic",               "Percussive", "Sound Effects"
    };
    
    const char* instrumentGroupName(uint8_t instrument)
    {
        return groupNames[(instrument) >> 4];
    }
    
    const char* instrumentNames[128] = {
        "Acoustic Grand Piano",         // 1
        "Bright Acoustic Piano",
        "Electric Grand Piano",
        "Honky-tonk Piano",
        "Electric Piano 1",
        "Electric Piano 2",
        "Harpsichord",
        "Clavichord",
        
        "Celesta",                      // 9
        "Glockenspiel",
        "Music Box",
        "Vibraphone",
        "Marimba",
        "Xylophone",
        "Tubular Bells",
        "Dulcimer",
        
        "Drawbar Organ",               // 17
        "Percussive Organ",
        "Rock Organ",
        "Church Organ",
        "Reed Organ",
        "Accordion",
        "Harmonica",
        "Tango Accordion",
        
        "Acoustic Guitar (nylon)",     // 25
        "Acoustic Guitar (steel)",
        "Electric Guitar (jazz)",
        "Electric Guitar (clean)",
        "Electric Guitar (muted)",
        "Overdriven Guitar",
        "Distortion Guitar",
        "Guitar harmonics",
        
        "Acoustic Bass",               // 33
        "Electric Bass (finger)",
        "Electric Bass (pick)",
        "Fretless Bass",
        "Slap Bass 1",
        "Slap Bass 2",
        "Synth Bass 1",
        "Synth Bass 2",
        
        "Violin",                     // 41
        "Viola",
        "Cello",
        "Contrabass",
        "Tremolo Strings",
        "Pizzicato Strings",
        "Orchestral Harp",
        "Timpani",
        
        "String Ensemble 1",          // 49
        "String Ensemble 2",
        "Synth Strings 1",
        "Synth Strings 2",
        "Choir Aahs",
        "Voice Oohs",
        "Synth Voice",
        "Orchestra Hit",
        
        "Trumpet",                    // 57
        "Trombone",
        "Tuba",
        "Muted Trumpet",
        "French Horn",
        "Brass Section",
        "Synth Brass 1",
        "Synth Brass 2",
        
        "Soprano Sax",                // 65
        "Alto Sax",
        "Tenor Sax",
        "Baritone Sax",
        "Oboe",
        "English Horn",
        "Bassoon",
        "Clarinet",
        
        "Piccolo",                    // 73
        "Flute",
        "Recorder",
        "Pan Flute",
        "Blown Bottle",
        "Shakuhachi",
        "Whistle",
        "Ocarina",
        
        "Lead 1 (square)",            // 81
        "Lead 2 (sawtooth)",
        "Lead 3 (calliope)",
        "Lead 4 (chiff)",
        "Lead 5 (charang)",
        "Lead 6 (voice)",
        "Lead 7 (fifths)",
        "Lead 8 (bass + lead)",
        
        "Pad 1 (new age)",            // 89
        "Pad 2 (warm)",
        "Pad 3 (polysynth)",
        "Pad 4 (choir)",
        "Pad 5 (bowed)",
        "Pad 6 (metallic)",
        "Pad 7 (halo)",
        "Pad 8 (sweep)",
        
        "FX 1 (rain)",                // 97
        "FX 2 (soundtrack)",
        "FX 3 (crystal)",
        "FX 4 (atmosphere)",
        "FX 5 (brightness)",
        "FX 6 (goblins)",
        "FX 7 (echoes)",
        "FX 8 (sci-fi)",
        
        "Sitar",                      // 105
        "Banjo",
        "Shamisen",
        "Koto",
        "Kalimba",
        "Bag pipe",
        "Fiddle",
        "Shanai",
        
        "Tinkle Bell",                // 113
        "Agogo",
        "Steel Drums",
        "Woodblock",
        "Taiko Drum",
        "Melodic Tom",
        "Synth Drum",
        "Reverse Cymbal",
        
        "Guitar Fret Noise",          // 121
        "Breath Noise",
        "Seashore",
        "Bird Tweet",
        "Telephone Ring",
        "Helicopter",
        "Applause",
        "Gunshot"
    };
    
    const char* instrumentName(uint8_t instrument)
    {
        return instrumentNames[instrument];
    }
    
    const char* percussionNames[47] = {
        "Acoustic Bass Drum",
        "Bass Drum 1",
        "Side Stick",
        "Acoustic Snare",
        "Hand Clap",
        "Electric Snare",
        "Low Floor Tom",
        "Closed Hi-hat",
        "Low Floor Tom",
        "Pedal Hi-hat",
        "Low Tom",
        "Open High Hat",
        "Mid Low Tom",
        "High Mid Tom",
        "Crash Cymbal",
        "High Tom",
        "Ride Cymbal 1",
        "Chinese Cymbal",
        "Ride Bell",
        "Tambourine",
        "Splash Cymbal",
        "Cowbell",
        "Crash Cymbal 2",
        "Vibra Slap",
        "Ride Cymbal 2",
        "High Bongo",
        "Low Bongo",
        "Mute High Conga",
        "Open High Conga",
        "Low Conga",
        "High Timbale",
        "Low Timbale",
        "High Agogo",
        "Low Agogo",
        "Cabasa",
        "Maracas",
        "Short Whistle",
        "Long Whistle",
        "Short Guiro",
        "Long Guiro",
        "Claves",
        "High Wood Block",
        "Low Wood Block",
        "Mute Cuica",
        "Open Cuica",
        "Mute Triangle",
        "Open Triangle"
    };
    
    const char* percussionName(uint8_t channel10noteNumber)
    {
        uint8_t note = channel10noteNumber & 0x7f;
        
        if (note < 35 || note > 81)
            return "Unknown";
        
        return percussionNames[note - 35];
    }
    
    float noteToFrequency(uint8_t note, float A)
    {
        return (A / 32.0f) * powf(2.0f, (note - 9) / 12.0f);
    }

    float noteToFrequency(uint8_t note)
    {
        return noteToFrequency(note, 440.0f);
    }
    
    uint8_t frequencyToNote(float freq, float A)
    {
        float n = freq / (A / 32.0f);
        n = logf(n) / logf(2.0f);
        n *= 12.0f;
        n += 9;
        return (uint8_t) n;
    }

    uint8_t frequencyToNote(float freq)
    {
        return frequencyToNote(freq, 440.0f);
    }

    char const*const bpmToTempoName(int bpm)
    {
        // cf. http://en.wikipedia.org/wiki/Tempo
        if (bpm >= 178) return "Prestissimo";       // 178-208
        if (bpm >= 168) return "Presto";            //168-177
        if (bpm >= 132) return "Vivace";            // 132
        if (bpm >= 105) return "Allegro";           // 105-132
        if (bpm >= 91) return "Allegretto";         // 91-104
        if (bpm >= 81) return "Moderato";           // 81-90
        //if (bpm >= 71) return "Andantino";        // a bit faster than Andante
        if (bpm >= 61) return "Andante";            // 61-80
        //if (bpm >= 55) return "Andante Moderato"; // a bit slower than Andante
        if (bpm >= 51) return "Adagio";             // 51-60
        //if (bpm >= 60) return "Larghetto";        // 40-50
        if (bpm >= 40) return "Largo";              // 40-50
        //if (bpm >= 40) return "Lento";            // 40-60
        if (bpm >= 20) return "Grave";              // 40-20
        return "Larghissimo";                       // 0-20
    }
    
} // Lab
