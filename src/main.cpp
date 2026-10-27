#include "racing/simulation/simulation.hpp"

#include <iostream>

int main() {
    racing::simulation::Simulation simulation;

    constexpr double time_step = 1.0 / 120.0;
    constexpr int number_of_steps = 1200;

    const racing::simulation::ControlInput input{
        .steering = 0.2,
        .throttle = 1.0,
        .brake = 0.0
    };

    for (int i = 0; i < number_of_steps; ++i) {
        simulation.step(input, time_step);
    }

    const auto& car = simulation.state();

    std::cout << "Simulation complete\n";
    std::cout << "Simulated time: " << number_of_steps * time_step << " s\n";
    std::cout << "Position: (" << car.x << ", " << car.y << ")\n";
    std::cout << "Speed: " << car.speed << " m/s\n";
    std::cout << "Heading: " << car.heading << " radians\n";

    return 0;
}