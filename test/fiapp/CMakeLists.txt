####################################################################################################
# Copyright 2022 Chair of EDA, Technical University of Munich
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################################

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

set(VRTL_NAME fiapp)
set(TOP_NAME ${VRTL_NAME})
project(fiapp-cmake-test)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    message(WARNING "CMAKE_CXX_STANDARD not set. Default to ${CMAKE_CXX_STANDARD}")
endif()

if(NOT IS_DIRECTORY $ENV{VERILATOR_ROOT})
    message(FATAL_ERROR "Set VERILATOR_ROOT environment variable!")
endif()
set(VERILATOR_ROOT $ENV{VERILATOR_ROOT})
message(STATUS "VERILATOR_ROOT: ${VERILATOR_ROOT}")
if(EXISTS "${VERILATOR_ROOT}/Makefile")
    # This is a non-install Verilator, aka repository+build
    # VERILATOR_ROOT/
    # |-> bin/verilator_bin
    # |-> src/
    # |-> ...
    # \-> include/
    set(VERILATOR_INCLUDE_DIRECTORY "${VERILATOR_ROOT}/include")
    find_package(verilator HINTS ${VERILATOR_ROOT} REQUIRED)
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
    else()
        # This might be pointing to <install>/share/verilator/ and is not supported
        # VERILATOR_ROOT/
        #               |-> bin/verilator_includer
        #               |-> include/
        #               \-> examples/
        message(FATAL_ERROR "Set VERILATOR_ROOT to base <install> directory!")
    endif()
endif()

####################################################################################################
# SystemC by conan package manager settings
if(NOT SYSTEMC_INCLUDE_DIRS AND NOT SystemC_LIBRARIES)

    if(NOT USE_CWR_SYSTEMC AND NOT USE_NCSC_SYSTEMC AND NOT DEFINED ENV{SYSTEMC_HOME})

        set(CONAN_CMAKE_LIST_DIR ${CMAKE_CURRENT_BINARY_DIR})
        if(NOT EXISTS ${CONAN_CMAKE_LIST_DIR}/ConanInline.cmake)
            message("Downloading ConanInline to ${CONAN_CMAKE_LIST_DIR}")
            set(URL https://raw.githubusercontent.com/VP-Vibes/SystemC-Components/main/cmake/ConanInline.cmake)
            file(DOWNLOAD ${URL} ${CONAN_CMAKE_LIST_DIR}/ConanInline.cmake TIMEOUT 60)
            set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_BINARY_DIR})
        endif()

        include(ConanInline)

        set(CONAN_PACKAGE_LIST ${CONAN_PACKAGE_LIST} systemc/2.3.3 systemc-cci/1.0.0)
        set(CONAN_PACKAGE_OPTIONS ${CONAN_PACKAGE_OPTIONS} systemc:shared=True systemc-cci:shared=True systemc:disable_virtual_bind=False)

        conan_check()
        conan_cmake_configure(
            REQUIRES ${CONAN_PACKAGE_LIST}
            GENERATORS cmake_find_package
            OPTIONS ${CONAN_PACKAGE_OPTIONS}
        )

        conan_cmake_autodetect(settings)

        set(settings ${settings} compiler.cppstd=${CMAKE_CXX_STANDARD})

        message("Conan settings are ${settings}")
        conan_cmake_install(
            PATH_OR_REFERENCE ${CMAKE_CURRENT_BINARY_DIR}
            BUILD missing
            SETTINGS ${settings}
        )

    endif()

    find_package(SystemCLanguage REQUIRED)
    get_target_property(SYSTEMC_INCLUDE_DIRS SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

endif()

# rtl sources: set RTL_SOURCES, RTL_INCLUDE_DIRS, and VERILATE_ARGS in PARENT_SCOPE:
set(RTL_SRCS
    fiapp.sv
)

set(RTL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(VERILATE_ARGS
    -sv
    -O3
    --unroll-count 256
    --pins-bv 2
    -Werror-PINMISSING
    -Wno-fatal
    -Wno-PINCONNECTEMPTY
    -Wno-ASSIGNDLY
    -Wno-DECLFILENAME
    -Wno-UNUSED
    -Wno-UNOPTFLAT
    -Wno-style
    -Wno-CASEINCOMPLETE
    -Wno-LITENDIAN
    -Wno-UNOPT
    -Wno-WIDTH
    -Wno-BLKANDNBLK
)

message(${RTL_SRCS})

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.cpp "")

add_library(dummy_proj SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/null.cpp
)

verilate(dummy_proj
    PREFIX V${TOP_NAME}
    SYSTEMC
    TOP_MODULE ${TOP_NAME}
    INCLUDE_DIRS ${RTL_INCLUDE_DIRS}
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_NAME}/obj_dir
    VERILATOR_ARGS ${VERILATE_ARGS}
    SOURCES ${RTL_SRCS}
)

target_link_libraries(dummy_proj PRIVATE
    ${SystemC_LIBRARIES}
)
target_include_directories(dummy_proj PRIVATE
    ${SYSTEMC_INCLUDE_DIRS}
)

file(GLOB CC_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_NAME}/obj_dir/*)

find_package(vrtlmod HINTS $ENV{VRTLMOD_ROOT} ${VRTLMOD_ROOT} REQUIRED)

set(VRTL_DIR ${VRTL_NAME}/obj_dir)

file( GLOB VRTL_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_DIR}/*.cpp)

set(VRTLMOD_OUTSOURCES ${VRTL_SRCS})

set(VRTLMOD_OUT_DIR "${VRTL_DIR}/vrtlmod/")

list(TRANSFORM VRTLMOD_OUTSOURCES REPLACE "/${VRTL_DIR}/" "/${VRTLMOD_OUT_DIR}/")

message("vrtlmod outsources" ${VRTLMOD_OUTSOURCES})

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}")
    set(MAKE_VRTLMOD On)
else()
    set(VARSET ${VRTLMOD_OUTSOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmodapi.cpp)
    foreach(var ${VARSET})
        if( NOT EXISTS "${var}" )
            set(MAKE_VRTLMOD On)
        endif()
    endforeach()
endif()

message("run vrtlmod: " ${MAKE_VRTLMOD})

if(MAKE_VRTLMOD)
    vrtlmod(
        SOURCES ${CC_SOURCES}
        OUTPUT ${VRTLMOD_OUTSOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmodapi.cpp
        VERBOSE
        SYSTEMC
        OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}
        INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_DIR}/ ${VERILATOR_INCLUDE_DIRECTORY} ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
        ${SYSTEMC_INCLUDE_DIRS}
    )
endif()

add_library(V${TOP_NAME}_vrtlmod SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/null.cpp
    ${VRTLMOD_OUTSOURCES}
    ${VERILATOR_INCLUDE_DIRECTORY}/verilated.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmodapi.cpp
)
target_include_directories(V${TOP_NAME}_vrtlmod PUBLIC
    ${VERILATOR_INCLUDE_DIRECTORY}
    ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}
    ${SYSTEMC_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    sc_fiapp_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../testinject.cpp
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    V${TOP_NAME}_vrtlmod
    ${SystemC_LIBRARIES}
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${SYSTEMC_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

message("VRTL_SRCS after vrtlmod: ${VRTLMOD_OUTSOURCES}")
