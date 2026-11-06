cmake_minimum_required(VERSION 3.15)
project(vrtlmod_docs LANGUAGES NONE)

get_filename_component(VRTLMOD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/VrtlmodDocs.cmake")
vrtlmod_add_documentation("${VRTLMOD_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/docs" REQUIRED)
