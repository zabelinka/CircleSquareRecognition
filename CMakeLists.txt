cmake_minimum_required(VERSION 3.10)
project(TransasTestProject)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)

add_executable(TransasTestProject main.cpp)

target_link_libraries(TransasTestProject ${OpenCV_LIBS})