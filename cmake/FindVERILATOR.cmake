####################################################################################################
# Copyright 2022 Chair of EDA, Technical University of Munich
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################################

# Select one installation: CMake VERILATOR_ROOT, then its environment value,
# then the verilator wrapper on PATH. Executable, package and headers stay paired.
function(_vrtlmod_verilator_root candidate output)
    get_filename_component(candidate "${candidate}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(EXISTS "${candidate}/verilator-config.cmake" AND EXISTS "${candidate}/include/verilated.h")
        get_filename_component(root "${candidate}" REALPATH)
    elseif(EXISTS "${candidate}/share/verilator/verilator-config.cmake"
           AND EXISTS "${candidate}/share/verilator/include/verilated.h")
        get_filename_component(root "${candidate}/share/verilator" REALPATH)
    else()
        message(FATAL_ERROR
            "Invalid Verilator root '${candidate}': expected a source/build tree, install prefix, "
            "or share/verilator directory containing its CMake package and headers.")
    endif()
    set(${output} "${root}" PARENT_SCOPE)
endfunction()

unset(_VRTLMOD_REQUESTED_ROOT)
if(DEFINED VERILATOR_ROOT AND NOT "${VERILATOR_ROOT}" STREQUAL "")
    set(_VRTLMOD_REQUESTED_ROOT "${VERILATOR_ROOT}")
elseif(NOT "$ENV{VERILATOR_ROOT}" STREQUAL "")
    set(_VRTLMOD_REQUESTED_ROOT "$ENV{VERILATOR_ROOT}")
endif()

unset(_VRTLMOD_VERILATOR_WRAPPER)
unset(_VRTLMOD_VERILATOR_WRAPPER CACHE)
if(DEFINED _VRTLMOD_REQUESTED_ROOT)
    _vrtlmod_verilator_root("${_VRTLMOD_REQUESTED_ROOT}" _VRTLMOD_SELECTED_ROOT)
    find_program(_VRTLMOD_VERILATOR_WRAPPER NAMES verilator
        PATHS "${_VRTLMOD_SELECTED_ROOT}/bin" "${_VRTLMOD_SELECTED_ROOT}/../../bin"
        NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    if(NOT _VRTLMOD_VERILATOR_WRAPPER)
        message(FATAL_ERROR "Cannot find the verilator wrapper for VERILATOR_ROOT='${_VRTLMOD_REQUESTED_ROOT}'.")
    endif()
else()
    unset(_VRTLMOD_SELECTED_ROOT)
    find_program(_VRTLMOD_VERILATOR_WRAPPER NAMES verilator
        PATHS ENV PATH NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    if(NOT _VRTLMOD_VERILATOR_WRAPPER)
        message(FATAL_ERROR "Cannot find verilator on PATH. Install Verilator or set VERILATOR_ROOT.")
    endif()
endif()
get_filename_component(VERILATOR_EXECUTABLE "${_VRTLMOD_VERILATOR_WRAPPER}" REALPATH)
unset(_VRTLMOD_VERILATOR_WRAPPER CACHE)

# An inherited root or binary override can redirect even an absolute wrapper.
set(_VRTLMOD_VERILATOR_COMMAND "${CMAKE_COMMAND}" -E env
    --unset=VERILATOR_ROOT --unset=VERILATOR_BIN "${VERILATOR_EXECUTABLE}")
execute_process(COMMAND ${_VRTLMOD_VERILATOR_COMMAND} --getenv VERILATOR_ROOT
    RESULT_VARIABLE _VRTLMOD_ROOT_RESULT OUTPUT_VARIABLE _VRTLMOD_REPORTED_ROOT
    ERROR_VARIABLE _VRTLMOD_ROOT_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _VRTLMOD_ROOT_RESULT EQUAL 0 OR "${_VRTLMOD_REPORTED_ROOT}" STREQUAL "")
    message(FATAL_ERROR "Cannot query VERILATOR_ROOT from '${VERILATOR_EXECUTABLE}': ${_VRTLMOD_ROOT_ERROR}")
endif()
_vrtlmod_verilator_root("${_VRTLMOD_REPORTED_ROOT}" _VRTLMOD_REPORTED_ROOT)
if(DEFINED _VRTLMOD_SELECTED_ROOT AND NOT _VRTLMOD_SELECTED_ROOT STREQUAL _VRTLMOD_REPORTED_ROOT)
    message(FATAL_ERROR
        "Verilator wrapper '${VERILATOR_EXECUTABLE}' reports '${_VRTLMOD_REPORTED_ROOT}', "
        "which does not match requested root '${_VRTLMOD_SELECTED_ROOT}'.")
endif()

execute_process(COMMAND ${_VRTLMOD_VERILATOR_COMMAND} --version
    RESULT_VARIABLE _VRTLMOD_VERSION_RESULT OUTPUT_VARIABLE _VRTLMOD_VERSION_OUTPUT
    ERROR_VARIABLE _VRTLMOD_VERSION_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _VRTLMOD_VERSION_RESULT EQUAL 0 OR NOT _VRTLMOD_VERSION_OUTPUT MATCHES "Verilator ([0-9]+[.][0-9]+)")
    message(FATAL_ERROR "Cannot query the version of '${VERILATOR_EXECUTABLE}': ${_VRTLMOD_VERSION_ERROR}")
endif()
set(_VRTLMOD_EXECUTABLE_VERSION "${CMAKE_MATCH_1}")

set(VERILATOR_ROOT "${_VRTLMOD_REPORTED_ROOT}" CACHE PATH "Selected Verilator runtime root" FORCE)
set(VERILATOR_ROOT "${_VRTLMOD_REPORTED_ROOT}")
set(verilator_DIR "${VERILATOR_ROOT}" CACHE PATH "CMake package for the selected Verilator" FORCE)
set(verilator_DIR "${VERILATOR_ROOT}")

set(VERILATOR_BIN "${VERILATOR_EXECUTABLE}" CACHE FILEPATH "Selected Verilator wrapper" FORCE)
set(VERILATOR_BIN "${VERILATOR_EXECUTABLE}")
unset(VERILATOR_BIN_COMMAND)
unset(VERILATOR_BIN_COMMAND CACHE)

find_package(verilator ${_VRTLMOD_EXECUTABLE_VERSION} EXACT CONFIG REQUIRED
    PATHS "${VERILATOR_ROOT}" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

# Newer packages expose a command list, separate from the executable dependency.
if(DEFINED VERILATOR_BIN_COMMAND)
    set(VERILATOR_BIN_COMMAND "${CMAKE_COMMAND}" -E env
        --unset=VERILATOR_ROOT --unset=VERILATOR_BIN ${VERILATOR_BIN_COMMAND})
endif()

# Keep upstream verilate() intact. Older packages need scoped configure-time
# environment changes and a target-specific launcher for generated build rules.
function(vrtlmod_verilate target)
    if(DEFINED VERILATOR_BIN_COMMAND)
        verilate(${target} ${ARGN})
        return()
    endif()
    if(NOT CMAKE_GENERATOR MATCHES "Makefiles|WMake|Ninja")
        message(FATAL_ERROR
            "Verilator ${verilator_VERSION} requires a Makefile or Ninja generator for vrtlmod_verilate(). "
            "The '${CMAKE_GENERATOR}' generator does not support its build-command environment handling.")
    endif()

    foreach(variable IN ITEMS VERILATOR_ROOT VERILATOR_BIN)
        set(_VRTLMOD_SAVED_${variable}_DEFINED FALSE)
        if(DEFINED ENV{${variable}})
            set(_VRTLMOD_SAVED_${variable}_DEFINED TRUE)
            set(_VRTLMOD_SAVED_${variable} "$ENV{${variable}}")
        endif()
        unset(ENV{${variable}})
    endforeach()
    verilate(${target} ${ARGN})
    foreach(variable IN ITEMS VERILATOR_ROOT VERILATOR_BIN)
        if(_VRTLMOD_SAVED_${variable}_DEFINED)
            set(ENV{${variable}} "${_VRTLMOD_SAVED_${variable}}")
        else()
            unset(ENV{${variable}})
        endif()
    endforeach()

    get_property(launcher TARGET ${target} PROPERTY RULE_LAUNCH_CUSTOM)
    get_property(applied_launcher TARGET ${target} PROPERTY _VRTLMOD_VERILATOR_LAUNCHER)
    if(NOT applied_launcher OR NOT launcher STREQUAL applied_launcher)
        set(prefix "\"${CMAKE_COMMAND}\" -E env --unset=VERILATOR_ROOT --unset=VERILATOR_BIN")
        if(launcher)
            string(APPEND prefix " ${launcher}")
        endif()
        set_property(TARGET ${target} PROPERTY RULE_LAUNCH_CUSTOM "${prefix}")
        set_property(TARGET ${target} PROPERTY _VRTLMOD_VERILATOR_LAUNCHER "${prefix}")
    endif()
endfunction()

set(VERILATOR_FOUND "${verilator_FOUND}")
set(VERILATOR_INCLUDE_DIRECTORY "${VERILATOR_ROOT}/include")
set(VERILATOR_INCLUDE_DIRS "${VERILATOR_INCLUDE_DIRECTORY}"
    "${VERILATOR_INCLUDE_DIRECTORY}/vltstd" "${VERILATOR_INCLUDE_DIRECTORY}/gtkwave")
string(REPLACE "." "" VRTLMOD_VERILATOR_VERSION "${verilator_VERSION}")
message(STATUS "VERILATOR_EXECUTABLE: ${VERILATOR_EXECUTABLE}")
message(STATUS "VERILATOR_ROOT: ${VERILATOR_ROOT}")
message(STATUS "VERILATOR_VERSION: ${verilator_VERSION}")
