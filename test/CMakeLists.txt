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

########### TESTING #################

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

    find_package(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT} REQUIRED)

    # VERILATE to C++ VRTL
    set(DUT_NAME fiapp)
    verilate(null
        TOP_MODULE ${DUT_NAME}
        DIRECTORY ${CC_SUBDIR}
        SOURCES ${TDIR}/${DUT_NAME}/${DUT_NAME}.sv
        VERILATOR_ARGS -O0 --hierarchical
    )

    file(GLOB CIN ${CC_SUBDIR}/*)
    string(REPLACE "${CC_SUBDIR}/" ";${CC_SUBDIR}/vrtlmod/" COUT ${CIN})

    message ("New sources: ${COUT}")

    set(REGPICK_OUT ${DUT_NAME}-pbb.xml)

    file(GLOB CC_SOURCES ${CC_SUBDIR}/*)

    add_custom_target(${REGPICK_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_custom_command(
        OUTPUT ${COUT} ${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp
        DEPENDS null ${PROJECT_NAME}-bin ${REGPICK_OUT}
        COMMAND ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ARGS --out=${CC_SUBDIR}/vrtlmod/ ${CIN} -v -- clang++ -v -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${CC_SUBDIR}/vrtlmod/ -I${VERILATOR_ROOT}/include -I${VERILATOR_ROOT}/include/vltstd -I${CLANG_INCLUDE_DIRS}
        COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --out=${CC_SUBDIR}/vrtlmod ${CIN} -v -- clang++ -v -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${CC_SUBDIR}/vrtlmod/ -I${VERILATOR_ROOT}/include -I${VERILATOR_ROOT}/include/vltstd -I${CLANG_INCLUDE_DIRS}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_executable( ${PROJECT_NAME}-test-cc
        EXCLUDE_FROM_ALL
        ${COUT}
        ${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
        ${TDIR}/${DUT_NAME}/${DUT_NAME}_test.cpp
        ${VERILATOR_ROOT}/include/verilated.cpp
    )
    target_include_directories( ${PROJECT_NAME}-test-cc PUBLIC
        ${CC_SUBDIR}/vrtlmod
        ${TBDIR}
        ${VERILATOR_ROOT}/include
        ${PROJECT_SOURCE_DIR}/include
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
        VERILATOR_ARGS -O3
    )
    target_link_libraries(sc-null PUBLIC
        SystemC::systemc
    )
    file(GLOB SCIN ${SC_SUBDIR}/*)
    string(REPLACE "${SC_SUBDIR}/" ";${SC_SUBDIR}/vrtlmod/" SCOUT ${SCIN})

    message ("New sources: ${SCOUT}")

    file(GLOB CC_SOURCES ${SC_SUBDIR}/*)

    set(SC_REGPICK_OUT sc-${DUT_NAME}-pbb.xml)
    add_custom_target(${SC_REGPICK_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_custom_command(
        OUTPUT ${SCOUT} ${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp
        DEPENDS sc-null ${PROJECT_NAME}-bin ${SC_REGPICK_OUT}
        COMMAND ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ARGS --systemc --out=${SC_SUBDIR}/vrtlmod ${SCIN} -v -- clang++ -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${SC_SUBDIR}/vrtlmod -I${VERILATOR_ROOT}/include -I${VERILATOR_ROOT}/include/vltstd -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIRS}
        COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --systemc --out=${SC_SUBDIR}/vrtlmod ${SCIN} -v -- clang++ -Wno-null-character -xc++ -stdlib=libstdc++ -std=c++${CMAKE_CXX_STANDARD} -I${SC_SUBDIR}/vrtlmod -I${VERILATOR_ROOT}/include -I${VERILATOR_ROOT}/include/vltstd -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIRS}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_executable( ${PROJECT_NAME}-test-sc
        EXCLUDE_FROM_ALL
        ${SCOUT}
        ${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
        ${TDIR}/${DUT_NAME}/sc_${DUT_NAME}_test.cpp
        ${VERILATOR_ROOT}/include/verilated.cpp
    )
    target_include_directories( ${PROJECT_NAME}-test-sc PUBLIC
        ${SC_SUBDIR}/vrtlmod
        ${TBDIR}
        ${VERILATOR_ROOT}/include
        ${PROJECT_SOURCE_DIR}/include
    )
    target_link_libraries(${PROJECT_NAME}-test-sc PUBLIC
        SystemC::systemc
    )

    add_test(NAME ${PROJECT_NAME}:build:test/fiapp-cc
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${PROJECT_NAME}-test-cc
    )
    set_tests_properties(${PROJECT_NAME}:build:test/fiapp-cc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build
    )

    add_test(NAME ${PROJECT_NAME}:build:test/fiapp-sc
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${PROJECT_NAME}-test-sc
    )
    set_tests_properties(${PROJECT_NAME}:build:test/fiapp-sc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build
    )

    add_test(NAME ${PROJECT_NAME}:run:test/fiapp-cc
        COMMAND ${PROJECT_NAME}-test-cc
    )
    set_tests_properties(${PROJECT_NAME}:run:test/fiapp-cc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build:test/fiapp-cc
    )
    add_test(NAME ${PROJECT_NAME}:run:test/fiapp-sc
        COMMAND ${PROJECT_NAME}-test-sc
    )
    set_tests_properties(${PROJECT_NAME}:run:test/fiapp-sc
        PROPERTIES DEPENDS ${PROJECT_NAME}:build:test/fiapp-sc
    )
endif()
