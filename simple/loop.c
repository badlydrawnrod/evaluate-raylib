#include "raylib.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#include <math.h>

#define SLOW_FPS 60
#define FAST_FPS 360

#define MAX_DELTA 0.1f

#define UPDATE_FPS 50

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#if defined(EMSCRIPTEN)
#define CAP_FRAME_RATE
#endif

struct
{
    double t;              // Physics time.
    double updateInterval; // Desired fixed update interval (seconds).
    double renderInterval; // Desired renderer interval (seconds).
    double lastTime;       // When did we last try a fixed update?
    double accumulator;    // How much time was left over?
    double alpha;          // How far into the next fixed update are we?
    double lastDrawTime;   // When did we last draw?
} timing;

const float width = SCREEN_WIDTH / 16.0f;
const float height = SCREEN_HEIGHT / 16.0f;
const float speed = 4.0f;
const float rotation = 2.0f;

// What rate do we want to render at?
int renderFps = FAST_FPS;

// The position and orientation of items that are updated when FixedUpdate() is called.
float fixedX = 0.0f;
float fixedY = SCREEN_HEIGHT / 5.0f;
float fixedAngle = 0.0f;
float noAccY = 2 * SCREEN_HEIGHT / 5.0f;

// The position and orientation of items that are updated when Update() is called.
float updateX = 0.0f;
float updateY = 3 * SCREEN_HEIGHT / 5.0f;
float updateAngle = 0.0f;

// The position and orientation of items that are updated when Draw() is called.
float drawX = 0.0f;
float drawY = 4 * SCREEN_HEIGHT / 5.0f;
float drawAngle = 0.0f;

// Update positions and orientations with a fixed timestep.
void FixedUpdate(void)
{
    fixedX += speed;
    if (fixedX >= SCREEN_WIDTH)
    {
        fixedX -= SCREEN_WIDTH + width;
    }
    fixedAngle += rotation;
}

// Update positions and orientations with a variable timestep.
void Update(double elapsed)
{
    float seconds = (float)elapsed;
    updateX += speed * UPDATE_FPS * seconds;
    if (updateX >= SCREEN_WIDTH)
    {
        updateX -= SCREEN_WIDTH + width;
    }
    updateAngle += rotation * UPDATE_FPS * seconds;
}

// Draw everything.
void Draw(double alpha)
{
    // Update positions and orientations at the same rate at which we draw.
    drawX += speed;
    if (drawX >= SCREEN_WIDTH)
    {
        drawX -= SCREEN_WIDTH + width;
    }
    drawAngle += rotation;

    ClearBackground(BLACK);
    BeginDrawing();

    DrawText("Main loop update demo", 4, 4, 32, RAYWHITE);
    DrawText("Toggle frame rate [space]", 4, 36, 20, GRAY);
    DrawText("Toggle full screen [F11]", 4, 56, 20, GRAY);

    // Draw the items that were updated in FixedUpdate(). When drawing, interpolate position and orientation based on how far we //
    // are into the next frame.
    float accX = speed * (float)alpha;        // Interpolate movement.
    float accAngle = rotation * (float)alpha; // Interpolate rotation.
    DrawRectangleRounded((Rectangle){fixedX + accX, fixedY, width, height}, 0.25f, 16, RED);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2.0f, fixedY + height / 2.0f, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, fixedAngle + accAngle, RED);
    DrawText("Fixed (interpolation)", 4, (int)fixedY, 32, MAROON);

    // Draw the items that were updated in FixedUpdate(). Do not interpolate position and orientation.
    DrawRectangleRounded((Rectangle){fixedX, noAccY, width, height}, 0.25f, 16, YELLOW);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2.0f, noAccY + height / 2.0f, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, fixedAngle, YELLOW);
    DrawText("Fixed (no interpolation)", 4, (int)noAccY, 32, GOLD);

    // Draw the items that were updated in Update().
    DrawRectangleRounded((Rectangle){updateX, updateY, width, height}, 0.25f, 16, GREEN);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2.0f, updateY + height / 2.0f, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, updateAngle, GREEN);
    DrawText("Variable", 4, (int)updateY, 32, DARKGREEN);

    // Draw the items that were updated in Draw().
    DrawRectangleRounded((Rectangle){drawX, drawY, width, height}, 0.25f, 16, BLUE);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2.0f, drawY + height / 2, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, drawAngle, BLUE);
    DrawText("Frame rate", 4, (int)drawY, 32, DARKBLUE);

    // Display some stats.
    DrawText(TextFormat("Physics: %3i updates/second", UPDATE_FPS), 4, SCREEN_HEIGHT - 24, 20, RED);
    DrawText(TextFormat("Render: %3i frames/second", GetFPS()), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 24, 20, BLUE);

    EndDrawing();
}

// Check for edge-triggered events such as keys being pressed or released.
void CheckTriggers(void)
{
    if (IsKeyPressed(KEY_F11))
    {
        ToggleFullscreen();
    }

    // Toggle the frame rate between fast and slow. You are unlikely to see this change on web builds, or if your graphics card's
    // vsync setting is pinned to the monitor's refresh rate.
    if (IsKeyPressed(KEY_SPACE))
    {
        renderFps = (renderFps == FAST_FPS) ? SLOW_FPS : FAST_FPS;
        SetTargetFPS(renderFps);
    }
}

// Update and draw.
//
// - FixedUpdate() is called with a fixed timestep.
// - Update() is called once per frame, with a delta giving the elapsed time in seconds since it was last called.
// - Draw() is called once per frame, with an alpha value (0.0 <= alpha < 1.0) showing how far we are into the next fixed update.
//
// See https://gafferongames.com/post/fix_your_timestep/ for more details on how this works.
void UpdateDrawFrame(void)
{
#if defined(__EMSCRIPTEN__)
    // For web builds we only need to check for edge-triggered events once per frame.
    CheckTriggers();
#endif

    double now = GetTime();
    double delta = fmin(now - timing.lastTime, MAX_DELTA);

    // Fixed timestep updates.
    timing.lastTime = now;
    timing.accumulator += delta;
    while (timing.accumulator >= timing.updateInterval)
    {
        FixedUpdate();
        timing.t += timing.updateInterval;
        timing.accumulator -= timing.updateInterval;
    }

    // How far are we into the next fixed update?
    timing.alpha = timing.accumulator / timing.updateInterval;

    // Draw, potentially capping the frame rate.
    now = GetTime();
    double drawInterval = now - timing.lastDrawTime;
    if (drawInterval >= timing.renderInterval)
    {
        // Per-frame update.
        Update(fmin(drawInterval, MAX_DELTA));

        // Draw the frame.
        Draw(timing.alpha);
        timing.lastDrawTime = now;

#if !defined(__EMSCRIPTEN__)
        // Raylib updates its input events in EndDrawing(), so we check for edge-triggered input events such as key-presses here so
        // that we don't miss them when we have a high frame rate.
        CheckTriggers();
#endif
    }
}

int main(void)
{
#if !defined(NO_MSAA)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#endif
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Loop");
    SetTargetFPS(renderFps);

    timing.t = 0.0;
    timing.updateInterval = 1.0 / UPDATE_FPS;
#ifdef CAP_FRAME_RATE
    timing.renderInterval = 1.0 / renderFps;
#else
    timing.renderInterval = 0.0;
#endif
    timing.accumulator = 0.0;
    timing.alpha = 0.0f;
    timing.lastTime = GetTime();
    timing.lastDrawTime = timing.lastTime;

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
