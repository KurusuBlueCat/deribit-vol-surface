cmake_minimum_required(VERSION 3.22.1)

project(spmc-prototype, VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(project)

target_include_directories(step1 PUBLIC 
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}")

target_include_directories(step2 PUBLIC 
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}")