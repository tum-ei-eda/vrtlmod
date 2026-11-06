set(UTILITY_TARGET ${PROJECT_NAME}-test-utility)
add_executable(${UTILITY_TARGET} EXCLUDE_FROM_ALL
    parse_int_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/util/utility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/util/logging.cpp
)
target_include_directories(${UTILITY_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${PROJECT_BINARY_DIR}/include
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(${UTILITY_TARGET} PRIVATE clangBasic LLVMSupport)
if(CMAKE_CXX_STANDARD LESS 17)
    target_link_libraries(${UTILITY_TARGET} PRIVATE Boost::filesystem)
endif()
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(${UTILITY_TARGET} PRIVATE -fno-rtti)
endif()

add_test(NAME ${PROJECT_NAME}:test/utility
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${UTILITY_TARGET}
)
set_tests_properties(${PROJECT_NAME}:test/utility PROPERTIES FIXTURES_SETUP utility-parser)
add_test(NAME run:test/utility COMMAND $<TARGET_FILE:${UTILITY_TARGET}>)
set_tests_properties(run:test/utility PROPERTIES
    DEPENDS ${PROJECT_NAME}:test/utility FIXTURES_REQUIRED utility-parser
)
