PROJECT(vrtlmod)

CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

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
SET(TEST_BUILD ON CACHE STRING "Include Tests in build")
IF(CMAKE_TESTING_ENABLED AND TEST_BUILD)
  FIND_PACKAGE(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})

  SET(TDIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
  SET(TBDIR ${CMAKE_CURRENT_BINARY_DIR}/Testing)

  SET(RTL_SRCS ${RTL_PKG_FILES} ${RTL_SRC_FILES} ${RTL_UTIL_FILES})
  FILE(WRITE ${TBDIR}/null.cpp "int main(){}")
  ADD_EXECUTABLE(null
    ${TBDIR}/null.cpp
  )
  VERILATE(null
    TOP_MODULE fiapp
    DIRECTORY ${TBDIR}/obj_dir
    SOURCES ${TDIR}/fiapp/fiapp.sv
  )

  set(CIN
    ${TBDIR}/obj_dir/Vfiapp.cpp
    ${TBDIR}/obj_dir/Vfiapp__Syms.cpp
    ${TBDIR}/obj_dir/Vfiapp__Slow.cpp
  )
  string(REPLACE ".cpp" "_vrtlmod.cpp;" COUT ${CIN})

  message ("cout ${COUT}")

  SET(DUT_NAME fiapp)

  add_custom_command(
    OUTPUT ${COUT} ${TBDIR}/obj_dir/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp ${TBDIR}/obj_dir/vrtlmod/V${DUT_NAME}_vrtlmodapi.hpp ${TBDIR}/obj_dir/vrtlmod/targetdictionary/V${DUT_NAME}_td.hpp
    DEPENDS null ${PROJECT_NAME}
    COMMAND ${PROJECT_NAME} ARGS --regxml=${TDIR}/fiapp/regpicker.xml --out=${TBDIR}/obj_dir ${CIN} -- clang++ -I${TBDIR}/obj_dir/ -I${VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}
  )

  ADD_EXECUTABLE( ${PROJECT_NAME}-test
    ${COUT}
    ${TBDIR}/obj_dir/vrtlmod/V${DUT_NAME}_vrtlmodapi.cpp
    ${TDIR}/fiapp/fiapp_test.cpp
    ${VERILATOR_ROOT}/include/verilated.cpp
  )
  TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME}-test PUBLIC
    ${TBDIR}/obj_dir
    ${TBDIR}
    ${VERILATOR_ROOT}/include
  )

  ADD_TEST(NAME ${PROJECT_NAME}:build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
  )
  ADD_TEST(NAME ${PROJECT_NAME}:vrtlmod:test/fiapp
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}-test
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:vrtlmod:test/fiapp
    PROPERTIES DEPENDS ${PROJECT_NAME}:build
  )
  ADD_TEST(NAME ${PROJECT_NAME}:run:test/fiapp
    COMMAND ${PROJECT_NAME}-test
  )
  SET_TESTS_PROPERTIES(${PROJECT_NAME}:run:test/fiapp
    PROPERTIES DEPENDS ${PROJECT_NAME}:vrtlmod:test/fiapp
  )

ENDIF()
