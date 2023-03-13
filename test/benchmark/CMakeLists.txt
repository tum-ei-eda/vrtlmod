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
project(V${VP})

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    message(WARNING "CMAKE_CXX_STANDARD not set. Default to ${CMAKE_CXX_STANDARD}")
endif()

set(VRTL_NAME ${VP}) #set(TOP_NAME ...) #from PARENT_SCOPE

if(NOT VERILATOR_ROOT)
    set(VERILATOR_ROOT $ENV{VERILATOR_ROOT})
    message(STATUS "VERILATOR_ROOT not specified in CMake .. using environment VERILATOR_ROOT: $ENV{VERILATOR_ROOT}")
endif()
find_package(verilator HINTS ${VERILATOR_ROOT} REQUIRED)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    message(WARNING "CMAKE_CXX_STANDARD not set. Default to ${CMAKE_CXX_STANDARD}")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})
####################################################################################################
# conan package manager settings
set(CONAN_CMAKE_LIST_DIR ${CMAKE_CURRENT_BINARY_DIR})
if(NOT EXISTS ${CONAN_CMAKE_LIST_DIR}/ConanInline.cmake)
    message("Downloading ConanInline to ${CONAN_CMAKE_LIST_DIR}")
    set(URL https://raw.githubusercontent.com/VP-Vibes/SystemC-Components/main/cmake/ConanInline.cmake)
    file(DOWNLOAD ${URL} ${CONAN_CMAKE_LIST_DIR}/ConanInline.cmake TIMEOUT 60)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_BINARY_DIR})
endif()

include(ConanInline)

set(CONAN_PACKAGE_OPTIONS ${CONAN_OPTIONS})

if(NOT USE_CWR_SYSTEMC AND NOT USE_NCSC_SYSTEMC AND NOT DEFINED ENV{SYSTEMC_HOME})
    set(CONAN_PACKAGE_LIST ${CONAN_PACKAGE_LIST} systemc/2.3.3 systemc-cci/1.0.0)
    set(CONAN_PACKAGE_OPTIONS ${CONAN_PACKAGE_OPTIONS} systemc:shared=True systemc-cci:shared=True systemc:disable_virtual_bind=False)
endif()

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
####################################################################################################
include(FetchContent)

if(NOT TARGET scc) # etiss-sc is standalone, i.e. not embedded alongside scc in a project
    FetchContent_Declare(
        scc
        GIT_REPOSITORY https://github.com/VP-Vibes/SystemC-Components.git
        GIT_TAG 8785c03
        PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/patches/scc-byteenabletransactions.patch
    )
    FetchContent_GetProperties(scc)
    if(NOT scc_POPULATED)
        FetchContent_Populate(scc)
    endif()
    # cmake packages
    include(${scc_SOURCE_DIR}/cmake/SystemCPackage.cmake) # get systemc package

    list(APPEND CMAKE_MODULE_PATH ${scc_SOURCE_DIR}/cmake PARENT_SCOPE)

    include(ConanInline)
    set(CONAN_OPTIONS
        boost:shared=True
        boost:header_only=False
        fmt:header_only=True
    )

    set(CONAN_PACKAGE_LIST
        boost/1.76.0
        elfio/3.8
        fmt/6.1.2
        zlib/1.2.11
    )
    set(CONAN_PACKAGE_OPTIONS ${CONAN_OPTIONS})

    if(NOT USE_CWR_SYSTEMC AND NOT USE_NCSC_SYSTEMC AND NOT DEFINED ENV{SYSTEMC_HOME})
        set(CONAN_PACKAGE_LIST ${CONAN_PACKAGE_LIST} systemc/2.3.3 systemc-cci/1.0.0)
        set(CONAN_PACKAGE_OPTIONS ${CONAN_PACKAGE_OPTIONS} systemc:shared=True systemc-cci:shared=True systemc:disable_virtual_bind=False)
    endif()

    conan_check()
    conan_cmake_configure(
        REQUIRES ${CONAN_PACKAGE_LIST}
        GENERATORS cmake_find_package
        OPTIONS ${CONAN_PACKAGE_OPTIONS}
    )

    conan_cmake_autodetect(settings)
    if(CMAKE_CXX_STANDARD)
        set(settings ${settings} compiler.cppstd=${CMAKE_CXX_STANDARD})
    endif()

    message("Conan settings are ${settings}")
    conan_cmake_install(
        PATH_OR_REFERENCE ${CMAKE_CURRENT_BINARY_DIR}
        BUILD missing
        SETTINGS ${settings}
    )
    set(Boost_NO_BOOST_CMAKE ON)

    set(SCC_LIB_ONLY ON)
    option(ENABLE_SCV "Enable the use of SCV" OFF)

    include(${scc_SOURCE_DIR}/cmake/SystemCPackage.cmake)

    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
    find_package(fmt REQUIRED)

    ####################################################################################################
    # cmake install settings
    set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/share")
    set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")
    set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif("${isSystemDir}" STREQUAL "-1")
    ####################################################################################################
    #if(NOT CMAKE_INSTALL_PREFIX)
    #    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/installed)
    #    message(WARNING "CMAKE_INSTALL_PREFIX not set. Default to ${CMAKE_CURRENT_BINARY_DIR}/installed")
    #endif()
    add_subdirectory(${scc_SOURCE_DIR} ${scc_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
####################################################################################################

find_package(elfio REQUIRED)
####################################################################################################

# rtl sources: set RTL_SOURCES, RTL_INCLUDE_DIRS, and VERILATE_ARGS in PARENT_SCOPE:
add_subdirectory(${VRTL_NAME})
message(${RTL_SRCS})

set(TOP_NAME ${TOP_NAME})

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.cpp "")

add_library(${PROJECT_NAME} OBJECT
    ${CMAKE_CURRENT_BINARY_DIR}/null.cpp
)

verilate(${PROJECT_NAME}
    PREFIX V${TOP_NAME}
    TOP_MODULE ${TOP_NAME}
    SYSTEMC
    INCLUDE_DIRS ${RTL_INCLUDE_DIRS}
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_NAME}/obj_dir
    VERILATOR_ARGS ${VERILATE_ARGS} +define+VRTLFI
    SOURCES ${RTL_SRCS}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${SystemC_LIBRARIES}
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${VERILATOR_ROOT}/include
    ${VERILATOR_ROOT}/include/vltstd
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_NAME}/obj_dir
)

get_target_property(SYSTEMC_INCLUDE_DIRS SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

file(GLOB CC_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_NAME}/obj_dir/*)

find_package(vrtlmod HINTS $ENV{VRTLMOD_ROOT} ${VRTLMOD_ROOT} REQUIRED)

set(VRTL_DIR ${VP}/obj_dir)

file( GLOB VRTL_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_DIR}/*.cpp)

set(VRTLMOD_OUTSOURCES ${VRTL_SRCS})

set(VRTLMOD_OUT_DIR "${VRTL_DIR}/vrtlmod/")

list(TRANSFORM VRTLMOD_OUTSOURCES REPLACE "/${VRTL_DIR}/" "/${VRTLMOD_OUT_DIR}/")

message("vrtlmod outsources" ${VRTLMOD_OUTSOURCES})

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}")
    set(MAKE_VRTLMOD On)
else()
    set(VARSET ${VRTLMOD_OUTSOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/${PROJECT_NAME}_vrtlmodapi.cpp)
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
        OUTPUT ${VRTLMOD_OUTSOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/${PROJECT_NAME}_vrtlmodapi.cpp
        VERBOSE
        SYSTEMC
        OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}
        WHITELIST_XML ${WHITELIST_XML}
        INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/${VRTL_DIR}/ ${VERILATOR_ROOT}/include ${VERILATOR_ROOT}/include/vltstd
        SYSTEMC_INCLUDE_DIRS ${SYSTEMC_INCLUDE_DIRS}
    )
endif()

add_library(${PROJECT_NAME}_vrtlmod OBJECT
    ${CMAKE_CURRENT_BINARY_DIR}/null.cpp
    ${VRTLMOD_OUTSOURCES}
    ${VERILATOR_ROOT}/include/verilated.cpp
    ${VERILATOR_ROOT}/include/verilated_dpi.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}/${PROJECT_NAME}_vrtlmodapi.cpp
)
target_link_libraries(${PROJECT_NAME}_vrtlmod PUBLIC
    ${SystemC_LIBRARIES}
)
target_include_directories(${PROJECT_NAME}_vrtlmod PUBLIC
    ${VERILATOR_ROOT}/include
    ${VERILATOR_ROOT}/include/vltstd
    ${CMAKE_CURRENT_BINARY_DIR}/${VRTLMOD_OUT_DIR}
)

message("VRTL_SRCS after vrtlmod: ${VRTLMOD_OUTSOURCES}")

add_executable(SC_TEST
    ${VRTL_NAME}/sc_test.cpp
)
target_link_libraries(SC_TEST
    ${PROJECT_NAME}
    scc
    elfio::elfio
    ${TGT_SPECIFIC_LIBS}
)
add_executable(SC_TEST_VRTLMOD
    ${VRTL_NAME}/sc_test.cpp
)
target_compile_options(SC_TEST_VRTLMOD PUBLIC -DVRTLMOD)
target_link_libraries(SC_TEST_VRTLMOD
    ${PROJECT_NAME}_vrtlmod
    scc
    elfio::elfio
    ${TGT_SPECIFIC_LIBS}
)
