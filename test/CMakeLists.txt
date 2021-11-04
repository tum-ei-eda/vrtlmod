#PROJECT(vrtlmod)

CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

########### TESTING #################

ENABLE_TESTING()
SET(TEST_BUILD OFF CACHE STRING "Include Tests in build")
IF(CMAKE_TESTING_ENABLED AND TEST_BUILD)

  add_subdirectory(vrtl-fi-framework)

  SET(TDIR ${CMAKE_CURRENT_SOURCE_DIR})
  SET(TBDIR ${CMAKE_CURRENT_BINARY_DIR})
  SET(CC_SUBDIR ${CMAKE_CURRENT_BINARY_DIR}/cc_obj_dir)
  SET(RTL_SRCS ${RTL_PKG_FILES} ${RTL_SRC_FILES} ${RTL_UTIL_FILES})

  # dummies:
  FILE(WRITE ${TBDIR}/null.cpp "int main(){return(0);}")
  ADD_EXECUTABLE(null
    EXCLUDE_FROM_ALL
    ${TBDIR}/null.cpp
  )
  FILE(WRITE ${TBDIR}/sc_null.cpp "#include \"systemc.h\"\nint sc_main(int argc, char* argv[]){return(0);}")
  ADD_EXECUTABLE(sc-null
    EXCLUDE_FROM_ALL
    ${TBDIR}/sc_null.cpp
  )
  set_target_properties(null sc-null PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD})

  FIND_PACKAGE(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT} REQUIRED)
  FIND_PACKAGE(regpicker HINTS $ENV{REGPICKER_ROOT} ${REGPICKER_ROOT} REQUIRED)

  # VERILATE to C++ VRTL
  SET(DUT_NAME fiapp)
  verilate(null
    TOP_MODULE ${DUT_NAME}
    DIRECTORY ${CC_SUBDIR}
    SOURCES ${TDIR}/${DUT_NAME}/${DUT_NAME}.sv
    VERILATOR_ARGS -O3
  )

  set(CIN
    ${CC_SUBDIR}/V${DUT_NAME}.cpp
    ${CC_SUBDIR}/V${DUT_NAME}__Syms.cpp
    ${CC_SUBDIR}/V${DUT_NAME}__Slow.cpp
  )
  string(REPLACE "${CC_SUBDIR}/" "${CC_SUBDIR}/vrtlmod/" COUT ${CIN})
  string(REPLACE ".cpp" "_vrtlmod.cpp;" COUT ${COUT})

  message ("New sources: ${COUT}")

  SET(REGPICK_OUT ${DUT_NAME}-pbb.xml)

  file(GLOB CC_SOURCES ${CC_SUBDIR}/*)

  add_custom_target(${REGPICK_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${REGPICK_OUT}
  )
  message("Using system regpicker at ${REGPICKER_ROOT}")
  regpick( TARGET ${CMAKE_CURRENT_BINARY_DIR}/${REGPICK_OUT}
    BYDESIGN
    TOP_MODULE ${DUT_NAME}
    SOURCES ${CC_SOURCES}
    VARGS ${VERILATE_ARGS}
    NETLIST ${TDIR}/${DUT_NAME}/V${DUT_NAME}.xml
  )

  add_custom_command(
    OUTPUT ${COUT} ${CC_SUBDIR}/vrtlmod/vrtlmodapi/V${DUT_NAME}_vrtlmodapi.cpp ${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp ${CC_SUBDIR}/vrtlmod/targetdictionary/V${DUT_NAME}_td.hpp
    DEPENDS null ${PROJECT_NAME}-bin ${REGPICK_OUT}
    COMMAND ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ARGS --regxml=${REGPICK_OUT} --out=${CC_SUBDIR}/vrtlmod/ ${CIN} -- clang++ -I${CC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}
    COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --regxml=${REGPICK_OUT} --out=${CC_SUBDIR}/vrtlmod ${CIN} -- clang++ -I${CC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  ADD_EXECUTABLE( ${PROJECT_NAME}-test-cc
    EXCLUDE_FROM_ALL
    ${COUT}
    ${CC_SUBDIR}/vrtlmod/vrtlmodapi/V${DUT_NAME}_vrtlmodapi.cpp
    ${TDIR}/${DUT_NAME}/${DUT_NAME}_test.cpp
    ${VERILATOR_ROOT}/include/verilated.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME}-test-cc PUBLIC
    ${CC_SUBDIR}
    ${CC_SUBDIR}/vrtlmod/
    ${TBDIR}
    ${VERILATOR_ROOT}/include
  )
  TARGET_LINK_LIBRARIES(${PROJECT_NAME}-test-cc PUBLIC
    vrtl-fi
  )
  ADD_TEST(NAME ${PROJECT_NAME}:build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
  )
  ADD_TEST(NAME ${PROJECT_NAME}:build:test/fiapp-cc
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}-test-cc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:build:test/fiapp-cc
    PROPERTIES DEPENDS ${PROJECT_NAME}:build
  )
  ADD_TEST(NAME ${PROJECT_NAME}:run:test/fiapp-cc
    COMMAND ${PROJECT_NAME}-test-cc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:run:test/fiapp-cc
    PROPERTIES DEPENDS ${PROJECT_NAME}:build:test/fiapp-cc
  )

  # VERILATE to SystemC VRTL

  SET(SC_SUBDIR ${CMAKE_CURRENT_BINARY_DIR}/sc_obj_dir)

  FIND_PACKAGE(SystemCLanguage REQUIRED)
  set (CMAKE_CXX_STANDARD ${SystemC_CXX_STANDARD} CACHE STRING
    "C++ standard to build all targets. Supported values are 98, 11, and 14.")
  set (CMAKE_CXX_STANDARD_REQUIRED ${SystemC_CXX_STANDARD_REQUIRED} CACHE BOOL
    "The CMAKE_CXX_STANDARD selected C++ standard is a requirement.")

  SET(DUT_NAME fiapp)
  verilate(sc-null
    SYSTEMC
    TOP_MODULE ${DUT_NAME}
    DIRECTORY ${SC_SUBDIR}
    SOURCES ${TDIR}/${DUT_NAME}/${DUT_NAME}.sv
    VERILATOR_ARGS -O3
  )
  TARGET_LINK_LIBRARIES(sc-null PUBLIC
      SystemC::systemc
  )
  set(SCIN
    ${SC_SUBDIR}/V${DUT_NAME}.cpp
    ${SC_SUBDIR}/V${DUT_NAME}__Syms.cpp
    ${SC_SUBDIR}/V${DUT_NAME}__Slow.cpp
  )
  string(REPLACE "${SC_SUBDIR}/" "${SC_SUBDIR}/vrtlmod/" SCOUT ${SCIN})
  string(REPLACE ".cpp" "_vrtlmod.cpp;" SCOUT ${SCOUT})

  message ("New sources: ${SCOUT}")

  file(GLOB CC_SOURCES ${SC_SUBDIR}/*)

  SET(SC_REGPICK_OUT sc-${DUT_NAME}-pbb.xml)
  add_custom_target(${SC_REGPICK_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SC_REGPICK_OUT}
  )
  message("Using system regpicker at ${REGPICKER_ROOT}")
  regpick( TARGET ${CMAKE_CURRENT_BINARY_DIR}/${SC_REGPICK_OUT}
    SYSTEMC
    BYDESIGN
    TOP_MODULE ${DUT_NAME}
    SOURCES ${CC_SOURCES}
    VARGS ${VERILATE_ARGS}
    NETLIST ${TDIR}/${DUT_NAME}/V${DUT_NAME}.xml
    CC_INCLUDE_DIRS ${SystemC_INCLUDE_DIRS}
  )

  get_target_property(SYSTEMC_INCLUDE_DIR SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

  add_custom_command(
    OUTPUT ${SCOUT} ${SC_SUBDIR}/vrtlmod/vrtlmodapi/V${DUT_NAME}_vrtlmodapi.cpp ${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp ${SC_SUBDIR}/vrtlmod/targetdictionary/V${DUT_NAME}_td.hpp
    DEPENDS sc-null ${PROJECT_NAME}-bin ${SC_REGPICK_OUT}
    COMMAND ${PROJECT_BINARY_DIR}/${PROJECT_NAME} ARGS --systemc --regxml=${SC_REGPICK_OUT} --out=${SC_SUBDIR}/vrtlmod ${SCIN} -- clang++ -I${SC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIR}
    COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --systemc --regxml=${SC_REGPICK_OUT} --out=${SC_SUBDIR}/vrtlmod ${SCIN} -- clang++ -I${SC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIR}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  ADD_EXECUTABLE( ${PROJECT_NAME}-test-sc
    EXCLUDE_FROM_ALL
    ${SCOUT}
    ${SC_SUBDIR}/vrtlmod/vrtlmodapi/V${DUT_NAME}_vrtlmodapi.cpp
    ${TDIR}/${DUT_NAME}/sc_${DUT_NAME}_test.cpp
    ${VERILATOR_ROOT}/include/verilated.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME}-test-sc PUBLIC
    ${SC_SUBDIR}
    ${SC_SUBDIR}/vrtlmod/
    ${TBDIR}
    ${VERILATOR_ROOT}/include
  )
  TARGET_LINK_LIBRARIES(${PROJECT_NAME}-test-sc PUBLIC
    SystemC::systemc
    vrtl-fi
  )

  ADD_TEST(NAME ${PROJECT_NAME}:build:test/fiapp-sc
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}-test-sc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:build:test/fiapp-sc
    PROPERTIES DEPENDS ${PROJECT_NAME}:build
  )
  ADD_TEST(NAME ${PROJECT_NAME}:run:test/fiapp-sc
    COMMAND ${PROJECT_NAME}-test-sc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:run:test/fiapp-sc
    PROPERTIES DEPENDS ${PROJECT_NAME}:build:test/fiapp-sc
  )

ENDIF()
