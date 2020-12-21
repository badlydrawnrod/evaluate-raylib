#include "raylib.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#include <math.h>

#define SLOW_FPS 60
#define FAST_FPS 240

#define UPDATE_FPS 50

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

int renderFps = FAST_FPS;

#if defined(EMSCRIPTEN)
#define CAP_FRAME_RATE false
#else
#define CAP_FRAME_RATE true
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
const double maxDelta = 0.25;

float fixedX = 0.0f;
float fixedY = SCREEN_HEIGHT / 5.0f;
float fixedAngle = 0.0f;
float noAccY = 2 * SCREEN_HEIGHT / 5.0f;

float updateX = 0.0f;
float updateY = 3 * SCREEN_HEIGHT / 5.0f;
float updateAngle = 0.0f;

float drawX = 0.0f;
float drawY = 4 * SCREEN_HEIGHT / 5.0f;
float drawAngle = 0.0f;

void FixedUpdate()
{
    fixedX += speed;
    if (fixedX >= SCREEN_WIDTH)
    {
        fixedX -= SCREEN_WIDTH + width;
    }
    fixedAngle += rotation;
}

void Update(double elapsed)
{
    updateX += speed * UPDATE_FPS * elapsed;
    if (updateX >= SCREEN_WIDTH)
    {
        updateX -= SCREEN_WIDTH + width;
    }
    updateAngle += rotation * UPDATE_FPS * elapsed;
}

void HandleEdgeTriggeredEvents()
{
    if (IsKeyPressed(KEY_F11))
    {
        ToggleFullscreen();
    }

    if (IsKeyPressed(KEY_SPACE))
    {
        renderFps = (renderFps == FAST_FPS) ? SLOW_FPS : FAST_FPS;
        SetTargetFPS(renderFps);
    }
}

void Draw(double alpha)
{
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

    // Updated in FixedUpdate() with interpolation based on accumulator.
    float accX = speed * alpha;        // Interpolate movement.
    float accAngle = rotation * alpha; // Interpolate rotation.
    DrawText("Fixed (interpolation)", 4, fixedY, 32, RED);
    DrawRectangleRounded((Rectangle){fixedX + accX, fixedY, width, height}, 0.25f, 16, RED);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2, fixedY + height / 2, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, fixedAngle + accAngle, RED);

    // Updated in FixedUpdate() without interpolation based on accumulator.
    DrawText("Fixed (no interpolation)", 4, noAccY, 32, YELLOW);
    DrawRectangleRounded((Rectangle){fixedX, noAccY, width, height}, 0.25f, 16, YELLOW);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2, noAccY + height / 2, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, fixedAngle, YELLOW);

    // Updated in Update() with variable delta.
    DrawText("Variable", 4, updateY, 32, GREEN);
    DrawRectangleRounded((Rectangle){updateX, updateY, width, height}, 0.25f, 16, GREEN);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2, updateY + height / 2, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, updateAngle, GREEN);

    // Updated in Draw() with no delta.
    DrawText("Frame rate", 4, drawY, 32, BLUE);
    DrawRectangleRounded((Rectangle){drawX, drawY, width, height}, 0.25f, 16, BLUE);
    DrawRectanglePro((Rectangle){SCREEN_WIDTH / 2, drawY + height / 2, width * 2.0f, height * 0.5f},
                     (Vector2){width, height * 0.25f}, drawAngle, BLUE);

    DrawText(TextFormat("Physics: %3i updates/second", UPDATE_FPS), 4, SCREEN_HEIGHT - 24, 20, RED);
    DrawText(TextFormat("Render: %3i frames/second", GetFPS()), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 24, 20, BLUE);

    EndDrawing();
}

void UpdateDrawFrame()
{
    // See https://gafferongames.com/post/fix_your_timestep/ for more details on how this works.
    double now = GetTime();
    double delta = fmin(now - timing.lastTime, maxDelta);

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
        Update(fmin(drawInterval, maxDelta));

        // Draw the frame.
        Draw(timing.alpha);
        timing.lastDrawTime = now;
#if !defined(EMSCRIPTEN)
        HandleEdgeTriggeredEvents(); // As raylib updates input events in EndDrawing(), we update here so as to not miss
                                     // anything edge triggered such as a keypress when we have a high frame rate.
#endif
    }
}

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Loop");
    SetTargetFPS(renderFps);

    timing.t = 0.0;
    timing.updateInterval = 1.0 / UPDATE_FPS;
    timing.renderInterval = CAP_FRAME_RATE ? 1.0 / renderFps : 0.0;
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