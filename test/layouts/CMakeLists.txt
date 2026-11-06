find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Generate a real SystemC model, then qualify its type spellings explicitly so
# old and new Verilator versions exercise the same source-type preservation.
set(PORTS_RAW ${CMAKE_CURRENT_BINARY_DIR}/ports-original)
set(PORTS_PREPARED ${CMAKE_CURRENT_BINARY_DIR}/ports-prepared)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/sc_null.cpp
    "#include <systemc>\nint sc_main(int, char**) { return 0; }\n")
add_executable(layout-ports-original EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/sc_null.cpp)
target_link_libraries(layout-ports-original PRIVATE SystemC::systemc)
vrtlmod_verilate(layout-ports-original SYSTEMC TOP_MODULE port_types PREFIX Vport_types
    DIRECTORY ${PORTS_RAW} SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ports.sv
    VERILATOR_ARGS -O3 --pins-bv 65 -Wno-fatal)
get_target_property(LAYOUT_RUNTIME layout-ports-original SOURCES)
list(FILTER LAYOUT_RUNTIME INCLUDE REGEX "/verilated[^/]*[.]cpp$")
collect_verilated_test_sources(PORTS_INPUTS layout-ports-original "${PORTS_RAW}")
set(PORTS_SOURCES ${PORTS_INPUTS})
list(TRANSFORM PORTS_SOURCES REPLACE "${PORTS_RAW}/" "${PORTS_PREPARED}/")
add_custom_command(OUTPUT ${PORTS_SOURCES}
    DEPENDS layout-ports-original ${PORTS_INPUTS} ${CMAKE_CURRENT_SOURCE_DIR}/verify_layout.py
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/verify_layout.py
        prepare ${PORTS_PREPARED} ${PORTS_INPUTS}
    VERBATIM)

foreach(LAYOUT_MODE IN ITEMS named ports)
    set(LAYOUT_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LAYOUT_MODE}-modified)
    if(LAYOUT_MODE STREQUAL "named")
        set(LAYOUT_PREFIX Vlayout)
        set(LAYOUT_INPUTS ${CMAKE_CURRENT_SOURCE_DIR}/Vlayout.h
            ${CMAKE_CURRENT_SOURCE_DIR}/Vlayout__Syms.h ${CMAKE_CURRENT_SOURCE_DIR}/Vlayout.cpp)
        set(LAYOUT_EXTRA_ARGS)
        set(LAYOUT_EXTRA_DEPS)
    else()
        set(LAYOUT_PREFIX Vport_types)
        set(LAYOUT_INPUTS ${PORTS_SOURCES})
        set(LAYOUT_EXTRA_ARGS --systemc)
        set(LAYOUT_EXTRA_DEPS layout-ports-original)
    endif()
    set(LAYOUT_OUTPUTS)
    foreach(LAYOUT_INPUT IN LISTS LAYOUT_INPUTS)
        get_filename_component(LAYOUT_FILENAME ${LAYOUT_INPUT} NAME)
        list(APPEND LAYOUT_OUTPUTS ${LAYOUT_OUTPUT}/${LAYOUT_FILENAME})
    endforeach()
    set(LAYOUT_API_SOURCES)
    foreach(LAYOUT_SUFFIX IN ITEMS api _diffapi _diffapi_compare_fast _diffapi_compare _diffapi_compute)
        list(APPEND LAYOUT_API_SOURCES ${LAYOUT_OUTPUT}/${LAYOUT_PREFIX}_vrtlmod${LAYOUT_SUFFIX}.cpp)
    endforeach()
    set(LAYOUT_STAMP ${LAYOUT_OUTPUT}/verified.stamp)
    add_custom_command(OUTPUT ${LAYOUT_STAMP}
        BYPRODUCTS ${LAYOUT_OUTPUTS} ${LAYOUT_API_SOURCES}
            ${LAYOUT_OUTPUT}/${LAYOUT_PREFIX}_vrtlmodapi.hpp
            ${LAYOUT_OUTPUT}/${LAYOUT_PREFIX}_vrtlmod_diffapi.hpp
        DEPENDS ${PROJECT_NAME}-bin ${LAYOUT_INPUTS} ${LAYOUT_EXTRA_DEPS}
            ${CMAKE_CURRENT_SOURCE_DIR}/verify_layout.py
        COMMAND $<TARGET_FILE:${PROJECT_NAME}-bin> ${LAYOUT_EXTRA_ARGS}
            --out=${LAYOUT_OUTPUT} ${LAYOUT_INPUTS}
            -- ${LLVM_TOOLS_BINARY_DIR}/clang++ -xc++ ${VRTLMOD_CLANG_STDLIB_FLAG}
            -std=c++${CMAKE_CXX_STANDARD} -I${LAYOUT_OUTPUT}
            -I${VERILATOR_INCLUDE_DIRECTORY} -I${VERILATOR_INCLUDE_DIRECTORY}/vltstd
            -I${CLANG_INCLUDE_DIRS} -I${SYSTEMC_INCLUDE_DIRS}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/verify_layout.py
            ${LAYOUT_MODE} ${LAYOUT_OUTPUT}/${LAYOUT_PREFIX}-vrtlmod.xml
        COMMAND ${CMAKE_COMMAND} -E touch ${LAYOUT_STAMP}
        VERBATIM)
    add_custom_target(layout-${LAYOUT_MODE}-transform DEPENDS ${LAYOUT_STAMP})
    set_source_files_properties(${LAYOUT_OUTPUTS} ${LAYOUT_API_SOURCES} PROPERTIES GENERATED TRUE)
    set(LAYOUT_TARGET ${PROJECT_NAME}-test-layouts-${LAYOUT_MODE})
    add_executable(${LAYOUT_TARGET} EXCLUDE_FROM_ALL ${LAYOUT_MODE}_test.cpp
        ${LAYOUT_OUTPUTS} ${LAYOUT_API_SOURCES} ${LAYOUT_RUNTIME})
    add_dependencies(${LAYOUT_TARGET} layout-${LAYOUT_MODE}-transform)
    target_include_directories(${LAYOUT_TARGET} PRIVATE ${LAYOUT_OUTPUT}
        ${VERILATOR_INCLUDE_DIRECTORY} ${VERILATOR_INCLUDE_DIRECTORY}/vltstd)
    target_link_libraries(${LAYOUT_TARGET} PRIVATE ${VERILATOR_MT_CFLAGS})
    if(LAYOUT_MODE STREQUAL "ports")
        target_link_libraries(${LAYOUT_TARGET} PRIVATE SystemC::systemc)
    endif()
    add_test(NAME ${PROJECT_NAME}:test/layouts-${LAYOUT_MODE}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} ${PARALLEL_BUILD} --target ${LAYOUT_TARGET})
    set_tests_properties(${PROJECT_NAME}:test/layouts-${LAYOUT_MODE}
        PROPERTIES DEPENDS ${PROJECT_NAME}:build FIXTURES_SETUP layouts-${LAYOUT_MODE})
    add_test(NAME run:test/layouts-${LAYOUT_MODE} COMMAND $<TARGET_FILE:${LAYOUT_TARGET}>)
    set_tests_properties(run:test/layouts-${LAYOUT_MODE}
        PROPERTIES DEPENDS ${PROJECT_NAME}:test/layouts-${LAYOUT_MODE}
            FIXTURES_REQUIRED layouts-${LAYOUT_MODE})
endforeach()
