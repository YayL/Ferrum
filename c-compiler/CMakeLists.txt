cmake_minimum_required(VERSION 3.23)

project(Compiler VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug) 
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wno-unused -pg -g")
set(CMAKE_C_FLAGS_RELEASE "-Ofast")

execute_process(
    COMMAND ping www.google.com -n 2 -w 1
    RESULT_VARIABLE NO_CONNECTION
)
if(NO_CONNECTION GREATER 0)
    set(FETCHCONTENT_FULLY_DISCONNECTED ON)
else()
    set(FETCHCONTENT_FULLY_DISCONNECTED OFF)
endif()

find_library(LIBBT NAMES libbacktrace)

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/yayl/string-formatter
    GIT_TAG main
)

FetchContent_MakeAvailable(fmt)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_executable(compiler ${SOURCES})

target_include_directories(compiler PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${fmt_SOURCE_DIR}/include
)

target_link_libraries(compiler PRIVATE fmt backtrace)
