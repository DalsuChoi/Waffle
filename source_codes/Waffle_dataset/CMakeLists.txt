cmake_minimum_required(VERSION 3.15)
project(Waffle_dataset)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Release)
add_executable(Waffle_dataset main.cpp)

find_package(Eigen3 REQUIRED)
target_include_directories(Waffle_dataset PUBLIC ${EIGEN3_INCLUDE_DIR})