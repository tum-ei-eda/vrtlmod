cmake_minimum_required(VERSION 3.15)
project(verilator_discovery_fixture LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(PREPEND CMAKE_MODULE_PATH "${DISCOVERY_MODULE_DIR}")
foreach(variable IN ITEMS VERILATOR_ROOT VERILATOR_BIN)
    set(initial_${variable}_defined FALSE)
    if(DEFINED ENV{${variable}})
        set(initial_${variable}_defined TRUE)
        set(initial_${variable} "$ENV{${variable}}")
    endif()
endforeach()
function(check_caller_environment stage)
    foreach(variable IN ITEMS VERILATOR_ROOT VERILATOR_BIN)
        if(initial_${variable}_defined)
            if(NOT DEFINED ENV{${variable}} OR NOT "$ENV{${variable}}" STREQUAL "${initial_${variable}}")
                message(FATAL_ERROR "${stage} changed caller environment variable ${variable}")
            endif()
        elseif(DEFINED ENV{${variable}})
            message(FATAL_ERROR "${stage} defined caller environment variable ${variable}")
        endif()
    endforeach()
endfunction()
find_package(VERILATOR REQUIRED)
check_caller_environment("Verilator discovery")

file(WRITE "${CMAKE_BINARY_DIR}/selection.txt"
    "version=${verilator_VERSION}\nheaders=${VERILATOR_INCLUDE_DIRECTORY}\nexecutable=${VERILATOR_EXECUTABLE}\n")
add_executable(discovery driver.cpp)
target_compile_definitions(discovery PRIVATE EXPECTED_VERILATOR_VERSION="${verilator_VERSION}")
vrtlmod_verilate(discovery
    PREFIX Vdiscovery TOP_MODULE discovery
    DIRECTORY "${CMAKE_BINARY_DIR}/model"
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/discovery.sv"
    VERILATOR_ARGS -Wno-fatal
)
check_caller_environment("vrtlmod_verilate")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/executable-$<CONFIG>.txt" CONTENT "$<TARGET_FILE:discovery>")
