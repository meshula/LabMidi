
-- awesome premake docs:
-- https://github.com/0ad/0ad/blob/master/build/premake/premake4.lua

-- to generate an Xcode project, type premake4 xcode4 (requires latest premake beta)

-- A solution contains projects, and defines the available configurations
solution "MidiApp"
   configurations { "Debug", "Release" }

   -- location is commented out because info.plist needs to be relative to xcodeproj
   -- but premake doesn't know where to find it if it is called ../osx/Info.plist
   -- location "build"
 
   -- A project defines one build target
   project "MidiApp"
      kind "WindowedApp"  -- ConsoleApp, SharedLib are alternates
      language "C++"

      -- location is commented out because info.plist needs to be relative to xcodeproj
      -- but premake doesn't know where to find it if it is called ../osx/Info.plist
      -- location "build"

      platforms { "x32", "x64" }
      
      includedirs { "src" }
      
      files { "**.h",     -- ** means recurse
              "**.cpp",
              "**.c",
              "osx/Info.plist",
              "Resources/**" }
              
      excludes { "src/LabMidi/rtmidi-1.0.15/tests/**"
               }

      links { -- "Accelerate.framework",
              "AudioToolbox.framework",
              "AudioUnit.framework",
              "Carbon.framework",
              "Cocoa.framework",
              "CoreAudio.framework",
              "CoreFoundation.framework",
              "CoreMIDI.framework",
              -- "CoreVideo.framework",
              -- "OpenGL.framework",
              -- "QTKit.framework",
              -- "QuickTime.framework"
              --  os.findlib("X11") placeholder how to find system library
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
