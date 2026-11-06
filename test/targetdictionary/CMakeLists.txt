set(TARGETDICTIONARY_TARGET ${PROJECT_NAME}-test-targetdictionary)
add_executable(${TARGETDICTIONARY_TARGET} EXCLUDE_FROM_ALL targetdictionary_test.cpp)
target_compile_features(${TARGETDICTIONARY_TARGET} PRIVATE cxx_std_17)
target_include_directories(${TARGETDICTIONARY_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/vapi/templates
    ${VERILATOR_INCLUDE_DIRECTORY}
    ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
)

add_test(NAME ${PROJECT_NAME}:test/targetdictionary
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${TARGETDICTIONARY_TARGET}
)
add_test(NAME run:test/targetdictionary COMMAND $<TARGET_FILE:${TARGETDICTIONARY_TARGET}>)
set_tests_properties(run:test/targetdictionary PROPERTIES DEPENDS ${PROJECT_NAME}:test/targetdictionary)
