#include "raylib.h"

#include <cmath>

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

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

    void UpdateDrawFrame()
    {
        Draw();
    }
} // namespace my

int main()
{
    InitWindow(my::screenWidth, my::screenHeight, "Empty");
    SetTargetFPS(my::updateFps);

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(my::UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose())
    {
        my::UpdateDrawFrame();
    }
#endif
    CloseWindow();

    return 0;
}
