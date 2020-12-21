#include "raylib.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#define UPDATE_FPS 60
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

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

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Empty");
    SetTargetFPS(UPDATE_FPS);

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif
    CloseWindow();

    return 0;
}
