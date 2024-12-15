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
    passed in as either an MML string, or a file path. The version parsed is a restricted
    form of Modern MML, and not nearly as sophisticated as what mml2mid can currently
    process.

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
CMake 3.15 or greater is required for building. Building in the usual way should work out of the box, as LabMidi
has no external dependencies.

Build Options:
- LABMIDI_BUILD_EXAMPLES (ON/OFF): Build example applications (default: ON)
- LABMIDI_BUILD_SHARED_LIBS (ON/OFF): Build as shared libraries (default: OFF)
- LABMIDI_INSTALL (ON/OFF): Generate installation target (default: ON)

Example:
```cmake
cmake -B build -DLABMIDI_BUILD_EXAMPLES=ON
cmake --build build
```

Usage
-----
LabMidi can be used in other CMake projects either via find_package() or FetchContent:

```cmake
# Option 1: Using find_package
find_package(LabMidi REQUIRED)
target_link_libraries(your_target PRIVATE Lab::Midi)

# Option 2: Using FetchContent
include(FetchContent)
FetchContent_Declare(
    labmidi
    GIT_REPOSITORY your_repo_url
    GIT_TAG your_tag
)
FetchContent_MakeAvailable(labmidi)
target_link_libraries(your_target PRIVATE Lab::Midi)
```

See the MidiApp source for examples of usage. Note that MidiApp.cpp currently has hard coded paths to the midi sample files.
You'll need to make sure you've set your working directory to the folder containing the resources folder. In XCode, under the Product menu, select Edit Schemes..., then the Options tab, then the target you want to run, and click the Use Custom Working Directory box. Fill in the path appropriately.

MidiPlayer
----------

MidiPlayerApp -o 0 -f path/to/file.midi

Will play file.midi on the 0th port.

License
-------
BSD 3-clause. <http://opensource.org/licenses/BSD-3-Clause>

Note that the sample midi files in the assets directory were obtained from the jasmid
distribution on github, and they contain their own internal copyright notices.

Thanks
------
Thanks to ofxMidi <https://github.com/chrisoshea/ofxMidi> for inspiration, and jasmid <https://github.com/gasman/jasmid> for a clean implementation of a standard midi file reader, which I
used as a jumping off point for the LabMidi midi file parser. Thanks to RtMidi <http://www.music.mcgill.ca/~gary/rtmidi/> for providing a robust platform abstraction.
LabMidi is quite different from jasmid and ofxMidi because the focus is playing midi files, and providing basic routing functionality.

Thanks to qiao for posting lots of base64 encoded MIDI tracks at <https://github.com/qiao/euphony>.

Thanks to <http://www.manythings.org/music/pianotheory/> for posting a very cool web utility that calculates scales and chords.
It's dual licensed GPL and CC-0. I used the tables in that utility, choosing the CC-0 license for this usage.

There's 9,310 piano MIDI files here: <http://www.kuhmann.com/Yamaha.htm>

Thanks to arle <http://www17.atpages.jp/~arle/index.php?%E3%83%8D%E3%82%BF> for publishing mml2mid <http://hpc.jp/~mml2mid/>,
and to g200kg <http://www.g200kg.com/en/docs/webmodular/> for an MML player.

Thanks to Andre Mazzone and Josh Fillstrup for helping get LabMidi up and running on Linux.
