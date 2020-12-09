PROJECT(vrtlmod)

CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

#IF(NOT ENV{LLVM_DIR})
#    SET(ENV{LLVM_DIR} "/usr/local/research/projects/SystemDesign/tools/llvm/9.0.0")
#ENDIF()

#SET(LLVM_DIR ENV{LLVM_DIR})
#SET(Clang_DIR ENV{CLANG_DIR})

#SET(THISDIR ${CMAKE_SOURCE_DIR}/src/vrtlmod)

IF(NOT Clang_DIR)
SET(Clang_DIR ${LLVM_DIR}/../clang)
ENDIF()

SET(THISLIB "${PROJECT_NAME}_tool")

FIND_PACKAGE(Clang REQUIRED CONFIG)
FIND_PACKAGE(LLVM 9 REQUIRED CONFIG)
FIND_PACKAGE(LibXml2 REQUIRED CONFIG)
FIND_PACKAGE(Boost REQUIRED
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

        add_custom_target( doc_doxygen ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating ${PROJECT_NAME} API documentation with Doxygen"
                VERBATIM )

        add_custom_target( doc_pdf ALL
                DEPENDS doc_doxygen
                COMMAND make && cp ${CMAKE_CURRENT_BINARY_DIR}/doc/latex/refman.pdf ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}.pdf
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/latex
                COMMENT "Generating ${PROJECT_NAME} API pdf"
                VERBATIM )

    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif(BUILD_DOC)

########### TESTING #################

ENABLE_TESTING()
SET(TEST_BUILD ON CACHE STRING "Include Tests in build")
IF(CMAKE_TESTING_ENABLED AND TEST_BUILD)
	FIND_PACKAGE(GTest)
	IF(NOT GTest_FOUND)
		INCLUDE(FetchContent)
		SET(FETCHCONTENT_QUIET FALSE)
		FetchContent_Declare(gtest_srcs
			GIT_REPOSITORY https://github.com/google/googletest.git
			GIT_PROGRESS TRUE
		)
		IF(NOT gtest_srcs_POPULATED)
			FetchContent_Populate(gtest_srcs)
			FetchContent_GetProperties(gtest_srcs)
			SET(GTest_LOCATION ${gtest_srcs_SOURCE_DIR})
			SET(INSTALL_GTest OFF CACHE PATH "Do not install GTEST - build gtest locally.")
			ADD_SUBDIRECTORY(${GTest_LOCATION})
		ENDIF()
	ENDIF()
#	IF(NOT GTest_FOUND)
#		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}-test PRIVATE
#			${GTest_LOCATION}/
#			${GTest_LOCATION}/googletest/include
#		)
#		TARGET_LINK_LIBRARIES(${PROJECT_NAME}-test PUBLIC gtest )
#	ELSE()
#		TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}-test PRIVATE
#			${GTest_INCLUDE_DIR}/
#		)
#		TARGET_LINK_LIBRARIES(${PROJECT_NAME}-test GTest::GTest)
#	ENDIF()

#	TARGET_LINK_LIBRARIES(${PROJECT_NAME}-test PUBLIC ${PROJECT_NAME} )
    SET(TDIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
    SET(TBDIR ${CMAKE_CURRENT_BINARY_DIR}/Testing)

	ADD_TEST(NAME ${PROJECT_NAME}-functional
		COMMAND ${PROJECT_NAME} --regxml=${TDIR}/regpicker.xml --out=${TBDIR} ${TDIR}/vrtl/Vfiapp.cpp -- clang++ -I${TDIR}/vrtl/ -I$ENV{VERILATOR_ROOT}/include -I${CLANG_INCLUDE_DIRS}
	)
ENDIF()
