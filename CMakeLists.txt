cmake_minimum_required(VERSION 3.10)
project(CVM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(cvm 
    src/main.cpp 
    src/lexer.cpp 
    src/parser.cpp 
    src/compiler.cpp 
    src/vm.cpp
)

target_include_directories(cvm PRIVATE include)

if(MSVC)
    target_compile_options(cvm PRIVATE /W4)
else()
    target_compile_options(cvm PRIVATE -Wall -Wextra -pedantic)
endif()

# Optional: Enable Address and Undefined Behavior Sanitizers
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

if(ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(cvm PRIVATE -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(cvm PRIVATE -fsanitize=address,undefined)
endif()
