cmake_minimum_required(VERSION 3.23)

project(RaceRL
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#-------------------------------------------
add_executable(race_rl)

target_sources(race_rl
    PRIVATE
        src/main.cpp
)

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

#-------------------------------------------























target_include_directories(race_core
    PUBLIC include
)

target_link_libraries(race_sim
    PRIVATE racing_core
)

target_compile_options(race_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_compile_options(race_sim
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)