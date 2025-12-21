cmake_minimum_required(VERSION 3.10)

project(algorithm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(algorithm main.cpp
        Matrix.h
        DeterminantCalculator.cpp
        DeterminantCalculator.h
        Application.cpp
        Application.h)

if (MSVC)
    target_compile_options(algorithm PRIVATE /W4 /permissive- /O2)
else()
    target_compile_options(algorithm PRIVATE -Wall -Wextra -pedantic -O3)
endif()
