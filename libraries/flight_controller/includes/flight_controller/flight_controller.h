#pragma once

#include <Eigen/Dense>

namespace FlightController {

enum Direction
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
};

struct Controller
{
    Eigen::Matrix3f rotation{};
    Eigen::Vector3f position{};
    float linearSpeed{};
    float rotationalSpeed{};
    Eigen::Vector2f cursorPosition{};
    Eigen::Vector2f lastCursorPosition{};
};

Controller*
make();

void
free(Controller* self);

void
move(Controller* self, Direction direction);

}
