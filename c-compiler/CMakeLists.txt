cmake_minimum_required(VERSION 3.23)

project(Compiler VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-pg -g")
set(CMAKE_C_FLAGS_RELEASE "-Wall")

message(STATUS "FLAGS: ${CMAKE_C_FLAGS_DEBUG}")

add_subdirectory("submodule/String-Formatter")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

message(STATUS "SOURCES: ${SOURCES}")

add_executable(compiler ${SOURCES})

# Add other flags to the compiler
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# Include header files
target_include_directories(compiler PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/String-Formatter/include)

# Link libraries
target_link_libraries(compiler PRIVATE fmt)
