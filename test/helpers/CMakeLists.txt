find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(HELPER_INPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/input)
set(HELPER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/vrtlmod)
if(verilator_VERSION_MAJOR LESS 5)
    set(HELPER_SEQUENT_FUNCTION Vhelperdest___sequent__TOP__0)
else()
    set(HELPER_SEQUENT_FUNCTION Vhelperdest___nba_sequent__TOP__0)
endif()
file(MAKE_DIRECTORY ${HELPER_INPUT_DIR})
configure_file(Vhelperdest.cpp.in ${HELPER_INPUT_DIR}/Vhelperdest.cpp @ONLY)
configure_file(Vhelperdest.h ${HELPER_INPUT_DIR}/Vhelperdest.h COPYONLY)
configure_file(Vhelperdest__Syms.h ${HELPER_INPUT_DIR}/Vhelperdest__Syms.h COPYONLY)
set(HELPER_INPUTS
    ${HELPER_INPUT_DIR}/Vhelperdest.cpp
    ${HELPER_INPUT_DIR}/Vhelperdest.h
    ${HELPER_INPUT_DIR}/Vhelperdest__Syms.h
)
set(HELPER_CLANG_ARGS
    ${LLVM_TOOLS_BINARY_DIR}/clang++ -xc++ ${VRTLMOD_CLANG_STDLIB_FLAG} -std=c++${CMAKE_CXX_STANDARD}
    -I${HELPER_INPUT_DIR} -I${VERILATOR_INCLUDE_DIRECTORY} -I${VERILATOR_INCLUDE_DIRECTORY}/vltstd
    -I${CLANG_INCLUDE_DIRS}
)
set(HELPER_API_SOURCES
    ${HELPER_OUTPUT_DIR}/Vhelperdest_vrtlmodapi.cpp
    ${HELPER_OUTPUT_DIR}/Vhelperdest_vrtlmod_diffapi.cpp
    ${HELPER_OUTPUT_DIR}/Vhelperdest_vrtlmod_diffapi_compare_fast.cpp
    ${HELPER_OUTPUT_DIR}/Vhelperdest_vrtlmod_diffapi_compare.cpp
    ${HELPER_OUTPUT_DIR}/Vhelperdest_vrtlmod_diffapi_compute.cpp
)
set(HELPER_OUTPUTS
    ${HELPER_OUTPUT_DIR}/Vhelperdest.cpp
    ${HELPER_OUTPUT_DIR}/Vhelperdest.h
    ${HELPER_OUTPUT_DIR}/Vhelperdest__Syms.h
    ${HELPER_OUTPUT_DIR}/Vhelperdest_vrtlmodapi.hpp
    ${HELPER_OUTPUT_DIR}/Vhelperdest-vrtlmod.xml
    ${HELPER_API_SOURCES}
)
add_custom_command(
    OUTPUT ${HELPER_OUTPUTS}
    DEPENDS ${PROJECT_NAME}-bin ${HELPER_INPUTS} ${CMAKE_CURRENT_SOURCE_DIR}/check_inventory.py
    COMMAND $<TARGET_FILE:${PROJECT_NAME}-bin> --out=${HELPER_OUTPUT_DIR} ${HELPER_INPUTS}
        -- ${HELPER_CLANG_ARGS}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_inventory.py
        ${HELPER_OUTPUT_DIR}/Vhelperdest-vrtlmod.xml
    VERBATIM
)
add_executable(${PROJECT_NAME}-test-helpers EXCLUDE_FROM_ALL
    helper_destinations_test.cpp ${HELPER_OUTPUTS} ${CC_RUNTIME_SOURCES}
)
target_include_directories(${PROJECT_NAME}-test-helpers PRIVATE
    ${HELPER_OUTPUT_DIR} ${VERILATOR_INCLUDE_DIRECTORY} ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
)
target_link_libraries(${PROJECT_NAME}-test-helpers PRIVATE ${VERILATOR_MT_CFLAGS})
add_test(NAME ${PROJECT_NAME}:test/helpers
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${PROJECT_NAME}-test-helpers
)
set_tests_properties(${PROJECT_NAME}:test/helpers PROPERTIES
    DEPENDS ${PROJECT_NAME}:build FIXTURES_SETUP helper-destinations
)
add_test(NAME run:test/helpers COMMAND $<TARGET_FILE:${PROJECT_NAME}-test-helpers>)
set_tests_properties(run:test/helpers PROPERTIES
    DEPENDS ${PROJECT_NAME}:test/helpers FIXTURES_REQUIRED helper-destinations
)

# Three unpacked dimensions plus VlWide exceed the API's three container levels.
# Check the modern constructor matcher separately without requesting that API.
file(READ ${VERILATOR_INCLUDE_DIRECTORY}/verilated_types.h HELPER_VERILATOR_TYPES)
if(HELPER_VERILATOR_TYPES MATCHES "class WDataOutP")
    set(HELPER_THREE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/three-unpacked)
    add_custom_command(
        OUTPUT ${HELPER_THREE_OUTPUT_DIR}/Vhelperdest-vrtlmod.xml
        DEPENDS ${PROJECT_NAME}-bin ${HELPER_INPUTS} ${CMAKE_CURRENT_SOURCE_DIR}/check_inventory.py
        COMMAND $<TARGET_FILE:${PROJECT_NAME}-bin> --xml-only --out=${HELPER_THREE_OUTPUT_DIR} ${HELPER_INPUTS}
            -- ${HELPER_CLANG_ARGS} -DHELPER_THREE_UNPACKED
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_inventory.py
            ${HELPER_THREE_OUTPUT_DIR}/Vhelperdest-vrtlmod.xml --three-unpacked
        VERBATIM
    )
    add_custom_target(${PROJECT_NAME}-test-helpers-three-unpacked
        DEPENDS ${HELPER_THREE_OUTPUT_DIR}/Vhelperdest-vrtlmod.xml
    )
    add_test(NAME ${PROJECT_NAME}:test/helpers-three-unpacked
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD}
            --target ${PROJECT_NAME}-test-helpers-three-unpacked
    )
    set_tests_properties(${PROJECT_NAME}:test/helpers-three-unpacked PROPERTIES DEPENDS ${PROJECT_NAME}:build)
endif()
