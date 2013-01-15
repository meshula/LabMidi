
project "LabMidi"
    kind "StaticLib"  -- ConsoleApp, SharedLib are alternates
    language "C++"    
    platforms { "x32", "x64" }
    
    includedirs {  } 
    files { "LabMidi/**.h", "LabMidi/**.cpp", "LabMidi/**.c" }        
    excludes { "LabMidi/rtmidi-1.0.15/tests/**" }

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
