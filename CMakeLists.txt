cmake_minimum_required(VERSION 3.13)

project(mir)

set(TOP ${CMAKE_SOURCE_DIR})
set(GENDIR ${CMAKE_BINARY_DIR}/gen)

# 1. generate metadata
add_subdirectory(metadata)

# 2. convert metadata to JSON/headers
add_subdirectory(metadata_kb)

# 3. use metadata
add_subdirectory(target)
