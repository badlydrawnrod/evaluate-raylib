#pragma once

#include "controllers.h"
#include "raylib.h"
#include "screens.h"

namespace menu
{
    void Draw();
    ScreenState FixedUpdate();
    ControllerId GetControllerAssignment(int player);
    void Start();
    int GetNumberOfPlayers();
} // namespace menu
