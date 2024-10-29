cmake_minimum_required(VERSION 3.25)
project(clew C)

set(CMAKE_C_STANDARD 23)

add_library(clew SHARED
        src/clew.h
        src/win32.c
        src/linux.c
        src/apple.c
)
target_include_directories(clew PUBLIC src/)
target_compile_definitions(clew PRIVATE -DCLEW_DLL_EXPORT -D_GNU_SOURCE)

add_executable(clew_sample sample/sample.c)
target_include_directories(clew_sample PRIVATE src/)
target_link_libraries(clew_sample clew)