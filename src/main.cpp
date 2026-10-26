#include <iostream>

#include "racing/core/math.hpp"

int main() {
    const double result = racing::core::distance(0.0, 0.0, 3.0, 4.0);

    std::cout << "Distance: " << result << '\n';

    return 0;
}