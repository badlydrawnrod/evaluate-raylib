#include "raylib.h"

#include <cmath>

namespace my
{
    constexpr int updateFps = 60;
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    void Draw()
    {
        ClearBackground(BLACK);
        BeginDrawing();
        DrawFPS(4, 4);
        EndDrawing();
    }

} // namespace my

int main()
{
    InitWindow(my::screenWidth, my::screenHeight, "Empty");
    SetTargetFPS(my::updateFps);

    while (!WindowShouldClose())
    {
        my::Draw();
    }
    CloseWindow();

    return 0;
}
