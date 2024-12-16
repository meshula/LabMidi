cmake_minimum_required(VERSION 3.15)

# Guard against multiple inclusion
include_guard()

project(LabMidi
    VERSION 1.0.0
    DESCRIPTION "MIDI library for C++"
    LANGUAGES CXX)

# set c++ to 17
set(CMAKE_CXX_STANDARD 17)

# Options
option(LABMIDI_BUILD_EXAMPLES "Build example applications" ON)
option(LABMIDI_BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LABMIDI_INSTALL "Generate installation target" ON)

# Define library type (static/shared)
set(LABMIDI_LIB_TYPE STATIC)
if(LABMIDI_BUILD_SHARED_LIBS)
    set(LABMIDI_LIB_TYPE SHARED)
endif()

# Dependencies
set(LABMIDI_INTERFACE_DEFS)
set(LABMIDI_PRIVATE_DEFS)
set(LABMIDI_PUBLIC_LIBS)
set(LABMIDI_PRIVATE_LIBS)
set(LABMIDI_PRIVATE_INCLUDES)

if(WIN32)
    list(APPEND LABMIDI_INTERFACE_DEFS 
        __WINDOWS_MM__
        _CRT_SECURE_NO_WARNINGS)
    list(APPEND LABMIDI_PUBLIC_LIBS winmm)
elseif(APPLE)
    list(APPEND LABMIDI_INTERFACE_DEFS __MACOSX_CORE__)
    list(APPEND LABMIDI_PUBLIC_LIBS 
        "-framework CoreFoundation" 
        "-framework CoreMidi" 
        "-framework CoreAudio" 
        "-framework AudioToolbox")
elseif(UNIX)
    find_library(JACK_LIB jack)
    find_package(PkgConfig)
    pkg_check_modules(jack jack)
    if(JACK_LIB OR jack_FOUND)
        list(APPEND LABMIDI_INTERFACE_DEFS "__LINUX_JACK__")
        list(APPEND LABMIDI_PRIVATE_LIBS ${jack_LIBRARIES})
        list(APPEND LABMIDI_PRIVATE_INCLUDES ${jack_INCLUDEDIR})
    else()
        find_package(ALSA REQUIRED)
        list(APPEND LABMIDI_INTERFACE_DEFS "__LINUX_ALSASEQ__")
        list(APPEND LABMIDI_PRIVATE_LIBS ${ALSA_LIBRARY})
        list(APPEND LABMIDI_PRIVATE_INCLUDES ${ALSA_INCLUDE_DIR})
    endif()
    find_package(Threads REQUIRED)
    list(APPEND LABMIDI_PRIVATE_LIBS Threads::Threads)
endif()

list(APPEND LABMIDI_PRIVATE_DEFS RTMIDI_DO_NOT_ENSURE_UNIQUE_PORTNAMES)

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

set(RTMIDI_SOURCES
    third-party/rtmidi-1.0.15/RtMidi.cpp
    third-party/rtmidi-1.0.15/RtMidi.h
)

# Core Library
add_library(liblabmidi ${LABMIDI_LIB_TYPE} 
    ${LABMIDI_HEADERS} 
    ${LABMIDI_SOURCES} 
    ${RTMIDI_SOURCES}
)

add_library(Lab::Midi ALIAS liblabmidi)

target_include_directories(liblabmidi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${LABMIDI_PRIVATE_INCLUDES}
)

target_compile_definitions(liblabmidi
    PUBLIC ${LABMIDI_INTERFACE_DEFS}
    PRIVATE ${LABMIDI_PRIVATE_DEFS}
)

target_link_libraries(liblabmidi
    PUBLIC ${LABMIDI_PUBLIC_LIBS}
    PRIVATE ${LABMIDI_PRIVATE_LIBS}
)

set_target_properties(liblabmidi PROPERTIES
    OUTPUT_NAME labmidi
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Examples
if(LABMIDI_BUILD_EXAMPLES)
    add_executable(LabMidiApp examples/MidiApp.cpp)
    target_link_libraries(LabMidiApp PRIVATE Lab::Midi)

    add_executable(LabMidiPlayerApp examples/MidiPlayerApp.cpp examples/OptionParser.cpp)
    target_link_libraries(LabMidiPlayerApp PRIVATE Lab::Midi)

    add_executable(LabMidiPortsApp examples/MidiPortsApp.cpp examples/OptionParser.cpp)
    target_link_libraries(LabMidiPortsApp PRIVATE Lab::Midi)
endif()

# Installation and Export
if(LABMIDI_INSTALL)
    include(GNUInstallDirs)

    # Install library and headers
    install(TARGETS liblabmidi
        EXPORT LabMidiTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/LabMidi/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LabMidi
        FILES_MATCHING PATTERN "*.h"
    )

    # Export targets
    install(EXPORT LabMidiTargets
        FILE LabMidiTargets.cmake
        NAMESPACE Lab::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi
    )

    # Config files
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LabMidiConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi
    )
endif()
