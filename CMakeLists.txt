
cmake_minimum_required(VERSION 3.23)

project(RacingSimulator
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(racing_sim
    src/main.cpp
    src/core/math.cpp
)

target_include_directories(racing_sim PRIVATE
    include
)

target_compile_options(racing_sim PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
