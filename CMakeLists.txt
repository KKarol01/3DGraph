cmake_minimum_required (VERSION 3.2)

project("opengl_engine")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory("src")
#add_subdirectory("tests")
