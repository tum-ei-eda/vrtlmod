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

if(NOT VERILATOR_ROOT)
    message(WARNING "VERILATOR_ROOT not set by CMake. Trying environment...")
    set(VERILATOR_ROOT $ENV{VERILATOR_ROOT})
endif()
if(NOT VERILATOR_ROOT)
    message(FATAL_ERROR "Please specify VERILATOR_ROOT in environment or CMake CL.")
else()
    message(STATUS "VERILATOR_ROOT: ${VERILATOR_ROOT}")
endif()

# Bind the executable to the requested tree, including when ROOT is share/verilator.
# Otherwise the upstream package can pick a different version from PATH.
unset(_VRTLMOD_VERILATOR_WRAPPER CACHE)
find_program(_VRTLMOD_VERILATOR_WRAPPER NAMES verilator
    HINTS "${VERILATOR_ROOT}/bin" "${VERILATOR_ROOT}/../../bin"
    NO_DEFAULT_PATH)
if(_VRTLMOD_VERILATOR_WRAPPER)
    set(VERILATOR_BIN "${_VRTLMOD_VERILATOR_WRAPPER}")
endif()
unset(_VRTLMOD_VERILATOR_WRAPPER CACHE)

if(EXISTS "${VERILATOR_ROOT}/Makefile")
    # This is a non-install Verilator, aka repository+build
    # VERILATOR_ROOT/
    # |-> bin/verilator_bin
    # |-> src/
    # |-> ...
    # \-> include/
    set(VERILATOR_INCLUDE_DIRECTORY "${VERILATOR_ROOT}/include")
    find_package(verilator HINTS ${VERILATOR_ROOT} REQUIRED)

    set(VERILATOR_INCLUDE_DIRS ${VERILATOR_INCLUDE_DIRECTORY} ${VERILATOR_INCLUDE_DIRECTORY}/vltstd ${VERILATOR_INCLUDE_DIRECTORY}/gtkwave)
else()
    # This is an installed Verilator:
    if(IS_DIRECTORY "${VERILATOR_ROOT}/share/verilator/include")
        # VERILATOR_ROOT/
        # |-> bin/verilator_bin
        # \-> share/
        #     \-> verilator/
        #         |-> bin/verilator_includer
        #         |-> include/
        #         \-> examples/
        set(VERILATOR_INCLUDE_DIRECTORY "${VERILATOR_ROOT}/share/verilator/include")
        find_package(verilator HINTS ${VERILATOR_ROOT}/share/verilator REQUIRED)
        set(VERILATOR_ROOT "${VERILATOR_ROOT}/share/verilator") # TODO: This is a workaround to trick verilate() function

        set(VERILATOR_INCLUDE_DIRS ${VERILATOR_INCLUDE_DIRECTORY} ${VERILATOR_INCLUDE_DIRECTORY}/vltstd ${VERILATOR_INCLUDE_DIRECTORY}/gtkwave)

    elseif(IS_DIRECTORY "${VERILATOR_ROOT}/include")
        # Verilator's own CMake package uses <install>/share/verilator as its root.
        set(VERILATOR_INCLUDE_DIRECTORY "${VERILATOR_ROOT}/include")
        find_package(verilator HINTS ${VERILATOR_ROOT} REQUIRED)
        set(VERILATOR_INCLUDE_DIRS ${VERILATOR_INCLUDE_DIRECTORY} ${VERILATOR_INCLUDE_DIRECTORY}/vltstd ${VERILATOR_INCLUDE_DIRECTORY}/gtkwave)
    else()
        message(FATAL_ERROR "VERILATOR_ROOT must point to a Verilator source tree, install prefix, or share/verilator directory.")
    endif()
endif()

if(verilator_FOUND)
    message(STATUS "VERILATOR_VERSION: ${verilator_VERSION}")
    string(REPLACE "." "" VRTLMOD_VERILATOR_VERSION "${verilator_VERSION}")
endif()
