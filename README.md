LabMidi
=======

Cross platform MIDI related utilities.

    class MidiIn
    Can read from a MIDI input port, typically provided by a keyboard
    
    class MidiOut
    Outputs to a MIDI port, typically consumed by a synthesizer
    
    class MidiSoftSynth : public MidiOutBase
    A software synthesizer that can be used as a MIDI output. Currently
    only implemented for OSX (and presumably it would work for iOS as well).
    
    class MidiSong
    A song is a collection of tracks, which are a list of MIDI events. A song can
    be parsed from a Standard MIDI file, or from a base64 encoded Standard MIDI file
    with an appropriate header. MML (Music Macro Language) data may also be parsed,
    passed in as either an MML string, or a file path.
    
    class MidiSongPlayer
    Can play a single MidiSong. The class is initialized with a pointer
    to a MidiSong. The data in the MidiSong is not retained in any way,
    so after a MidiSongPlayer is instantiated it is fine to discard the
    MidiSong object.
    
    LabMidiUtil.h
    Contains various routines to convert between note names, note numbers,
    and frequency, as well as routines to fetch standard General MIDI names
    for instruments.
    
Like ofxMidi, I'm thinking of using PGMidi <https://github.com/petegoodliffe/PGMidi> to implement the MidiIn and MidiOUt classes on iOS.

Building
--------
Premake 4 is required to generate project and solution files.
On OSX, type 
    premake4 xcode4
in the root folder to generate xcode workspace and project files.

Usage
-----
See the MidiApp source for examples of usage. 

Simply include the sources under src/LabMidi in your own project to use as a library.

Note that MidiApp.cpp currently has hard coded paths to the midi sample files. 
You'll need to make sure you've set your working directory to the folder containing the resources folder. In XCode, under the Product menu, select Edit Schemes..., then the Options tab, then the target you want to run, and click the Use Custom Working Directory box. Fill in the path appropriately.

Tested on OSX. Implementation for Windows and Linux in place, but not tested. 

premake file needs to be set up for multiple platforms. 

Pull requests welcome.

License
-------
BSD 3-clause. <http://opensource.org/licenses/BSD-3-Clause>


Thanks
------
Thanks to ofxMidi <https://github.com/chrisoshea/ofxMidi> for inspiration, and jasmid <https://github.com/gasman/jasmid> for a clean implementation of a standard midi file reader, which I
used as a jumping off point for the LabMidi midi file parser. Thanks to RtMidi <http://www.music.mcgill.ca/~gary/rtmidi/> for providing a robust platform abstraction.
LabMidi is quite different from jasmid and ofxMidi because the focus is playing midi files, and providing basic routing functionality.

Thanks to qiao for posting lots of base64 encoded MIDI tracks at <https://github.com/qiao/euphony>.

Thanks to <http://www.manythings.org/music/pianotheory/> for posting a very cool web utility that calculates scales and chords.
It's dual licensed GPL and CC-0. I used the tables in that utility, choosing the CC-0 license for this usage.

There's 9,310 piano MIDI files here: <http://www.kuhmann.com/Yamaha.htm>

Thanks to arle for publishing mml2mid (although where have the sources gone, the pages are down!), and to g200kg <http://www.g200kg.com/en/docs/webmodular/> for
an MML player.
