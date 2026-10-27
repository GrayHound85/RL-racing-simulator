#pragma once

#include "racing/simulation/car_state.hpp"

namespace racing::simulation {

struct ControlInput {
    double steering = 0.0;
    double throttle = 0.0;
    double brake = 0.0;
};

struct SimulationConfig {
    double acceleration = 5.0;
    double braking_deceleration = 10.0;
    double drag = 0.1;
    double wheelbase = 2.5;
    double max_steering_angle = 0.5;
};

class Simulation {
public:
    explicit Simulation(SimulationConfig config = {});

    void reset(const CarState& initail_state = {});
    void step(const ControlInput& input, double delta_time);

    [[nodiscard]] const CarState& state() const noexcept;

private:
    SimulationConfig config_;
    CarState state_;

};

}