cmake_minimum_required(VERSION 3.23)

project(RacingSimulator
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(racing_core
    src/core/math.cpp
    src/simulation/simulation.cpp
)

target_include_directories(racing_core
    PUBLIC include
)

target_compile_options(racing_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(racing_sim
    src/main.cpp
)

target_link_libraries(racing_sim
    PRIVATE racing_core
)

target_compile_options(racing_sim
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)