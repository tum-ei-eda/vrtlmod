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

CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/eda-conan)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})


include(ConanUser)

PROJECT(vrtlmod)

option(CMAKE_CXX_STANDARD_REQUIRED "Set CXX_STANDARD!" ON)

# ENVIRONMENT ##################################################
IF(NOT LLVM_DIR) #try environment
  SET(LLVM_DIR $ENV{LLVM_DIR})
ENDIF()

IF(NOT Clang_DIR)
  SET(Clang_DIR ${LLVM_DIR}/../clang)
ENDIF()

SET(THISLIB "${PROJECT_NAME}_tool")

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." True)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

set(CONAN_OPTIONS
  boost:shared=True
  boost:header_only=False
  systemc:shared=True
)

set(CONAN_PACKAGE_LIST
  libxml2/[>2.9.10]
  boost/[>1.70]
  systemc/2.3.3@eda/stable
)
set(CONAN_PACKAGE_OPTIONS ${CONAN_OPTIONS})

eda_conan_check(${CMAKE_CURRENT_SOURCE_DIR}/cmake/eda-conan)
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

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

set(Boost_NO_BOOST_CMAKE ON)

FIND_PACKAGE(Clang REQUIRED CONFIG
  NO_DEFAULT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  PATHS ${LLVM_DIR}/lib/cmake/clang ${Clang_DIR})

FIND_PACKAGE(LLVM 9 REQUIRED CONFIG
  NO_DEFAULT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  PATHS ${LLVM_DIR}/lib/cmake/llvm)

if(NOT LLVM_ENABLE_RTTI)
  message(FATAL_ERROR "LLVM built with RTTI required (-DLLVM_ENABLE_RTTI=On)")
endif()

FIND_PACKAGE(LibXml2 REQUIRED)

find_package(Boost REQUIRED
  COMPONENTS filesystem
)

MESSAGE("Boost Version: ${Boost_VERSION}")
MESSAGE("Using LLVM_DIR: ${LLVM_DIR}")
MESSAGE("LLVM Version: ${LLVM_VERSION} at ${LLVM_INSTALL_PREFIX}")
MESSAGE("LLVM INCLUDE DIRS: ${LLVM_INCLUDE_DIRS}")
MESSAGE("Using Clang_DIR: ${Clang_DIR}")
MESSAGE("Clang INCLUDE DIRS: ${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION}/include")
################################################################

# SETTINGS #####################################################
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" IS_SYSTEMDIR)
if("${IS_SYSTEMDIR}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${IS_SYSTEMDIR}" STREQUAL "-1")
################################################################

IF(LLVM_FOUND AND Clang_FOUND AND LibXml2_FOUND)

    SET(CLANG_INCLUDE_DIRS ${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION}/include)

    llvm_map_components_to_libnames(LLVM_LIBS
      option object profiledata support core mc bitreader MCParser BinaryFormat ExecutionEngine Target MCJIT X86CodeGen X86AsmParser Interpreter Support
    )

    add_library(${PROJECT_NAME} SHARED
        src/transform/consumer.cpp
        src/transform/filecontext.cpp
        src/transform/extendedmatchers.cpp
        src/transform/injectionrewriter.cpp
        src/transform/rewrite/rewritemacrosaction.cpp

        src/util/logging.cpp
        src/util/system.cpp

        src/vapi/generator.cpp
        src/vapi/xmlhelper.cpp
        src/vapi/target.cpp

        src/vapi/templates/vrtlmodapi_header.cpp
        src/vapi/templates/vrtlmodapi_source.cpp
    )

    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC
        ${LLVM_LIBS}
        ${LIBXML2_LIBRARIES}
        Boost::filesystem
        clangFrontend
        clangCodeGen
        clangTooling
        clangARCMigrate
        clangAST
        clangASTMatchers
        clangAnalysis
        clangBasic
        clangCrossTU
        clangDependencyScanning
        clangDirectoryWatcher
        clangDriver
        clangDynamicASTMatchers
        clangEdit
        clangFormat
        clangFrontendTool
        clangHandleCXX
        clangHandleLLVM
        clangIndex
        clangLex
        clangParse
        clangRewrite
        clangRewriteFrontend
        clangSema
        clangSerialization
        clangStaticAnalyzerCheckers
        clangStaticAnalyzerCore
        clangStaticAnalyzerFrontend
        clangToolingASTDiff
        clangToolingCore
        clangToolingInclusions
        clangToolingRefactoring
        clangToolingSyntax
    )
    TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
        include
        #/usr/include/libxml2
        ${LIBXML2_INCLUDE_DIRS}
        ${LLVM_INCLUDE_DIRS}
        #${CLANG_INCLUDE_DIRS}
    )

    add_executable(${PROJECT_NAME}-bin
        src/main.cpp
    )
    target_link_libraries(${PROJECT_NAME}-bin PUBLIC
        ${PROJECT_NAME}
    )
    set_target_properties(${PROJECT_NAME}-bin
        PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
    )

ENDIF()

option(BUILD_DOC "Build documentation" OFF)
find_package(Doxygen)

if(BUILD_DOC)
  if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxyfile.cmake.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doc/doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    add_custom_target( doc ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating ${PROJECT_NAME} API documentation with Doxygen"
      VERBATIM )

  else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
  endif (DOXYGEN_FOUND)
endif(BUILD_DOC)

########### TESTING #################

ENABLE_TESTING()
SET(TEST_BUILD OFF CACHE STRING "Include Tests in build")
IF(CMAKE_TESTING_ENABLED AND TEST_BUILD)
  add_subdirectory(test)
ENDIF()

########### INSTALL #################
install(TARGETS ${PROJECT_NAME}-bin ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vrtlmod-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/vrtlmod-config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vrtlmod-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX})
