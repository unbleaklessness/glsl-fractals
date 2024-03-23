#include "flight_controller.h"

namespace FlightController {

Controller*
make()
{
    Controller* result = new Controller;
    return result;
}

void
free(Controller* self)
{
    delete self;
}

void
move(Controller* self, Direction direction)
{
}

}
