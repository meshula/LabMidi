cmake_minimum_required(VERSION 3.15)

project(LabMidi
    VERSION 1.0.0
    DESCRIPTION "MIDI library for C++"
    LANGUAGES CXX)

# set c++ to 17
set(CMAKE_CXX_STANDARD 17)

option(LABMIDI_USE_RTMIDI6 "Enable RtMidi 6 support" ON)

if(LABMIDI_USE_RTMIDI6)
    find_package(RtMidi 6.0.0 QUIET)

    if (NOT RtMidi_FOUND)
        message(STATUS "RtMidi not found, using FetchContent to download it")
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

# Create configuration file for CMake package config
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LabMidiConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/LabMidiConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi"
)

# Install the library
install(TARGETS LabMidi
    EXPORT LabMidiTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install package config file
install(FILES "${CMAKE_BINARY_DIR}/LabMidiConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi)

# Create alias target if it doesn't exist
if(NOT TARGET LabMidi)
    add_library(LabMidi ALIAS LabMidi)
endif()

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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
