cmake_minimum_required(VERSION 3.23)

project(Compiler VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-pg -g")
set(CMAKE_C_FLAGS_RELEASE "-Wall")

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/yayl/string-formatter
    GIT_TAG main
)

FetchContent_MakeAvailable(fmt)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_executable(compiler ${SOURCES})

# Add other flags to the compiler
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# Include header files
target_include_directories(compiler PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${fmt_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(compiler PRIVATE fmt)
