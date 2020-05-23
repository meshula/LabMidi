cmake_minimum_required(VERSION 3.10)

include_guard()
project(LabMidi)

set(LABMIDI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_DEBUG_POSTFIX _d)

#------------------------------------------------------------------------------
# Dependencies

set(LABMIDI_DEFS)
set(LABMIDI_LIBS)
set(LABMIDI_INC)

if (WIN32)
    list(APPEND LABMIDI_DEFS __WINDOWS_MM__)
    list(APPEND LABMIDI_DEFS _CRT_SECURE_NO_WARNINGS)
    list(APPEND LABMIDI_LIBS winmm.lib)
elseif (APPLE)
    list(APPEND LABMIDI_DEFS __MACOSX_CORE__)
    list(APPEND LABMIDI_LIBS 
        "-framework CoreFoundation" 
        "-framework CoreMidi" 
        "-framework CoreAudio" 
        "-framework AudioToolbox")
elseif (UNIX)
    find_library(JACK_LIB jack)
    find_package(PkgConfig)
    pkg_check_modules(jack jack)
    if (JACK_LIB OR jack_FOUND)
      set(HAVE_JACK TRUE)
      set(NEED_PTHREAD ON)
      list(APPEND PKGCONFIG_REQUIRES "jack")
      list(APPEND LABMIDI_DEFS "__LINUX_JACK__")
      if (jack_FOUND)
        list(APPEND LABMIDI_LIBS ${jack_LIBRARIES})
        list(APPEND LABMIDI_INC ${jack_INCLUDEDIR})
      else()
        list(APPEND LABMIDI_LIBS ${JACK_LIB})
      endif()
    else ()
      find_package(ALSA)
      if (NOT ALSA_FOUND)
        message(FATAL_ERROR "ALSA API requested but no ALSA dev libraries found")
      endif()
      set(NEED_PTHREAD ON)
      list(APPEND LABMIDI_LIBS ${ALSA_LIBRARY})
      list(APPEND LABMIDI_INC ${ALSA_INCLUDE_DIR})
      list(APPEND LABMIDI_DEFS "__LINUX_ALSASEQ__")
    endif()

    if (NEED_PTHREAD)
      find_package(Threads REQUIRED
        CMAKE_THREAD_PREFER_PTHREAD
        THREADS_PREFER_PTHREAD_FLAG)
      list(APPEND LABMIDI_LIBS Threads::Threads)
    endif()
else()
endif()

#------------------------------------------------------------------------------
# Utilities

function(_set_compile_options proj)
    target_compile_features(${proj} INTERFACE cxx_std_14)
    if(WIN32)
        target_compile_options(${proj} PRIVATE /Zi)
    elseif (APPLE)
    elseif (UNIX)
        target_compile_options(${proj} PRIVATE -fPIC)
    endif()
    set_target_properties(${proj} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    install(TARGETS ${proj}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)
endfunction()

#------------------------------------------------------------------------------
# LabMidi

file(GLOB labmidi_inc "${LABMIDI_ROOT}/include/LabMidi/*")
file(GLOB labmidi_src "${LABMIDI_ROOT}/src/*")
file(GLOB rtmidi_src  "${LABMIDI_ROOT}/third-party/rtmidi-1.0.15/R*.*")

add_library(liblabmidi STATIC 
    ${labmidi_inc} 
    ${labmidi_src} 
    ${rtmidi_src})

target_include_directories(liblabmidi PRIVATE
    ${LABMIDI_ROOT}/include
    ${LABMIDI_ROOT}/third-party/rtmidi-1.0.15
    ${LABMIDI_ROOT}/src
    ${LABMIDI_INC})

target_compile_definitions(liblabmidi PRIVATE ${LABMIDI_DEFS})
target_link_libraries(liblabmidi PUBLIC ${LABMIDI_LIBS})
_set_compile_options(liblabmidi)
set_property(TARGET liblabmidi PROPERTY CXX_STANDARD 14)
source_group(LabMidi\\include FILES ${labmidi_inc})
source_group(LabMidi\\src FILES ${labmidi_src})
source_group(LabMidi\\RtMidi FILES ${rtmidi_src})
install(FILES ${labmidi_inc} DESTINATION include/LabMidi)
add_library(Lab::Midi ALIAS liblabmidi)

#------------------------------------------------------------------------------
# example: MidiApp

set(midiapp_src "${LABMIDI_ROOT}/examples/MidiApp.h"
                "${LABMIDI_ROOT}/examples/MidiApp.cpp")
add_executable(LabMidiApp ${midiapp_src})
target_link_libraries(LabMidiApp Lab::Midi)
target_include_directories(LabMidiApp PRIVATE ${LABMIDI_ROOT}/include)
_set_compile_options(LabMidiApp)
set_property(TARGET LabMidiApp PROPERTY CXX_STANDARD 14)
set_property(TARGET LabMidiApp PROPERTY FOLDER "examples")

#------------------------------------------------------------------------------
# example MidiPlayerApp

set(midiplayerapp_src "${LABMIDI_ROOT}/examples/MidiPlayerApp.h"
                      "${LABMIDI_ROOT}/examples/MidiPlayerApp.cpp"
                      "${LABMIDI_ROOT}/examples/OptionParser.h"
                      "${LABMIDI_ROOT}/examples/OptionParser.cpp")
add_executable(LabMidiPlayerApp ${midiplayerapp_src})
target_link_libraries(LabMidiPlayerApp Lab::Midi)
target_include_directories(LabMidiPlayerApp PRIVATE ${LABMIDI_ROOT}/include)
_set_compile_options(LabMidiPlayerApp)
set_property(TARGET LabMidiPlayerApp PROPERTY CXX_STANDARD 14)
set_property(TARGET LabMidiPlayerApp PROPERTY FOLDER "examples")

#------------------------------------------------------------------------------
# example MidiPorts

set(midiportsapp_src "${LABMIDI_ROOT}/examples/MidiPlayerApp.h"
                      "${LABMIDI_ROOT}/examples/MidiPlayerApp.cpp"
                      "${LABMIDI_ROOT}/examples/OptionParser.h"
                      "${LABMIDI_ROOT}/examples/OptionParser.cpp")
add_executable(LabMidiPortsApp ${midiportsapp_src})
target_link_libraries(LabMidiPortsApp Lab::Midi)
target_include_directories(LabMidiPortsApp PRIVATE ${LABMIDI_ROOT}/include)
_set_compile_options(LabMidiPortsApp)
set_property(TARGET LabMidiPortsApp PROPERTY CXX_STANDARD 14)
set_property(TARGET LabMidiPortsApp PROPERTY FOLDER "examples")


