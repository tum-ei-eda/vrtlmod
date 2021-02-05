PROJECT(vrtlmod)

CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

IF(NOT LLVM_DIR) #try environment
  SET(LLVM_DIR $ENV{LLVM_DIR})
ENDIF()

IF(NOT Clang_DIR)
  SET(Clang_DIR ${LLVM_DIR}/../clang)
ENDIF()

SET(THISLIB "${PROJECT_NAME}_tool")

FIND_PACKAGE(Clang REQUIRED CONFIG)
FIND_PACKAGE(LLVM 9 REQUIRED CONFIG)
FIND_PACKAGE(LibXml2 REQUIRED CONFIG)
FIND_PACKAGE(Boost 1.72.0 REQUIRED
  COMPONENTS filesystem
)

MESSAGE("Boost Version: ${Boost_VERSION}")
MESSAGE("Using LLVM_DIR: ${LLVM_DIR}")
MESSAGE("LLVM Version: ${LLVM_VERSION} at ${LLVM_INSTALL_PREFIX}")
MESSAGE("LLVM INCLUDE DIRS: ${LLVM_INCLUDE_DIRS}")
MESSAGE("Using Clang_DIR: ${Clang_DIR}")
MESSAGE("Clang INCLUDE DIRS: ${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION}/include")

IF(LLVM_FOUND AND Clang_FOUND AND LibXml2_FOUND)

    SET(CLANG_INCLUDE_DIRS ${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION}/include)

    llvm_map_components_to_libnames(LLVM_LIBS
      option object profiledata support core mc bitreader MCParser BinaryFormat ExecutionEngine Target MCJIT X86CodeGen X86AsmParser Interpreter Support
    )

    ADD_EXECUTABLE(${PROJECT_NAME}
        src/main.cpp

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
        src/vapi/templates/targetdictionary_header.cpp
        src/vapi/templates/vrtlmodapi_header.cpp
        src/vapi/templates/vrtlmodapi_source.cpp
    )

    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC
        ${LLVM_LIBS}
        ${LIBXML2_LIBRARIES}
        ${Boost_LIBRARIES}
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

  FIND_PACKAGE(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT} REQUIRED)
  FIND_PACKAGE(regpicker HINTS $ENV{REGPICKER_ROOT} ${REGPICKER_ROOT} REQUIRED)

  SET(TDIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
  SET(TBDIR ${CMAKE_CURRENT_BINARY_DIR}/Testing)
  SET(CC_SUBDIR obj_dir)
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

  add_custom_target(${REGPICK_OUT})
  message("Using installed regpicker")
  regpick( TARGET ${REGPICK_OUT}
    BYBEHAVIOR
    #VERBOSE
    TOP_MODULE ${DUT_NAME}
    INPUT_DIRS ${TDIR}/${DUT_NAME}/
    CLOCK_PORT clk
    RESET_PORT reset
    VARGS ${VERILATE_ARGS}
    DIRECTORY ${TBDIR}/cc-regpick/
  )

  add_custom_command(
    OUTPUT ${COUT} ${TBDIR}/${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${TBDIR}/${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp ${TBDIR}/${CC_SUBDIR}/vrtlmod/targetdictionary/V${DUT_NAME}_td.hpp
    DEPENDS null ${PROJECT_NAME} ${REGPICK_OUT}
    COMMAND ${PROJECT_NAME} ARGS --regxml=${REGPICK_OUT} --out=${TBDIR}/${CC_SUBDIR} ${CIN} -- clang++ -I${TBDIR}/${CC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}
    COMMENT "executing vrtlmod: .. ${PROJECT_NAME} ARGS --regxml=${REGPICK_OUT} --out=${TBDIR}/${CC_SUBDIR} ${CIN} -- clang++ -I${TBDIR}/${CC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}"
  )

  ADD_EXECUTABLE( ${PROJECT_NAME}-test-cc
    ${COUT}
    ${TBDIR}/${CC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
    ${TDIR}/fiapp/fiapp_test.cpp
    ${VERILATOR_ROOT}/include/verilated.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME}-test-cc PUBLIC
    ${TBDIR}/${CC_SUBDIR}
    ${TBDIR}
    ${VERILATOR_ROOT}/include
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

  SET(SC_SUBDIR sc_obj_dir)

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
  VERILATE(sc-null
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

  SET(SC_REGPICK_OUT sc-${DUT_NAME}-pbb.xml)

  add_custom_target(${SC_REGPICK_OUT})
  message("Using installed regpicker")
  regpick( TARGET ${SC_REGPICK_OUT}
    BYBEHAVIOR
    #VERBOSE
    SYSTEMC
    TOP_MODULE ${DUT_NAME}
    INPUT_DIRS ${TDIR}/${DUT_NAME}/
    CLOCK_PORT clk
    RESET_PORT reset
    VARGS ${VERILATE_ARGS}
    DIRECTORY ${TBDIR}/sc-regpick/
  )

  get_target_property(SYSTEMC_INCLUDE_DIR SystemC::systemc INTERFACE_INCLUDE_DIRECTORIES)

  add_custom_command(
    OUTPUT ${SCOUT} ${TBDIR}/${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${TBDIR}/${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp ${TBDIR}/${SC_SUBDIR}/vrtlmod/targetdictionary/V${DUT_NAME}_td.hpp
    DEPENDS sc-null ${PROJECT_NAME} ${SC_REGPICK_OUT}
    COMMAND ${PROJECT_NAME} ARGS --systemc --regxml=${SC_REGPICK_OUT} --out=${TBDIR}/${SC_SUBDIR} ${SCIN} -- clang++ -I${TBDIR}/${SC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIR}
    COMMENT "executing vrtlmod: .. ${PROJECT_NAME} ARGS --systemc --regxml=${SC_REGPICK_OUT} --out=${TBDIR}/${SC_SUBDIR} ${SCIN} -- clang++ -I${TBDIR}/${SC_SUBDIR}/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIR}"
  )

  ADD_EXECUTABLE( ${PROJECT_NAME}-test-sc
    ${SCOUT}
    ${TBDIR}/${SC_SUBDIR}/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
    ${TDIR}/fiapp/sc_fiapp_test.cpp
    ${VERILATOR_ROOT}/include/verilated.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME}-test-sc PUBLIC
    ${TBDIR}/${SC_SUBDIR}
    ${TBDIR}
    ${VERILATOR_ROOT}/include
  )
  TARGET_LINK_LIBRARIES(${PROJECT_NAME}-test-sc PUBLIC
      SystemC::systemc
  )
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
