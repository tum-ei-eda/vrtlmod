# Shared by the full project and the standalone documentation project.
function(vrtlmod_add_documentation source_dir output_dir)
    cmake_parse_arguments(DOC "REQUIRED" "" "" ${ARGN})
    if(DOC_REQUIRED)
        find_package(Doxygen REQUIRED COMPONENTS doxygen dot)
    else()
        find_package(Doxygen QUIET COMPONENTS doxygen dot)
    endif()
    if(NOT DOXYGEN_FOUND)
        message(STATUS "Doxygen and Graphviz are needed to enable the doc and pages targets")
        return()
    endif()

    # Reuse the project's version without configuring its compiler or dependencies.
    if(NOT DEFINED VRTLMOD_VERSION_FULL)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source_dir}/CMakeLists.txt")
        file(READ "${source_dir}/CMakeLists.txt" version_source)
        set(version_parts)
        foreach(component IN ITEMS MAJOR MINOR PATCH)
            if(version_source MATCHES "set\\(VRTLMOD_VERSION_${component}[ \t]+([0-9]+)\\)")
                list(APPEND version_parts "${CMAKE_MATCH_1}")
            endif()
        endforeach()
        list(LENGTH version_parts version_part_count)
        if(version_part_count EQUAL 3)
            list(JOIN version_parts "." VRTLMOD_VERSION_FULL)
        else()
            set(VRTLMOD_VERSION_FULL "")
        endif()
    endif()

    set(VRTLMOD_DOC_SOURCE_DIR "${source_dir}")
    set(VRTLMOD_DOC_OUTPUT_DIR "${output_dir}")
    configure_file("${source_dir}/docs/doxygen/doxyfile.cmake.in" "${output_dir}/doxyfile" @ONLY)
    add_custom_target(doc
        COMMAND "${DOXYGEN_EXECUTABLE}" "${output_dir}/doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating vRTLmod API documentation with Doxygen"
        VERBATIM
    )
    add_custom_target(pages
        COMMAND "${CMAKE_COMMAND}"
            "-DSITE_SOURCE_DIR=${source_dir}/docs/site"
            "-DDOXYGEN_HTML_DIR=${output_dir}/html"
            "-DSITE_OUTPUT_DIR=${output_dir}/site"
            -P "${source_dir}/docs/cmake/StagePages.cmake"
        COMMENT "Assembling the vRTLmod user guide and API reference"
        VERBATIM
    )
    add_dependencies(pages doc)
endfunction()
