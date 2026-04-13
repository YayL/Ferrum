cmake_minimum_required(VERSION 3.23)

project(Compiler VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug) 
set(CMAKE_C_FLAGS_DEBUG "-lm -Wall -Wno-unused -pg -g")
set(CMAKE_C_FLAGS_RELEASE "-lm -Ofast")

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

# set(FETCHCONTENT_FULLY_DISCONNECTED ON)

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
target_compile_definitions(compiler PRIVATE "DEBUG_MODE=$<IF:$<CONFIG:Debug>,1,0>")
