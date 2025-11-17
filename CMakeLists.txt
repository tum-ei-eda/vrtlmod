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

####################################################################################################
# cmake project settings
cmake_minimum_required(VERSION 3.15)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

project(vrtlmod)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    message(WARNING "CMAKE_CXX_STANDARD not set. Default to ${CMAKE_CXX_STANDARD}")
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(WARNING "CMAKE_BUILD_TYPE not set. Default to ${CMAKE_BUILD_TYPE}")
endif()
####################################################################################################

####################################################################################################
# cmake project settings
option(BUILD_TESTING "Enable Tests" OFF)
include(CTest)

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." True)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif()
endif()
####################################################################################################

####################################################################################################
# cmake install settings
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/installed" CACHE PATH "default install path" FORCE)
endif()
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
# Verilator
find_package(VERILATOR REQUIRED)
if( ${verilator_VERSION_MAJOR} LESS 4 OR ${verilator_VERSION_MAJOR} GREATER 4)
    message(FATAL_ERROR "Verilator version \"${verilator_VERSION}\" is not required major 4.xxx" )
endif()
####################################################################################################
# LLVM
if(NOT DEFINED LLVM_DIR)  #try environment
    if(DEFINED ENV{LLVM_DIR})
        set(LLVM_DIR "$ENV{LLVM_DIR}")
    endif()
endif()
# 1st try only environment and LLVM_DIR
find_package(LLVM CONFIG NO_DEFAULT_PATH PATHS ${LLVM_DIR})
if(NOT LLVM_FOUND) # 2nd try with system and default paths
    find_package(LLVM CONFIG)
endif()
if(NOT LLVM_FOUND)
    message(STATUS "[${PROJECT_NAME}] Did not find LLVM... Skipping ${PROJECT_NAME} target.")
elseif((LLVM_FOUND AND LLVM_VERSION_MAJOR LESS 11))
    message(STATUS "[${PROJECT_NAME}] Could find LLVM at ${LLVM_DIR}, but version ${LLVM_PACKAGE_VERSION}<11... Skipping target.")
else()
    message(STATUS "[${PROJECT_NAME}] Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")
    string(REGEX MATCH "^[0-9]+" LLVM_VERSION_MAJOR "${LLVM_PACKAGE_VERSION}")
    math(EXPR LLVM_VERSION_MAJOR_NEXT "${LLVM_VERSION_MAJOR} + 1" OUTPUT_FORMAT DECIMAL)
 
    if(NOT Clang_FOUND) # 1st try only in relation to found LLVM
        find_package(Clang CONFIG NO_DEFAULT_PATH PATHS "${LLVM_DIR}/../clang" "${LLVM_DIR}")
    endif()
    if(NOT Clang_FOUND) # 2nd try with system and default paths
        find_package(Clang CONFIG)
    endif()
    if(NOT Clang_FOUND)
        message(STATUS "[${PROJECT_NAME}] Did not find matching Clang... Skipping target.")
        
    else()
        message(STATUS "[${PROJECT_NAME}] Found Clang ${Clang_PACKAGE_VERSION} at ${Clang_DIR}")

        add_compile_definitions(LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})

        if (LLVM_VERSION_MAJOR LESS 16)
            set(CLANG_INCLUDE_DIRS ${LLVM_LIBRARY_DIRS}/clang/${LLVM_VERSION}/include)
        else()
            set(CLANG_INCLUDE_DIRS ${LLVM_LIBRARY_DIRS}/clang/${LLVM_VERSION_MAJOR}/include)
        endif()

        if(LLVM_VERSION_MAJOR GREATER 15 AND CMAKE_CXX_STANDARD LESS 17)
            message(FATAL_ERROR "LLVM_VERSION_MAJOR[${LLVM_VERSION_MAJOR}] requires c++std>=17! Is ${CMAKE_CXX_STANDARD}")
        endif()
    endif()
endif()

if(CMAKE_CXX_STANDARD LESS_EQUAL 17)
find_package(Boost 1.70 REQUIRED
    COMPONENTS filesystem
)
message("Boost Version: ${Boost_VERSION}")
endif()

message("Using LLVM_DIR: ${LLVM_DIR}")
message("LLVM Version: ${LLVM_VERSION} at ${LLVM_INSTALL_PREFIX}")
message("LLVM INCLUDE DIRS: ${LLVM_INCLUDE_DIRS}")
message("Using Clang_DIR: ${Clang_DIR}")
message("Clang INCLUDE DIRS: ${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION}/include" "")

####################################################################################################

####################################################################################################
# Third Party/ non-package Libraries ###############################################################
add_subdirectory(third_party)
####################################################################################################

if(LLVM_FOUND AND Clang_FOUND AND verilator_FOUND)
    add_definitions(-DVRTLMOD_VERSION="0.10.0")
    add_definitions(-DVRTLMOD_VERILATOR_VERSION=${VRTLMOD_VERILATOR_VERSION})

    set(CLANG_LIBS clangTooling clangRewriteFrontend)
    llvm_map_components_to_libnames(LLVM_LIBS option support Support)

    # read default target dictionary from file
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/vapi/templates/targetdictionary.hpp CMAKE_TARGET_DICTIONARY_BODY)
    # replace default boilerplate
    string(REGEX REPLACE "^/\*.*#ifndef" "#ifndef" CMAKE_TARGET_DICTIONARY_BODY "${CMAKE_TARGET_DICTIONARY_BODY}")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/vapi/templates/targetdictionary_header.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/targetdictionary_header.cpp @ONLY)

    add_library(${PROJECT_NAME}-core OBJECT
        src/util/logging.cpp
        src/util/utility.cpp

        src/vrtlmod.cpp

        src/vapi/generator.cpp
        src/vapi/templates/vrtlmodapi_header.cpp
        src/vapi/templates/vrtlmodapi_source.cpp
        src/vapi/templates/vrtlmodapi_diffapi_source.cpp
        src/vapi/templates/vrtlmodapi_diffapi_header.cpp
        src/vapi/templates/vrtlmodapi_diffapi_comparefast_source.cpp
        src/vapi/templates/vrtlmodapi_diffapi_compare_source.cpp
        src/vapi/templates/vrtlmodapi_diffapi_compute_source.cpp
        src/vapi/templates/targetdictionary_python_module.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/targetdictionary_header.cpp

        src/core/consumer.cpp
        src/core/filecontext.cpp
        src/core/core.cpp
        src/core/types.cpp
        src/core/vrtlparse.cpp

        src/passes/elaborate.cpp
        src/passes/analyze.cpp
        src/passes/signaldeclrewrite.cpp
        src/passes/injectionrewrite.cpp
        src/passes/rewritemacrosaction.cpp
    )

    set(AUTO_INCLUDE_DIRS ${CLANG_INCLUDE_DIRS} ${VERILATOR_INCLUDE_DIRS})
    list(LENGTH AUTO_INCLUDE_DIRS AUTO_INCLUDE_DIRS_SIZE)
    list(TRANSFORM AUTO_INCLUDE_DIRS PREPEND "\"-I")
    list(TRANSFORM AUTO_INCLUDE_DIRS APPEND "\"")
    string(REPLACE ";" ",\n    " AUTO_INCLUDE_DIRS "${AUTO_INCLUDE_DIRS}")
    configure_file(include/vrtlmod/util/utility.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/vrtlmod/util/utility.hpp @ONLY)
    
    set(VRTLMOD_PRIVATE_LINK_LIBRARIES
        pugixml
        ${CLANG_LIBS}
        ${LLVM_LIBS}
    )
    if(CMAKE_CXX_STANDARD LESS_EQUAL 17)
        set(VRTLMOD_PRIVATE_LINK_LIBRARIES ${VRTLMOD_PRIVATE_LINK_LIBRARIES} Boost::filesystem)
    endif()
    target_link_libraries(${PROJECT_NAME}-core PRIVATE
        ${VRTLMOD_PRIVATE_LINK_LIBRARIES}
    )

    target_include_directories(${PROJECT_NAME}-core PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${LLVM_INCLUDE_DIRS}
    )

    target_include_directories(${PROJECT_NAME}-core PRIVATE
        ${VERILATOR_INCLUDE_DIRS}
    )

    add_executable(${PROJECT_NAME}-bin
        src/main.cpp
    )

    target_link_libraries(${PROJECT_NAME}-bin PRIVATE
        ${PROJECT_NAME}-core
    )

    if(NOT LLVM_ENABLE_RTTI) # set or unset by find_package(LLVM)
        message(WARNING "LLVM built without Runtime Type Information (RTTI) (-DLLVM_ENABLE_RTTI=Off)... Disable Runtime Type Information (-fno-rtti) for vrtlmod.")
        target_compile_options(${PROJECT_NAME}-core PUBLIC -fno-rtti)
        target_compile_options(${PROJECT_NAME}-bin PUBLIC -fno-rtti)
    endif()
    set_target_properties(${PROJECT_NAME}-bin
        PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
    )

endif()

option(BUILD_DOC "Build documentation" OFF)
find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxyfile.cmake.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doc/doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target( doc EXCLUDE_FROM_ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating ${PROJECT_NAME} API documentation with Doxygen"
        VERBATIM
    )

else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

########### TESTING #################
add_subdirectory(test)

install(TARGETS ${PROJECT_NAME}-bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vrtlmod-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/vrtlmod-config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vrtlmod-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake)
