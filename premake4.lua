
-- awesome premake docs:
-- https://github.com/0ad/0ad/blob/master/build/premake/premake4.lua
-- to generate an Xcode project, type premake4 xcode4 (requires latest premake beta)

solution "MidiApp"
    configurations { "Debug", "Release" }
    
    project "LabMidiDemo"
        kind "WindowedApp"  -- ConsoleApp, SharedLib are alternates
        language "C++"
        platforms { "x32", "x64" }
        
        links { "LabMidi" }
        
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

    project "LabMidi"
        kind "StaticLib"  -- ConsoleApp, SharedLib are alternates
        language "C++"    
        platforms { "x32", "x64" }
        
        includedirs { "src" } 
        files { "src/**.h", "src/**.cpp", "src/**.c" }        
        excludes { "src/LabMidi/rtmidi-1.0.15/tests/**" }
    
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "DEBUG", "__MACOSX_CORE__", "OSX" }
            flags { "Symbols" }
    
        configuration "Release"
            targetdir "build/Release"
            defines { "NDEBUG", "__MACOSX_CORE__", "OSX" }
            flags { "Optimize" } 
