cmake_minimum_required(VERSION 3.23)
project(stack)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(stack main.cpp include/lexer.h include/global.h src/lexer.cpp)
