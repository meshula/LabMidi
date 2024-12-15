cmake_minimum_required(VERSION 3.15)

include_guard()
project(LabMidi
    VERSION 1.0.0
    DESCRIPTION "MIDI library for C++"
    LANGUAGES CXX)

# Options
option(LABMIDI_BUILD_EXAMPLES "Build example applications" ON)
option(LABMIDI_BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LABMIDI_INSTALL "Generate installation target" ON)

# Global settings
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_DEBUG_POSTFIX _d)

# Define library type (static/shared)
if(LABMIDI_BUILD_SHARED_LIBS)
    set(LABMIDI_LIB_TYPE SHARED)
else()
    set(LABMIDI_LIB_TYPE STATIC)
endif()

#------------------------------------------------------------------------------
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
        set(HAVE_JACK TRUE)
        set(NEED_PTHREAD ON)
        list(APPEND PKGCONFIG_REQUIRES "jack")
        list(APPEND LABMIDI_INTERFACE_DEFS "__LINUX_JACK__")
        if(jack_FOUND)
            list(APPEND LABMIDI_PRIVATE_LIBS ${jack_LIBRARIES})
            list(APPEND LABMIDI_PRIVATE_INCLUDES ${jack_INCLUDEDIR})
        else()
            list(APPEND LABMIDI_PRIVATE_LIBS ${JACK_LIB})
        endif()
    else()
        find_package(ALSA REQUIRED)
        set(NEED_PTHREAD ON)
        list(APPEND LABMIDI_PRIVATE_LIBS ${ALSA_LIBRARY})
        list(APPEND LABMIDI_PRIVATE_INCLUDES ${ALSA_INCLUDE_DIR})
        list(APPEND LABMIDI_INTERFACE_DEFS "__LINUX_ALSASEQ__")
    endif()

    if(NEED_PTHREAD)
        find_package(Threads REQUIRED
            CMAKE_THREAD_PREFER_PTHREAD
            THREADS_PREFER_PTHREAD_FLAG)
        list(APPEND LABMIDI_PRIVATE_LIBS Threads::Threads)
    endif()
endif()

# Add RtMidi-specific definitions
list(APPEND LABMIDI_PRIVATE_DEFS RTMIDI_DO_NOT_ENSURE_UNIQUE_PORTNAMES)

#------------------------------------------------------------------------------
# Core Library

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

add_library(liblabmidi ${LABMIDI_LIB_TYPE}
    ${LABMIDI_HEADERS}
    ${LABMIDI_SOURCES}
    ${RTMIDI_SOURCES}
)

add_library(Lab::Midi ALIAS liblabmidi)

# Set modern C++ standard
target_compile_features(liblabmidi PUBLIC cxx_std_14)

# Include directories
target_include_directories(liblabmidi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/rtmidi-1.0.15
        ${LABMIDI_PRIVATE_INCLUDES}
)

# Compile definitions and link libraries
target_compile_definitions(liblabmidi
    PUBLIC ${LABMIDI_INTERFACE_DEFS}
    PRIVATE ${LABMIDI_PRIVATE_DEFS}
)

target_link_libraries(liblabmidi
    PUBLIC ${LABMIDI_PUBLIC_LIBS}
    PRIVATE ${LABMIDI_PRIVATE_LIBS}
)

# Set properties
set_target_properties(liblabmidi PROPERTIES
    OUTPUT_NAME labmidi
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    FOLDER "Library"
)

#------------------------------------------------------------------------------
# Examples
if(LABMIDI_BUILD_EXAMPLES)
    # MidiApp
    add_executable(LabMidiApp
        examples/MidiApp.h
        examples/MidiApp.cpp
    )
    target_link_libraries(LabMidiApp PRIVATE Lab::Midi)
    set_target_properties(LabMidiApp PROPERTIES FOLDER "Examples")

    # MidiPlayerApp
    add_executable(LabMidiPlayerApp
        examples/MidiPlayerApp.h
        examples/MidiPlayerApp.cpp
        examples/OptionParser.h
        examples/OptionParser.cpp
    )
    target_link_libraries(LabMidiPlayerApp PRIVATE Lab::Midi)
    set_target_properties(LabMidiPlayerApp PROPERTIES FOLDER "Examples")

    # MidiPortsApp
    add_executable(LabMidiPortsApp
        examples/MidiPortsApp.h
        examples/MidiPortsApp.cpp
        examples/OptionParser.h
        examples/OptionParser.cpp
    )
    target_link_libraries(LabMidiPortsApp PRIVATE Lab::Midi)
    set_target_properties(LabMidiPortsApp PROPERTIES FOLDER "Examples")
endif()

#------------------------------------------------------------------------------
# Installation

if(LABMIDI_INSTALL)
    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)

    # Install library and headers
    install(TARGETS liblabmidi
        EXPORT LabMidiTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        DIRECTORY include/LabMidi
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )

    # Configure and install targets file
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LabMidiTargets.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiTargets.cmake
        @ONLY
    )

    # Export targets
    install(EXPORT LabMidiTargets
        FILE LabMidiTargets.cmake
        NAMESPACE Lab::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi
    )

    # Create and install config files
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LabMidiConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install config files
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LabMidiConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LabMidi
    )

    # Install examples if built
    if(LABMIDI_BUILD_EXAMPLES)
        install(TARGETS 
            LabMidiApp
            LabMidiPlayerApp
            LabMidiPortsApp
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()
