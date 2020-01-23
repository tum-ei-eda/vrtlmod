PROJECT(vrtlmod)

IF(NOT ENV{LLVM_DIR})
    SET(ENV{LLVM_DIR} "/usr/local/research/projects/SystemDesign/tools/llvm/9.0.0")
ENDIF()

SET(THISDIR ${CMAKE_SOURCE_DIR}/src/vrtlmod)

SET(THISLIB "L${PROJECT_NAME}")

FIND_PACKAGE(LLVM 9 CONFIG)

IF(LLVM_FOUND)
    MESSAGE("Using LLVM_DIR: ${LLVM_DIR}")
    MESSAGE("LLVM Version: ${LLVM_VERSION}")

    LINK_DIRECTORIES(${LLVM_LIBRARY_DIRS})
    ADD_DEFINITIONS(${LLVM_DEFINITIONS})
    llvm_map_components_to_libnames(LLVM_LIBS option object profiledata support core mc bitreader MCParser BinaryFormat ExecutionEngine Target MCJIT X86CodeGen X86AsmParser Interpreter Support)
   
    ADD_LIBRARY(${THISLIB} STATIC
        src/Consumer.cpp
        src/FileContext.cpp
        src/Misc.cpp
        src/rewriterHandler/RewriteMacrosAction.cpp
        src/APIbuild/apibuilder.cpp
        src/APIbuild/xmlhelper.cpp
        src/APIbuild/target.cpp
        src/APIbuild/injectionrewriter.cpp
        src/APIbuild/extendedmatchers.cpp
    )
    
    TARGET_LINK_LIBRARIES(${THISLIB} PUBLIC
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
    TARGET_INCLUDE_DIRECTORIES(${THISLIB} PUBLIC 
        include
        /usr/include/libxml2
        ${LLVM_INCLUDE_DIRS}
        )

    ADD_EXECUTABLE(${PROJECT_NAME}
        vrtlmod.cpp
    )
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC
        ${THISLIB}
    )

ENDIF()

option(BUILD_DOC "Build documentation" ON)
find_package(Doxygen)

if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxyfile)
    set(DOXYGEN_IN ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating ${PROJECT_NAME} API documentation with Doxygen"
            VERBATIM )
            
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
