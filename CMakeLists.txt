cmake_minimum_required(VERSION 3.23)

project(RaceRL
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Executable
#-------------------------------------------
add_executable(race_rl)

target_sources(race_rl
    PRIVATE
        src/main.cpp
)

target_compile_options(race_rl
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

# Libraries
#-------------------------------------------
add_library(race_core)

target_sources(race_core
    PRIVATE
        src/core/math.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include/race
        FILES
            include/race/core/math.hpp
)

target_compile_options(race_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

#-------------------------------------------
add_library(race_sim)

target_sources(race_sim
    PRIVATE
        src/sim/sim.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include/race
        FILES
            include/race/sim/sim.hpp
            include/race/sim/car_state.hpp
)

target_compile_options(race_sim
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

# Linking Libraries
#-------------------------------------------
target_link_libraries(race_rl
    PRIVATE
        race_core
        race_sim
)
