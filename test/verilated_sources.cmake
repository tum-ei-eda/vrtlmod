# Some Verilator versions list generated implementation files before writing them.
# Merge the target's source list with existing headers instead of relying on a glob
# snapshot. Only code files are outputs of vRTLmod; build metadata is not copied.
function(collect_verilated_test_sources output target directory)
    file(GLOB model_sources LIST_DIRECTORIES false
        "${directory}/*.cpp" "${directory}/*.cc" "${directory}/*.h" "${directory}/*.hpp")
    get_target_property(target_sources ${target} SOURCES)
    foreach(source IN LISTS target_sources)
        string(FIND "${source}" "${directory}/" model_prefix)
        if(model_prefix EQUAL 0 AND source MATCHES "[.](cpp|cc|h|hpp)$")
            list(APPEND model_sources "${source}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES model_sources)
    set(${output} ${model_sources} PARENT_SCOPE)
endfunction()
