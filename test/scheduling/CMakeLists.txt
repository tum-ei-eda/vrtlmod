file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.cpp "int main() { return 0; }\n")
set(SCHEDULING_PREFIX Vscheduling_comb)

# Exercise both split sequential functions and small functions inlined into region
# drivers. The same runtime assertions must hold for both generated layouts.
foreach(SCHEDULING_MODE IN ITEMS split inline)
    set(SCHEDULING_ORIGINAL_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SCHEDULING_MODE}/${SCHEDULING_PREFIX})
    set(SCHEDULING_MODIFIED_DIR ${SCHEDULING_ORIGINAL_DIR}/vrtlmod)
    set(SCHEDULING_ORIGINAL_TARGET scheduling-original-${SCHEDULING_MODE})
    set(SCHEDULING_TARGET ${PROJECT_NAME}-test-scheduling-${SCHEDULING_MODE})
    set(SCHEDULING_SPLIT_ARGS)
    if(SCHEDULING_MODE STREQUAL "split")
        set(SCHEDULING_SPLIT_ARGS --output-split-cfuncs 1)
    endif()

    add_executable(${SCHEDULING_ORIGINAL_TARGET} EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/null.cpp)
    verilate(${SCHEDULING_ORIGINAL_TARGET}
        TOP_MODULE scheduling
        # A model prefix containing _comb must not identify a comb region.
        PREFIX ${SCHEDULING_PREFIX}
        DIRECTORY ${SCHEDULING_ORIGINAL_DIR}
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/scheduling.sv
        VERILATOR_ARGS -O3 ${SCHEDULING_SPLIT_ARGS} -Wno-fatal
    )
    get_target_property(SCHEDULING_RUNTIME_SOURCES ${SCHEDULING_ORIGINAL_TARGET} SOURCES)
    list(FILTER SCHEDULING_RUNTIME_SOURCES INCLUDE REGEX "/verilated[^/]*[.]cpp$")
    collect_verilated_test_sources(SCHEDULING_INPUTS ${SCHEDULING_ORIGINAL_TARGET} "${SCHEDULING_ORIGINAL_DIR}")
    set(SCHEDULING_OUTPUTS ${SCHEDULING_INPUTS})
    list(TRANSFORM SCHEDULING_OUTPUTS REPLACE "${SCHEDULING_ORIGINAL_DIR}/" "${SCHEDULING_MODIFIED_DIR}/")
    set(SCHEDULING_API_SOURCES
        ${SCHEDULING_MODIFIED_DIR}/${SCHEDULING_PREFIX}_vrtlmodapi.cpp
        ${SCHEDULING_MODIFIED_DIR}/${SCHEDULING_PREFIX}_vrtlmod_diffapi.cpp
        ${SCHEDULING_MODIFIED_DIR}/${SCHEDULING_PREFIX}_vrtlmod_diffapi_compare_fast.cpp
        ${SCHEDULING_MODIFIED_DIR}/${SCHEDULING_PREFIX}_vrtlmod_diffapi_compare.cpp
        ${SCHEDULING_MODIFIED_DIR}/${SCHEDULING_PREFIX}_vrtlmod_diffapi_compute.cpp
    )
    add_custom_command(
        OUTPUT ${SCHEDULING_OUTPUTS} ${SCHEDULING_API_SOURCES}
            ${SCHEDULING_MODIFIED_DIR}/${SCHEDULING_PREFIX}_vrtlmodapi.hpp
        DEPENDS ${SCHEDULING_ORIGINAL_TARGET} ${PROJECT_NAME}-bin ${SCHEDULING_INPUTS}
        COMMAND $<TARGET_FILE:${PROJECT_NAME}-bin> --out=${SCHEDULING_MODIFIED_DIR} ${SCHEDULING_INPUTS}
            -- ${LLVM_TOOLS_BINARY_DIR}/clang++ -xc++ ${VRTLMOD_CLANG_STDLIB_FLAG}
            -std=c++${CMAKE_CXX_STANDARD}
            -I${SCHEDULING_MODIFIED_DIR} -I${VERILATOR_INCLUDE_DIRECTORY}
            -I${VERILATOR_INCLUDE_DIRECTORY}/vltstd -I${CLANG_INCLUDE_DIRS}
        VERBATIM
    )
    add_executable(${SCHEDULING_TARGET} EXCLUDE_FROM_ALL
        scheduling_test.cpp ${SCHEDULING_OUTPUTS} ${SCHEDULING_API_SOURCES} ${SCHEDULING_RUNTIME_SOURCES}
    )
    target_include_directories(${SCHEDULING_TARGET} PRIVATE
        ${SCHEDULING_MODIFIED_DIR} ${VERILATOR_INCLUDE_DIRECTORY} ${VERILATOR_INCLUDE_DIRECTORY}/vltstd
    )
    target_link_libraries(${SCHEDULING_TARGET} PRIVATE ${VERILATOR_MT_CFLAGS})

    add_test(NAME ${PROJECT_NAME}:test/scheduling-${SCHEDULING_MODE}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${SCHEDULING_TARGET}
    )
    set_tests_properties(${PROJECT_NAME}:test/scheduling-${SCHEDULING_MODE}
        PROPERTIES DEPENDS ${PROJECT_NAME}:build)
    add_test(NAME run:test/scheduling-${SCHEDULING_MODE} COMMAND $<TARGET_FILE:${SCHEDULING_TARGET}>)
    set_tests_properties(run:test/scheduling-${SCHEDULING_MODE}
        PROPERTIES DEPENDS ${PROJECT_NAME}:test/scheduling-${SCHEDULING_MODE})
endforeach()
