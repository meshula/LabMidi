//
//  LabMidiSoftSynth.cpp
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

#include "LabMidiSoftSynth.h"
#include "LabMidiCommand.h"

#if defined(__MACOSX_CORE__)

// The CoreMIDI API is based on the use of a callback function for
// MIDI input.  We convert the system specific time stamps to delta
// time values.

// OS-X CoreMIDI header files.
#include <AudioToolbox/AudioToolbox.h> // for AUGraph
#include <CoreMIDI/CoreMIDI.h>
#include <CoreAudio/HostTime.h>
#include <CoreServices/CoreServices.h>

#endif

namespace Lab {
    
    #if defined(__MACOSX_CORE__)

	// some MIDI constants:
	enum {
		kMidiMessage_ControlChange 		= 0xB,
		kMidiMessage_ProgramChange 		= 0xC,
		kMidiMessage_BankMSBControl 	= 0,
		kMidiMessage_BankLSBControl		= 32,
		kMidiMessage_NoteOn 			= 0x9
	};
    
    class MidiSoftSynth::Detail
    {
    public:
        Detail()
        : auGraph(0)
        , limiterNode(0)
        , outNode(0)
        , synthUnit(0)
        , midiChannelInUse(0xff)
        {
        }
        
        ~Detail()
        {
            shutdown();
        }

        // On OS X there are known places were sample banks can be stored
        // Library/Audio/Sounds/Banks - so you could scan this directory and give the user options
        // about which sample bank to use...
        // passing in zero will result in the default bank being used

        void start(int midiChannel, char const*const bankPath)
        {
            if (auGraph != 0)
                return;		// don't multiply init
            
            midiChannelInUse = midiChannel;
            
            OSStatus result;
            
            //create the nodes of the graph
            AUNode synthNode;
            
            AudioComponentDescription cd;
            cd.componentManufacturer = kAudioUnitManufacturer_Apple;
            cd.componentFlags = 0;
            cd.componentFlagsMask = 0;
            
            require_noerr (result = NewAUGraph (&auGraph), home);
            
            cd.componentType = kAudioUnitType_MusicDevice;
            cd.componentSubType = kAudioUnitSubType_DLSSynth;
            
            require_noerr (result = AUGraphAddNode (auGraph, &cd, &synthNode), home);
            
            cd.componentType = kAudioUnitType_Effect;
            cd.componentSubType = kAudioUnitSubType_PeakLimiter;
            
            require_noerr (result = AUGraphAddNode (auGraph, &cd, &limiterNode), home);
            
            cd.componentType = kAudioUnitType_Output;
            cd.componentSubType = kAudioUnitSubType_DefaultOutput;
            require_noerr (result = AUGraphAddNode (auGraph, &cd, &outNode), home);
            
            require_noerr (result = AUGraphOpen (auGraph), home);
            
            require_noerr (result = AUGraphConnectNodeInput(auGraph, synthNode, 0, limiterNode, 0), home);
            require_noerr (result = AUGraphConnectNodeInput(auGraph, limiterNode, 0, outNode, 0), home);
            
            // ok we're good to go - get the Synth Unit...
            require_noerr (result = AUGraphNodeInfo(auGraph, synthNode, 0, &synthUnit), home);
            
            // if the user supplies a sound bank, we'll set that before we initialize and start playing
            if (bankPath)
            {
                FSRef fsRef;
                require_noerr (result = FSPathMakeRef ((const UInt8*)bankPath, &fsRef, 0), home);
                
                printf ("Setting Sound Bank:%s\n", bankPath);
                
                require_noerr (result = AudioUnitSetProperty(synthUnit,
                                                             kMusicDeviceProperty_SoundBankFSRef,
                                                             kAudioUnitScope_Global, 0,
                                                             &fsRef, sizeof(fsRef)), home);
                
            }
            
            // ok we're set up to go - initialize and start the graph
            require_noerr (result = AUGraphInitialize (auGraph), home);
            
            //set our bank
            require_noerr (result = MusicDeviceMIDIEvent(synthUnit,
                                                         kMidiMessage_ControlChange << 4 | midiChannelInUse,
                                                         kMidiMessage_BankMSBControl, 0,
                                                         0/*sample offset*/), home);
            
            require_noerr (result = MusicDeviceMIDIEvent(synthUnit,
                                                         kMidiMessage_ProgramChange << 4 | midiChannelInUse,
                                                         0/*prog change num*/, 0,
                                                         0/*sample offset*/), home);
            
            CAShow(auGraph); // prints out the graph so we can see what it looks like...
            
            require_noerr (result = AUGraphStart(auGraph), home);
            return;
            
        home:
            shutdown();
        }
        
        void shutdown()
        {
            if (auGraph) {
                AUGraphStop (auGraph); // stop playback - AUGraphDispose will do that for us but just showing you what to do
                DisposeAUGraph (auGraph);
                auGraph = 0;
            }
        }
        
        void command(const MidiCommand* c)
        {
            OSStatus result;
            require_noerr(result = MusicDeviceMIDIEvent(synthUnit, c->command, c->byte1, c->byte2, 0), home);
            return;
        home: ;
        }

        
        AUGraph auGraph;
        AUNode  limiterNode;
        AUNode	outNode;
        AudioUnit synthUnit;
        UInt8 midiChannelInUse;
    };
    
#elif defined (_MSC_VER)

    class MidiSoftSynth::Detail
    {
    public:
        void start(int midiChannel, char const*const bankPath) { }
        void command(MidiCommand*) { }
    };
    
#else
    
    class MidiSoftSynth::Detail
    {
    public:
        void start(int midiChannel, char const*const bankPath) { }
        void command(MidiCommand*) { }
    };
    
#endif
    
    MidiSoftSynth::MidiSoftSynth()
    : _detail(new Detail())
    {
    }
    
    MidiSoftSynth::~MidiSoftSynth()
    {
        delete _detail;
    }
    
    void MidiSoftSynth::initialize(int midiChannel, char const*const bankPath)
    {
        _detail->start(midiChannel, bankPath);
    }
    
    void MidiSoftSynth::command(const MidiCommand* c)
    {
        _detail->command(c);
    }

} // Lab

