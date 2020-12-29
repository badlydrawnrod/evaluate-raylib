#include "raylib.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#define UPDATE_FPS 60
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

static const char* instruction = "Press [space] or [enter] to advance the message";
static const char* messages[] = {"raylib", "is", "great"};
static int index = 0;
static int numMessages = sizeof(messages) / sizeof(messages[0]);
static bool wasEnterDown = false;

static void Draw(void)
{
    ClearBackground(BLACK);
    BeginDrawing();

    int width = MeasureText(messages[index], 64);
    DrawText(messages[index], (GetScreenWidth() - width) / 2, GetScreenHeight() / 2, 64, GREEN);

    width = MeasureText(instruction, 32);
    DrawText(instruction, (GetScreenWidth() - width) / 2, GetScreenHeight() / 4, 32, RAYWHITE);

    DrawFPS(4, 4);
    EndDrawing();
}

static void UpdateDrawFrame(void)
{
    // Check for [space] changing state to pressed with IsKeyPressed().
    if (IsKeyPressed(KEY_SPACE))
    {
        index = (index + 1) % numMessages;
    }

    // Check for [enter] changing state to pressed by maintaining our own state.
    bool isEnterDown = IsKeyDown(KEY_ENTER);
    if (isEnterDown && !wasEnterDown)
    {
        index = (index + 1) % numMessages;
    }
    wasEnterDown = isEnterDown;

    Draw();
}

int main(void)
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
