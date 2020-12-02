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

MESSAGE("Using LLVM_DIR: ${LLVM_DIR}")
MESSAGE("LLVM Version: ${LLVM_VERSION} at ${LLVM_INSTALL_PREFIX}")
MESSAGE("LLVM INCLUDE DIRS: ${LLVM_INCLUDE_DIRS}")
MESSAGE("Using Clang_DIR: ${Clang_DIR}")

IF(LLVM_FOUND AND Clang_FOUND)
   
    llvm_map_components_to_libnames(LLVM_LIBS
      option object profiledata support core mc bitreader MCParser BinaryFormat ExecutionEngine Target MCJIT X86CodeGen X86AsmParser Interpreter Support
    )
    
    ADD_EXECUTABLE(${PROJECT_NAME}
        vrtlmod.cpp
        src/Consumer.cpp
        src/FileContext.cpp
        src/Misc.cpp
        src/rewriterHandler/RewriteMacrosAction.cpp
        src/APIbuild/apibuilder.cpp
        src/APIbuild/xmlhelper.cpp
        src/APIbuild/target.cpp
        src/APIbuild/injectionrewriter.cpp
        src/APIbuild/extendedmatchers.cpp
        src/APIbuild/utils.cpp
    )
    
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC
        ${LLVM_LIBS}
        xml2
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
        /usr/include/libxml2
        ${LLVM_INCLUDE_DIRS}
        ${CLANG_INCLUDE_DIRS}
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
