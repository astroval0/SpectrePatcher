cmake_minimum_required(VERSION 4.0)

project(SpectrePatcher)

set(CMAKE_CXX_STANDARD 20)

add_library(BEClient_x64 SHARED "BE_ClientDLL.cpp")

set_target_properties(BEClient_x64 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/built"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/built"
)