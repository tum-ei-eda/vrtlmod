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
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)

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
find_package(VERILATOR REQUIRED)

find_package(SystemCLanguage REQUIRED)
get_target_property(SYSTEMC_INCLUDE_DIRS SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)
set(SystemC_LIBRARIES SystemC::systemc)

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
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_NAME}/sc_obj_dir
    VERILATOR_ARGS ${VERILATE_ARGS}
    SOURCES ${RTL_SRCS}
)

target_link_libraries(dummy_proj PRIVATE
    ${SystemC_LIBRARIES}
)
target_include_directories(dummy_proj PRIVATE
    ${SYSTEMC_INCLUDE_DIRS}
)

file(GLOB CC_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_NAME}/sc_obj_dir/*)

find_package(vrtlmod HINTS $ENV{VRTLMOD_ROOT} ${VRTLMOD_ROOT} REQUIRED)

set(VRTL_DIR ${VRTL_NAME}/sc_obj_dir)

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

set(GENERATED_API_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmodapi.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmod_diffapi.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmod_diffapi_compare_fast.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmod_diffapi_compare.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/V${TOP_NAME}_vrtlmod_diffapi_compute.cpp
)

if(MAKE_VRTLMOD)
    vrtlmod(
        SOURCES ${CC_SOURCES}
        OUTPUT ${VRTLMOD_OUTSOURCES} ${GENERATED_API_SRCS} 
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
    ${GENERATED_API_SRCS}
)
target_link_libraries(V${TOP_NAME}_vrtlmod PUBLIC
    ${SystemC_LIBRARIES}
)
target_include_directories(V${TOP_NAME}_vrtlmod PUBLIC
    ${VERILATOR_INCLUDE_DIRECTORY}
    ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}
    ${SYSTEMC_INCLUDE_DIRS}
)
message("SYSTEMC_INCLUDE_DIRS: ${SYSTEMC_INCLUDE_DIRS}")
message("SystemC_LIBRARIES: ${SystemC_LIBRARIES}")
add_executable(${PROJECT_NAME}
    sc_fiapp_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../testinject.cpp
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    V${TOP_NAME}_vrtlmod
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

message("VRTL_SRCS after vrtlmod: ${VRTLMOD_OUTSOURCES}")
