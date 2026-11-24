#include "racing/simulation/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace racing::simulation {

Simulation::Simulation(SimulationConfig config)
    : config_(config) {
    if (config_.acceleration < 0.0 ||
        config_.braking_deceleration < 0.0 ||
        config_.drag < 0.0 ||
        config_.wheelbase <= 0.0 ||
        config_.max_steering_angle < 0.0 ||
        config_.max_steering_angle >= std::numbers::pi / 2.0) {
        throw std::invalid_argument("Invalid simulation configuration");
    }
}

void Simulation::reset(const CarState& initial_state) {
    if (!std::isfinite(initial_state.x) ||
        !std::isfinite(initial_state.y) ||
        !std::isfinite(initial_state.heading) ||
        !std::isfinite(initial_state.speed) ||
        initial_state.speed < 0.0) {
        throw std::invalid_argument("Invalid initial car state");
    }

    state_ = initial_state;
}

void Simulation::step(
    const ControlInput& input,
    double delta_time
) {
    if (!std::isfinite(delta_time) || delta_time <= 0.0) {
        throw std::invalid_argument("Time step must be positive and finite");
    }

    if (!std::isfinite(input.steering) ||
        !std::isfinite(input.throttle) ||
        !std::isfinite(input.brake)) {
        throw std::invalid_argument("Control inputs must be finite");
    }

    const double steering = std::clamp(input.steering, -1.0, 1.0);
    const double throttle = std::clamp(input.throttle, 0.0, 1.0);
    const double brake = std::clamp(input.brake, 0.0, 1.0);

    const double acceleration =
        throttle * config_.acceleration
        - brake * config_.braking_deceleration
        - config_.drag * state_.speed;

    state_.speed = std::max(
        0.0,
        state_.speed + acceleration * delta_time
    );

    const double steering_angle =
        steering * config_.max_steering_angle;

    const double turning_rate =
        state_.speed / config_.wheelbase
        * std::tan(steering_angle);

    state_.heading += turning_rate * delta_time;

    state_.x += state_.speed * std::cos(state_.heading) * delta_time;
    state_.y += state_.speed * std::sin(state_.heading) * delta_time;
}

const CarState& Simulation::state() const noexcept {
    return state_;
}

}