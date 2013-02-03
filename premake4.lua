
-- awesome premake docs:
-- https://github.com/0ad/0ad/blob/master/build/premake/premake4.lua
-- to generate an Xcode project, type premake4 xcode4 (requires latest premake beta)

solution "MidiApp"
    configurations { "Debug", "Release" }
    
    include "src"
    
    project "LabMidiDemo"
        kind "WindowedApp"  -- ConsoleApp, SharedLib are alternates
        language "C++"
        platforms { "x32", "x64" }
        
        includedirs { "src" }
        
        files { "src/MidiApp.h", "src/MidiApp.cpp",
                "osx/Info.plist", "Resources/**" }
                
        links {
              "AudioToolbox.framework",
              "AudioUnit.framework",
              "Carbon.framework",
              "Cocoa.framework",
              "CoreAudio.framework",
              "CoreFoundation.framework",
              "CoreMIDI.framework",
              "LabMidi"
              }
        
        libdirs { }
            
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "DEBUG", "__MACOSX_CORE__", "OSX" }
            flags { "Symbols" }
    
        configuration "Release"
            targetdir "build/Release"
            defines { "NDEBUG", "__MACOSX_CORE__", "OSX" }
            flags { "Optimize" } 

        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

    project "LabMidiPorts"
        kind "ConsoleApp"
        language "C++"
        platforms { "x32", "x64" }
        
        includedirs { "src" }
        
        files { "src/MidiPortsApp.h", "src/MidiPortsApp.cpp",
                "osx/Info.plist", "Resources/**" }
                
        links {
              "AudioToolbox.framework",
              "AudioUnit.framework",
              "Carbon.framework",
              "Cocoa.framework",
              "CoreAudio.framework",
              "CoreFoundation.framework",
              "CoreMIDI.framework",
              "LabMidi"
              }
        
        libdirs { }
            
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "DEBUG", "__MACOSX_CORE__", "OSX" }
            flags { "Symbols" }
    
        configuration "Release"
            targetdir "build/Release"
            defines { "NDEBUG", "__MACOSX_CORE__", "OSX" }
            flags { "Optimize" } 

        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }
