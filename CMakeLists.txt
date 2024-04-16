cmake_minimum_required(VERSION 3.28)
project(USKeyboardRemover)

set(CMAKE_CXX_STANDARD 17)

add_executable(USKeyboardRemover WIN32 main.cpp)
target_link_libraries(USKeyboardRemover -static-libstdc++ -static-libgcc --static)