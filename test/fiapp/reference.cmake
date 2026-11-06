# Build a plain model with a separate prefix so it can run beside the rewritten
# model in the same harness. Its code never passes through vRTLmod.
set(FIAPP_REFERENCE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/fiapp.sv")
function(add_fiapp_reference target)
    cmake_parse_arguments(REF "SYSTEMC" "" "VERILATOR_ARGS" ${ARGN})
    set(mode)
    if(REF_SYSTEMC)
        set(mode SYSTEMC)
    endif()
    add_library(${target} STATIC EXCLUDE_FROM_ALL)
    vrtlmod_verilate(${target}
        ${mode}
        TOP_MODULE fiapp
        PREFIX VfiappReference
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target}
        SOURCES ${FIAPP_REFERENCE_SOURCE}
        VERILATOR_ARGS ${REF_VERILATOR_ARGS}
    )
    if(REF_SYSTEMC)
        target_link_libraries(${target} PUBLIC SystemC::systemc)
    endif()
endfunction()
