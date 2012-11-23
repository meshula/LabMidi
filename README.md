LabMidi
=======

Cross platform Midi IN and OUT. Standard midi file parser and player. 

Midi Softsynth implementation for OSX.

Thanks to ofxMidi <https://github.com/chrisoshea/ofxMidi> for inspiration, and jasmid <https://github.com/gasman/jasmid> for a clean implementation of a standard midi file reader, which I
used as a jumping off point for the LabMidi midi file parser.

LabMidi is quite different from either of those because the focus is playing midi files, and providing basing routing functionality.

Like ofxMidi, I'm thinking of using PGMidi <https://github.com/petegoodliffe/PGMidi> to implement the API on iOS.

See the MidiApp source for examples of usage. Note that MidiApp.cpp currently has hard coded paths to the midi sample files. You'll probably need to adjust those.

Tested on OSX. Implementation for Windows and Linux in place, but not tested. premake file needs to be set up for multiple platforms. Pull requests welcome.
