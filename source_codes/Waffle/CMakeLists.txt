cmake_minimum_required(VERSION 3.15)
project(Waffle)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Release)

set(SOURCE_FILES main.cpp)
add_executable(Waffle ${SOURCE_FILES})

find_package(Torch REQUIRED)
target_link_libraries(Waffle PUBLIC "${TORCH_LIBRARIES}")

include_directories(include)
add_subdirectory(include)
target_link_libraries(Waffle PUBLIC include)

SET(CMAKE_CXX_FLAGS -pthread)