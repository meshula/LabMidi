LabMidi
=======

Cross platform Midi IN and OUT. Standard midi file parser and player. 

Midi Softsynth implementation for OSX.

Thanks to ofxMidi <https://github.com/chrisoshea/ofxMidi> for inspiration, and jasmid <https://github.com/gasman/jasmid> for a clean implementation of a standard midi file reader, which I
used as a jumping off point for the LabMidi midi file parser. Thanks to RtMidi <http://www.music.mcgill.ca/~gary/rtmidi/> for providing a robust platform abstraction.

LabMidi is quite different from either of those because the focus is playing midi files, and providing basing routing functionality.

Like ofxMidi, I'm thinking of using PGMidi <https://github.com/petegoodliffe/PGMidi> to implement the API on iOS.

See the MidiApp source for examples of usage. Note that MidiApp.cpp currently has hard coded paths to the midi sample files. 
You'll need to make sure you've set your working directory to the folder containing the resources folder. In XCode, under the Product menu, select Edit Schemes..., then the Options tab, then the target you want to run, and click the Use Custom Working Directory box. Fill in the path appropriately.

Tested on OSX. Implementation for Windows and Linux in place, but not tested. premake file needs to be set up for multiple platforms. Pull requests welcome.
