#include "racing/core/math.hpp"

#include <cmath>

namespace racing::core {

double distance(double x1, double y1, double x2, double y2) {
    const double dx = x2 - x1;
    const double dy = y2 - y1;

    return std::sqrt(dx * dx + dy * dy);
}

}