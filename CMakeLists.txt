cmake_minimum_required(VERSION 2.8)
project (moses)

SET(CMAKE_BUILD_TYPE "Debug")
ADD_DEFINITIONS("-Wall -std=c++11")
ADD_SUBDIRECTORY(src)
ADD_EXECUTABLE(moses src/lib/main.cpp)
TARGET_LINK_LIBRARIES(moses Engine IRSupport IR IRBuild Lexer Parser Support)
