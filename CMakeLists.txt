cmake_minimum_required(VERSION 3.15)

project(LabMidi
    VERSION 1.0.0
    DESCRIPTION "MIDI library for C++"
    LANGUAGES CXX)

# set c++ to 17
set(CMAKE_CXX_STANDARD 17)

set(LABMIDI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})


option(LABMIDI_USE_RTMIDI6 "Enable RtMidi 6 support" ON)

if(LABMIDI_USE_RTMIDI6)
    find_package(RtMidi 6.0.0 QUIET)

    if (NOT RtMidi_FOUND)
        message(STATUS "Using FetchContent to manage RtMidi")
        include(FetchContent)

        FetchContent_Declare(
            rtmidi
            GIT_REPOSITORY https://github.com/thestk/rtmidi.git
            GIT_TAG 6.0.0
        )

        set(BUILD_SHARED_LIBS_OLD "${BUILD_SHARED_LIBS}")
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(rtmidi)
        set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_OLD}")
    endif()

    if(NOT TARGET rtmidi)
        message(FATAL_ERROR "RtMidi not found")
    endif()
endif()

# Sources and Headers
set(LABMIDI_HEADERS
    include/LabMidi/LabMidi.h
    include/LabMidi/MidiFile.h
    include/LabMidi/MidiFilePlayer.h
    include/LabMidi/MidiInOut.h
    include/LabMidi/MusicTheory.h
    include/LabMidi/Ports.h
    include/LabMidi/SoftSynth.h
    include/LabMidi/Util.h
)

set(LABMIDI_SOURCES
    src/LabMidiIn.cpp
    src/LabMidiMusicTheory.cpp
    src/LabMidiOut.cpp
    src/LabMidiPorts.cpp
    src/LabMidiSoftSynth.cpp
    src/LabMidiSong.cpp
    src/LabMidiSongPlayer.cpp
    src/LabMidiUtil.cpp
)

# Build the library
add_library(LabMidi ${LABMIDI_SOURCES} ${LABMIDI_HEADERS})
target_include_directories(LabMidi PUBLIC include)
target_link_libraries(LabMidi PRIVATE rtmidi)
add_library(Lab::Midi ALIAS LabMidi)

# On Apple platforms the built-in DLS software synthesizer (LabMidiSoftSynth.cpp)
# is guarded by __MACOSX_CORE__ and depends on the CoreAudio framework family.
# Without these, the synth silently compiles to an empty stub and nothing plays.
if(APPLE)
    target_compile_definitions(LabMidi PRIVATE __MACOSX_CORE__)
    target_link_libraries(LabMidi PRIVATE
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework CoreFoundation"
        "-framework CoreServices")
endif()

set_target_properties(
    LabMidi
    PROPERTIES
        FOLDER "LabMidi"
        POSITION_INDEPENDENT_CODE ON
        PUBLIC_HEADER "${LABMIDI_HEADERS}"
        OUTPUT_NAME "LabMidi"
        OUTPUT_NAME_DEBUG "LabMidi_d"
        INTERFACE_INCLUDE_DIRECTORIES ${LABMIDI_ROOT}/include
)

# Create configuration file for CMake package config
configure_file(cmake/LabMidiConfig.cmake.in "${PROJECT_BINARY_DIR}/LabMidiConfig.cmake" @ONLY)
install(TARGETS LabMidi
    EXPORT LabMidi
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include")

install(FILES
  "${PROJECT_BINARY_DIR}/LabMidiConfig.cmake" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake"
)

option(LABMIDI_BUILD_EXAMPLES "Build example applications" OFF)

if(LABMIDI_BUILD_EXAMPLES)
    add_executable(LabMidiApp examples/MidiApp.cpp)
    target_link_libraries(LabMidiApp PRIVATE LabMidi)

    add_executable(LabMidiPlayerApp examples/MidiPlayerApp.cpp examples/OptionParser.cpp)
    target_link_libraries(LabMidiPlayerApp PRIVATE LabMidi)

    add_executable(LabMidiPortsApp examples/MidiPortsApp.cpp examples/OptionParser.cpp)
    target_link_libraries(LabMidiPortsApp PRIVATE LabMidi)

    # Install examples
    install(TARGETS LabMidiApp LabMidiPlayerApp LabMidiPortsApp
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    )
endif()

#-----------------------------------------------------------------------------
# LabMuse : a scoped, faithful iMUSE interactive-music engine built on LabMidi
#-----------------------------------------------------------------------------

set(LABMUSE_HEADERS
    LabMuse/include/LabMuse/LabMuse.h
)

set(LABMUSE_SOURCES
    LabMuse/src/LabMuseEngine.cpp
    LabMuse/src/LabMuseSequencer.cpp
)

add_library(LabMuse ${LABMUSE_SOURCES} ${LABMUSE_HEADERS} LabMuse/src/LabMuseInternal.h)
target_include_directories(LabMuse PUBLIC LabMuse/include)
target_link_libraries(LabMuse PUBLIC LabMidi)
add_library(Lab::Muse ALIAS LabMuse)

set_target_properties(LabMuse PROPERTIES
    FOLDER "LabMuse"
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME "LabMuse"
    OUTPUT_NAME_DEBUG "LabMuse_d")

option(LABMUSE_BUILD_EXAMPLES "Build LabMuse example applications" ON)

if(LABMUSE_BUILD_EXAMPLES)
    add_executable(DieToteninsel
        LabMuse/examples/DieToteninsel.cpp
        LabMuse/examples/LabMuseComposition.cpp)
    target_link_libraries(DieToteninsel PRIVATE LabMuse)
    set_target_properties(DieToteninsel PROPERTIES FOLDER "LabMuse")

    install(TARGETS DieToteninsel
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    )
endif()
