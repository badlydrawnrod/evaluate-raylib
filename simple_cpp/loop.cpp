#include "raylib.h"

#include <algorithm>

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

namespace my
{
    constexpr int slowFps = 60;
    constexpr int fastFps = 240;

    constexpr int updateFps = 50;
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    int renderFps = fastFps;

#if defined(EMSCRIPTEN)
    constexpr bool capFrameRate = false;
#else
    constexpr bool capFrameRate = true;
#endif

    struct
    {
        double t = 0.0;                                               // Physics time.
        double updateInterval = 1.0 / updateFps;                      // Desired fixed update interval (seconds).
        double renderInterval = capFrameRate ? 1.0 / renderFps : 0.0; // Desired renderer interval (seconds).
        double lastTime = 0.0;                                        // When did we last try a fixed update?
        double accumulator = 0.0;                                     // How much time was left over?
        double alpha = 0.0f;                                          // How far into the next fixed update are we?
        double lastDrawTime = 0.0;                                    // When did we last draw?
    } timing;

    constexpr float width = screenWidth / 16;
    constexpr float height = screenHeight / 16;
    constexpr float speed = 4.0f;
    constexpr float rotation = 2.0f;
    constexpr double maxDelta = 0.25;

    float fixedX = 0.0f;
    float fixedY = screenHeight / 5;
    float fixedAngle = 0.0f;
    float noAccY = 2 * screenHeight / 5;

    float updateX = 0.0f;
    float updateY = 3 * screenHeight / 5;
    float updateAngle = 0.0f;

    float drawX = 0.0f;
    float drawY = 4 * screenHeight / 5;
    float drawAngle = 0.0f;

    void FixedUpdate()
    {
        fixedX += speed;
        if (fixedX >= screenWidth)
        {
            fixedX -= screenWidth + width;
        }
        fixedAngle += rotation;
    }

    void Update(double elapsed)
    {
        updateX += speed * updateFps * elapsed;
        if (updateX >= screenWidth)
        {
            updateX -= screenWidth + width;
        }
        updateAngle += rotation * updateFps * elapsed;
    }

    void HandleEdgeTriggeredEvents()
    {
        if (IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        if (IsKeyPressed(KEY_SPACE))
        {
            renderFps = (renderFps == fastFps) ? slowFps : fastFps;
            SetTargetFPS(renderFps);
        }
    }

    void Draw(double alpha)
    {
        drawX += speed;
        if (drawX >= screenWidth)
        {
            drawX -= screenWidth + width;
        }
        drawAngle += rotation;

        ClearBackground(BLACK);
        BeginDrawing();

        DrawText("Main loop update demo", 4, 4, 32, RAYWHITE);
        DrawText("Toggle frame rate [space]", 4, 36, 20, GRAY);
        DrawText("Toggle full screen [F11]", 4, 56, 20, GRAY);

        // Updated in FixedUpdate() with interpolation based on accumulator.
        float accX = speed * alpha;        // Interpolate movement.
        float accAngle = rotation * alpha; // Interpolate rotation.
        DrawText("Fixed (interpolation)", 4, fixedY, 32, RED);
        DrawRectangleRounded({fixedX + accX, fixedY, width, height}, 0.25f, 16, RED);
        DrawRectanglePro({screenWidth / 2, fixedY + height / 2, width * 2.0f, height * 0.5f}, {width, height * 0.25f},
                         fixedAngle + accAngle, RED);

        // Updated in FixedUpdate() without interpolation based on accumulator.
        DrawText("Fixed (no interpolation)", 4, noAccY, 32, YELLOW);
        DrawRectangleRounded({fixedX, noAccY, width, height}, 0.25f, 16, YELLOW);
        DrawRectanglePro({screenWidth / 2, noAccY + height / 2, width * 2.0f, height * 0.5f}, {width, height * 0.25f}, fixedAngle,
                         YELLOW);

        // Updated in Update() with variable delta.
        DrawText("Variable", 4, updateY, 32, GREEN);
        DrawRectangleRounded({updateX, updateY, width, height}, 0.25f, 16, GREEN);
        DrawRectanglePro({screenWidth / 2, updateY + height / 2, width * 2.0f, height * 0.5f}, {width, height * 0.25f}, updateAngle,
                         GREEN);

        // Updated in Draw() with no delta.
        DrawText("Frame rate", 4, drawY, 32, BLUE);
        DrawRectangleRounded({drawX, drawY, width, height}, 0.25f, 16, BLUE);
        DrawRectanglePro({screenWidth / 2, drawY + height / 2, width * 2.0f, height * 0.5f}, {width, height * 0.25f}, drawAngle,
                         BLUE);

        DrawText(TextFormat("Physics: %3i updates/second", updateFps), 4, screenHeight - 24, 20, RED);
        DrawText(TextFormat("Render: %3i frames/second", GetFPS()), screenWidth / 2, screenHeight - 24, 20, BLUE);

        EndDrawing();
    }

    void UpdateDrawFrame()
    {
        // See https://gafferongames.com/post/fix_your_timestep/ for more details on how this works.
        double now = GetTime();
        double delta = std::min(now - timing.lastTime, maxDelta);

        // Fixed timestep updates.
        timing.lastTime = now;
        timing.accumulator += delta;
        while (timing.accumulator >= timing.updateInterval)
        {
            FixedUpdate();
            timing.t += timing.updateInterval;
            timing.accumulator -= timing.updateInterval;
#if defined(EMSCRIPTEN)
            HandleEdgeTriggeredEvents();
#endif
        }
        timing.alpha = timing.accumulator / timing.updateInterval;

        // Draw, potentially capping the frame rate.
        now = GetTime();
        double drawInterval = now - timing.lastDrawTime;
        if (drawInterval >= timing.renderInterval)
        {
            // Per-frame update.
            Update(std::min(drawInterval, maxDelta));

            // Draw the frame.
            Draw(timing.alpha);
            timing.lastDrawTime = now;
#if !defined(EMSCRIPTEN)
            HandleEdgeTriggeredEvents(); // As raylib updates input events in EndDrawing(), we update here so as to not miss
                                         // anything edge triggered such as a keypress when we have a high frame rate.
#endif
        }
    }
} // namespace my

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(my::screenWidth, my::screenHeight, "Loop");
    SetTargetFPS(my::renderFps);
    my::timing.lastTime = GetTime();
    my::timing.lastDrawTime = my::timing.lastTime;
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
