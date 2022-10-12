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

cmake_minimum_required(VERSION 3.15)

enable_testing()
set(TEST_BUILD OFF CACHE STRING "Include Tests in build")
if(CMAKE_TESTING_ENABLED AND TEST_BUILD)

    include(ProcessorCount)
    ProcessorCount(NCORES)
    if(NOT NCORES EQUAL 0)
        set(CTEST_BUILD_FLAGS -j${NCORES})
        set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${NCORES})
        set(PARALLEL_BUILD "--parallel;${NCORES}")
    endif()

    add_test(NAME ${PROJECT_NAME}:build
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${PROJECT_NAME}
    )

    set(TDIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(TBDIR ${CMAKE_CURRENT_BINARY_DIR})
    set(CC_SUBDIR ${CMAKE_CURRENT_BINARY_DIR}/cc_obj_dir)
    set(RTL_SRCS ${RTL_PKG_FILES} ${RTL_SRC_FILES} ${RTL_UTIL_FILES})

    # dummies:
    file(WRITE ${TBDIR}/null.cpp "int main(){return(0);}")
    add_executable(null
        EXCLUDE_FROM_ALL
        ${TBDIR}/null.cpp
    )
    file(WRITE ${TBDIR}/sc_null.cpp "#include \"systemc.h\"\nint sc_main(int argc, char* argv[]){return(0);}")
    add_executable(sc-null
        EXCLUDE_FROM_ALL
        ${TBDIR}/sc_null.cpp
    )
    set_target_properties(null sc-null PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD})

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

    set(VERILATE_ARGS
        -sv
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

    # VERILATE to C++ VRTL
    set(DUT_NAME fiapp)
    verilate(null
        TOP_MODULE ${DUT_NAME}
        DIRECTORY ${CC_SUBDIR}
        SOURCES ${TDIR}/${DUT_NAME}/${DUT_NAME}.sv
        VERILATOR_ARGS -O0 --hierarchical ${VERILATE_ARGS}
    )

    file(GLOB CIN ${CC_SUBDIR}/*)
    string(REPLACE "${CC_SUBDIR}/" ";${CC_SUBDIR}/vrtlmod/" COUT ${CIN})

    message ("${PROJECT_NAME}-test-cc vrtlmod sources: ${COUT}")

    file(GLOB CC_SOURCES ${CC_SUBDIR}/*)

    add_custom_command(
        OUTPUT ${COUT} ${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp
        DEPENDS null ${PROJECT_NAME}-bin
        COMMAND ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ARGS --out=${CC_SUBDIR}/vrtlmod/ ${CIN} -v -- clang++ -v -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${CC_SUBDIR}/vrtlmod/ -I${VERILATOR_INCLUDE_DIRECTORY} -I${VERILATOR_INCLUDE_DIRECTORY}/vltstd -I${CLANG_INCLUDE_DIRS}
        COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --out=${CC_SUBDIR}/vrtlmod ${CIN} -v -- clang++ -v -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${CC_SUBDIR}/vrtlmod/ -I${VERILATOR_ROOT}/include -I${VERILATOR_ROOT}/include/vltstd -I${CLANG_INCLUDE_DIRS}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_library(${PROJECT_NAME}-test-cc_vrtlmod SHARED
        EXCLUDE_FROM_ALL
        ${COUT}
        ${VERILATOR_INCLUDE_DIRECTORY}/verilated.cpp
        ${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
    )
    target_include_directories(${PROJECT_NAME}-test-cc_vrtlmod PUBLIC
        ${CC_SUBDIR}/vrtlmod
        ${VERILATOR_INCLUDE_DIRECTORY}
        ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
    )

    add_executable( ${PROJECT_NAME}-test-cc
        EXCLUDE_FROM_ALL
        ${TDIR}/${DUT_NAME}/${DUT_NAME}_test.cpp
    )
    target_link_libraries(${PROJECT_NAME}-test-cc PUBLIC
        ${PROJECT_NAME}-test-cc_vrtlmod
    )

    # VERILATE to SystemC VRTL

    set(SC_SUBDIR ${CMAKE_CURRENT_BINARY_DIR}/sc_obj_dir)

    find_package(SystemCLanguage REQUIRED)

    get_target_property(SYSTEMC_INCLUDE_DIRS SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

    set(DUT_NAME fiapp)
    verilate(sc-null
        SYSTEMC
        TOP_MODULE ${DUT_NAME}
        DIRECTORY ${SC_SUBDIR}
        SOURCES ${TDIR}/${DUT_NAME}/${DUT_NAME}.sv
        VERILATOR_ARGS -O3 ${VERILATE_ARGS}
    )
    target_link_libraries(sc-null PUBLIC
        SystemC::systemc
    )
    file(GLOB SCIN ${SC_SUBDIR}/*)
    string(REPLACE "${SC_SUBDIR}/" ";${SC_SUBDIR}/vrtlmod/" SCOUT ${SCIN})

    message ("${PROJECT_NAME}-test-sc vrtlmod sources: ${SCOUT}")

    file(GLOB CC_SOURCES ${SC_SUBDIR}/*)

    add_custom_command(
        OUTPUT ${SCOUT} ${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp
        DEPENDS sc-null ${PROJECT_NAME}-bin
        COMMAND ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ARGS --systemc --out=${SC_SUBDIR}/vrtlmod ${SCIN} -v -- clang++ -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${SC_SUBDIR}/vrtlmod -I${VERILATOR_INCLUDE_DIRECTORY} -I${VERILATOR_INCLUDE_DIRECTORY}/vltstd -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIRS}
        COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --systemc --out=${SC_SUBDIR}/vrtlmod ${SCIN} -v -- clang++ -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${SC_SUBDIR}/vrtlmod -I${VERILATOR_ROOT}/include -I${VERILATOR_ROOT}/include/vltstd -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIRS}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_library(${PROJECT_NAME}-test-sc_vrtlmod SHARED
        EXCLUDE_FROM_ALL
        ${SCOUT}
        ${VERILATOR_INCLUDE_DIRECTORY}/verilated.cpp
        ${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
    )
    target_include_directories(${PROJECT_NAME}-test-sc_vrtlmod PUBLIC
        ${SC_SUBDIR}/vrtlmod
        ${VERILATOR_INCLUDE_DIRECTORY}
        ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
        ${SYSTEMC_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME}-test-sc_vrtlmod PUBLIC
        ${SystemC_LIBRARIES}
    )

    add_executable( ${PROJECT_NAME}-test-sc
        EXCLUDE_FROM_ALL
        ${TDIR}/${DUT_NAME}/sc_${DUT_NAME}_test.cpp
    )
    target_link_libraries(${PROJECT_NAME}-test-sc PUBLIC
        ${PROJECT_NAME}-test-sc_vrtlmod
    )

    ##########################################################################################################
    # Testing the CXX VRTL: ##################################################################################
    add_test(NAME ${PROJECT_NAME}:build:test/fiapp-cc
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${PROJECT_NAME}-test-cc
    )
    set_tests_properties(${PROJECT_NAME}:build:test/fiapp-cc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build
    )
    add_test(NAME ${PROJECT_NAME}:run:test/fiapp-cc
        COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-test-cc
    )
    set_tests_properties(${PROJECT_NAME}:run:test/fiapp-cc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build:test/fiapp-cc
    )
    ##########################################################################################################
    # Testing the SystemC VRTL: ##############################################################################
    add_test(NAME ${PROJECT_NAME}:build:test/fiapp-sc
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${PROJECT_NAME}-test-sc
    )
    set_tests_properties(${PROJECT_NAME}:build:test/fiapp-sc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build
    )
    add_test(NAME ${PROJECT_NAME}:run:test/fiapp-sc
        COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-test-sc
    )
    set_tests_properties(${PROJECT_NAME}:run:test/fiapp-sc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build:test/fiapp-sc
    )
    ##########################################################################################################
    # Testing the CMake config helper scripts: ###############################################################
    set(DIR_CMAKE_TEST ${CMAKE_CURRENT_BINARY_DIR}/cmaked)
    add_test(NAME ${PROJECT_NAME}:configure:test/fiapp-cmake
        COMMAND ${CMAKE_COMMAND} "-S${CMAKE_CURRENT_SOURCE_DIR}/fiapp" "-B${DIR_CMAKE_TEST}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DVRTLMOD_ROOT=${CMAKE_BINARY_DIR}"
            "-DSystemC_LIBRARIES=\'${SystemC_LIBRARIES}\'"
            "-DSYSTEMC_INCLUDE_DIRS=\'${SYSTEMC_INCLUDE_DIRS}\'"
    )
    set_tests_properties(${PROJECT_NAME}:configure:test/fiapp-cmake
        PROPERTIES
        DEPENDS ${PROJECT_NAME}:build
        ENVIRONMENT VERILATOR_ROOT=$ENV{VERILATOR_ROOT}
    )
    add_test(NAME ${PROJECT_NAME}:build:test/fiapp-cmake
        COMMAND ${CMAKE_COMMAND} --build ${DIR_CMAKE_TEST}
    )
    set_tests_properties(${PROJECT_NAME}:build:test/fiapp-cmake
        PROPERTIES DEPENDS ${PROJECT_NAME}:configure:test/fiapp-cmake
    )
    add_test(NAME ${PROJECT_NAME}:run:test/fiapp-cmake
        COMMAND
        ${DIR_CMAKE_TEST}/fiapp-cmake-test
    )
    set_tests_properties(${PROJECT_NAME}:run:test/fiapp-cmake
        PROPERTIES DEPENDS ${PROJECT_NAME}:build:test/fiapp-cmake
    )
endif()
