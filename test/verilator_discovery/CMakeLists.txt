find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME ${PROJECT_NAME}:test/verilator-discovery
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_discovery.py
        --module-dir ${PROJECT_SOURCE_DIR}/cmake
        --root ${VERILATOR_ROOT}
        --executable ${VERILATOR_EXECUTABLE}
        --version ${verilator_VERSION}
        --cmake ${CMAKE_COMMAND}
        --cc ${CMAKE_C_COMPILER}
        --cxx ${CMAKE_CXX_COMPILER}
        --generator ${CMAKE_GENERATOR}
        --make-program ${CMAKE_MAKE_PROGRAM}
        --configuration $<CONFIG>
        --work-dir ${CMAKE_CURRENT_BINARY_DIR}/cases
)
set_tests_properties(${PROJECT_NAME}:test/verilator-discovery PROPERTIES PROCESSORS 2 TIMEOUT 300)
