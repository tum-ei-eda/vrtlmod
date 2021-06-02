PROJECT(vrtlmod)

CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

########### TESTING #################

ENABLE_TESTING()
SET(TEST_BUILD OFF CACHE STRING "Include Tests in build")
IF(CMAKE_TESTING_ENABLED AND TEST_BUILD)

  FIND_PACKAGE(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT} REQUIRED)
  FIND_PACKAGE(regpicker HINTS $ENV{REGPICKER_ROOT} ${REGPICKER_ROOT} REQUIRED)

  SET(TDIR ${CMAKE_CURRENT_SOURCE_DIR})
  SET(TBDIR ${CMAKE_CURRENT_BINARY_DIR}/Testing)
  SET(CC_SUBDIR ${CMAKE_CURRENT_BINARY_DIR}/cc_obj_dir)
  SET(RTL_SRCS ${RTL_PKG_FILES} ${RTL_SRC_FILES} ${RTL_UTIL_FILES})
  FILE(WRITE ${TBDIR}/null.cpp "int main(){return(0);}")
  ADD_EXECUTABLE(null
    ${TBDIR}/null.cpp
  )
  # VERILATE to C++ VRTL
  SET(DUT_NAME fiapp)
  VERILATE(null
    TOP_MODULE ${DUT_NAME}
    DIRECTORY ${TBDIR}/${CC_SUBDIR}
    SOURCES ${TDIR}/${DUT_NAME}/${DUT_NAME}.sv
  )

  set(CIN
    ${TBDIR}/${CC_SUBDIR}/V${DUT_NAME}.cpp
    ${TBDIR}/${CC_SUBDIR}/V${DUT_NAME}__Syms.cpp
    ${TBDIR}/${CC_SUBDIR}/V${DUT_NAME}__Slow.cpp
  )
  string(REPLACE ".cpp" "_vrtlmod.cpp;" COUT ${CIN})

  message ("cout ${COUT}")

  SET(REGPICK_OUT ${DUT_NAME}-pbb.xml)

  file(GLOB CC_SOURCES ${TBDIR}/${CC_SUBDIR}/*)
  
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
    OUTPUT ${COUT} ${TBDIR}/${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${TBDIR}/${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp ${TBDIR}/${CC_SUBDIR}/vrtlmod/targetdictionary/V${DUT_NAME}_td.hpp
    DEPENDS null ${PROJECT_NAME} ${REGPICK_OUT}
    COMMAND ${PROJECT_NAME} ARGS --regxml=${REGPICK_OUT} --out=${TBDIR}/${CC_SUBDIR} ${CIN} -- clang++ -I${TBDIR}/${CC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}
    COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --regxml=${REGPICK_OUT} --out=${TBDIR}/${CC_SUBDIR} ${CIN} -- clang++ -I${TBDIR}/${CC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}"
  )

  ADD_EXECUTABLE( ${PROJECT_NAME}-test-cc
    ${COUT}
    ${TBDIR}/${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
    ${TDIR}/${DUT_NAME}/${DUT_NAME}_test.cpp
    ${VERILATOR_ROOT}/include/verilated.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME}-test-cc PUBLIC
    ${TBDIR}/${CC_SUBDIR}
    ${TBDIR}
    ${VERILATOR_ROOT}/include
    ${TDIR}/vrtl-fi-framework/include
  )

  ADD_TEST(NAME ${PROJECT_NAME}:build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
  )
  ADD_TEST(NAME ${PROJECT_NAME}:vrtlmod:test/fiapp-cc
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}-test-cc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:vrtlmod:test/fiapp-cc
    PROPERTIES DEPENDS ${PROJECT_NAME}:build
  )
  ADD_TEST(NAME ${PROJECT_NAME}:run:test/fiapp-cc
    COMMAND ${PROJECT_NAME}-test-cc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:run:test/fiapp-cc
    PROPERTIES DEPENDS ${PROJECT_NAME}:vrtlmod:test/fiapp-cc
  )

  # VERILATE to SystemC VRTL

  SET(SC_SUBDIR ${CMAKE_CURRENT_BINARY_DIR}/sc_obj_dir)

  FIND_PACKAGE(SystemCLanguage CONFIG REQUIRED)
  set (CMAKE_CXX_STANDARD ${SystemC_CXX_STANDARD} CACHE STRING
    "C++ standard to build all targets. Supported values are 98, 11, and 14.")
  set (CMAKE_CXX_STANDARD_REQUIRED ${SystemC_CXX_STANDARD_REQUIRED} CACHE BOOL
    "The CMAKE_CXX_STANDARD selected C++ standard is a requirement.")

  FILE(WRITE ${TBDIR}/sc_null.cpp "#include \"systemc.h\"\nint sc_main(int argc, char* argv[]){return(0);}")
  ADD_EXECUTABLE(sc-null
    ${TBDIR}/sc_null.cpp
  )
  SET(DUT_NAME fiapp)
  verilate(sc-null
    SYSTEMC
    TOP_MODULE ${DUT_NAME}
    DIRECTORY ${TBDIR}/${SC_SUBDIR}
    SOURCES ${TDIR}/${DUT_NAME}/${DUT_NAME}.sv
  )
  TARGET_LINK_LIBRARIES(sc-null PUBLIC
      SystemC::systemc
  )
  set(SCIN
    ${TBDIR}/${SC_SUBDIR}/V${DUT_NAME}.cpp
    ${TBDIR}/${SC_SUBDIR}/V${DUT_NAME}__Syms.cpp
    ${TBDIR}/${SC_SUBDIR}/V${DUT_NAME}__Slow.cpp
  )
  string(REPLACE ".cpp" "_vrtlmod.cpp;" SCOUT ${SCIN})

  message ("cout ${SCOUT}")

  file(GLOB CC_SOURCES ${TBDIR}/${SC_SUBDIR}/*)
  
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
  )

  get_target_property(SYSTEMC_INCLUDE_DIR SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

  add_custom_command(
    OUTPUT ${SCOUT} ${TBDIR}/${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${TBDIR}/${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp ${TBDIR}/${SC_SUBDIR}/vrtlmod/targetdictionary/V${DUT_NAME}_td.hpp
    DEPENDS sc-null ${PROJECT_NAME} ${SC_REGPICK_OUT}
    COMMAND ${PROJECT_NAME} ARGS --systemc --regxml=${SC_REGPICK_OUT} --out=${TBDIR}/${SC_SUBDIR} ${SCIN} -- clang++ -I${TBDIR}/${SC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIR}
    COMMENT "executing vrtlmod: .. ${PROJECT_NAME} --systemc --regxml=${SC_REGPICK_OUT} --out=${TBDIR}/${SC_SUBDIR} ${SCIN} -- clang++ -I${TBDIR}/${SC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIR}"
  )

  ADD_EXECUTABLE( ${PROJECT_NAME}-test-sc
    ${SCOUT}
    ${TBDIR}/${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
    ${TDIR}/${DUT_NAME}/sc_${DUT_NAME}_test.cpp
    ${VERILATOR_ROOT}/include/verilated.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME}-test-sc PUBLIC
    ${TBDIR}/${SC_SUBDIR}
    ${TBDIR}
    ${VERILATOR_ROOT}/include
    ${TDIR}/vrtl-fi-framework/include
  )
  TARGET_LINK_LIBRARIES(${PROJECT_NAME}-test-sc PUBLIC
      SystemC::systemc
  )
  SET_PROPERTY(TARGET ${PROJECT_NAME}-test-sc PROPERTY CXX_STANDARD ${SystemC_CXX_STANDARD})

  ADD_TEST(NAME ${PROJECT_NAME}:vrtlmod:test/fiapp-sc
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}-test-sc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:vrtlmod:test/fiapp-sc
    PROPERTIES DEPENDS ${PROJECT_NAME}:build
  )
  ADD_TEST(NAME ${PROJECT_NAME}:run:test/fiapp-sc
    COMMAND ${PROJECT_NAME}-test-sc
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:run:test/fiapp-sc
    PROPERTIES DEPENDS ${PROJECT_NAME}:vrtlmod:test/fiapp-sc
  )

ENDIF()
