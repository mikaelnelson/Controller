# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -Wno-unused-variable")
set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -Wno-parentheses")
set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -Wno-misleading-indentation")
set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -Wno-unused-function")

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

message("SrcDir: ${CMAKE_SOURCE_DIR}")

idf_component_register(SRCS ${app_sources})
