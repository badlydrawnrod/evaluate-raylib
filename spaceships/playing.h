#pragma once

#include "controllers.h"
#include "screens.h"

namespace playing
{
    void Draw();
    ScreenState FixedUpdate();
    void HandleEdgeTriggeredEvents();
    void Start(int players, const ControllerId* controllers);
}
