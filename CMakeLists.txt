cmake_minimum_required(VERSION 3.11)

include_guard()
project(LabMidi)

set(LABMIDI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH ${LABMIDI_ROOT}/cmake)
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX ${LABMIDI_ROOT}/install CACHE PATH "Install root" FORCE)
ENDIF()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
if (NOT CMAKE_DEBUG_POSTFIX)
    set(CMAKE_DEBUG_POSTFIX _d)
endif()

#------------------------------------------------------------------------------
# Utilities

function(_set_compile_options proj)
    target_compile_features(${proj} INTERFACE cxx_std_11)
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
add_library(liblabmidi STATIC ${labmidi_inc} ${labmidi_src} ${rtmidi_src})
target_include_directories(liblabmidi PRIVATE 
    ${LABMIDI_ROOT}/include
    ${LABMIDI_ROOT}/third-party/rtmidi-1.0.15
    ${LABMIDI_ROOT}/src)
if (WIN32)
    target_compile_definitions(liblabmidi PRIVATE __WINDOWS_MM__ _CRT_SECURE_NO_WARNINGS)
endif()
_set_compile_options(liblabmidi)
source_group(LabMidi\\include FILES ${labmidi_inc})
source_group(LabMidi\\src FILES ${labmidi_src})
source_group(LabMidi\\RtMidi FILES ${rtmidi_src})
install(FILES ${labmidi_inc} DESTINATION include/LabMidi)
add_library(LabMidi::Lib ALIAS liblabmidi)

