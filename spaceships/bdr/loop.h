#pragma once

#include "raylib.h"

#include <math.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BDR_LOOP_STATIC)
#define BDRLDEF static
#else
#define BDRLDEF extern
#endif

#if !defined(BDR_MAX_DELTA)
#define BDR_MAX_DELTA 0.1f
#endif

#if !defined(BDR_LOOP_UPDATE_INTERVAL)
#define BDR_LOOP_UPDATE_INTERVAL 0.02
#endif

#if !defined(BDR_LOOP_RENDER_INTERVAL)
#define BDR_LOOP_RENDER_INTERVAL 0.0
#endif

// Allow overriding the names of the functions required by the update function.
#if !defined(BDR_LOOP_FIXED_UPDATE)
#define BDR_LOOP_FIXED_UPDATE FixedUpdate
#endif

#if !defined(BDR_LOOP_UPDATE)
#define BDR_LOOP_UPDATE Update
#endif

#if !defined(BDR_LOOP_DRAW)
#define BDR_LOOP_DRAW Draw
#endif

#if !defined(BDR_LOOP_CHECK_TRIGGERS)
#define BDR_LOOP_CHECK_TRIGGERS CheckTriggers
#endif

// The update function itself.
#if !defined(BDR_LOOP_UPDATE_DRAW_FRAME)
#define BDR_LOOP_UPDATE_DRAW_FRAME UpdateDrawFrame
#define IMPLEMENT_UPDATE_DRAW_FRAME
#endif

// The function that tells the loop to quit.
#if !defined(BDR_LOOP_SHOULD_QUIT) && !defined(PLATFORM_WEB) && !defined(EMSCRIPTEN)
#define BDR_LOOP_SHOULD_QUIT QuitLoop
#define IMPLEMENT_QUIT_LOOP
#endif

// The main loop function.
#if !defined(BDR_LOOP_RUN_MAIN_LOOP)
#define BDR_LOOP_RUN_MAIN_LOOP RunMainLoop
#endif

// Forward declare the functions used by the update function and the main loop function.
BDRLDEF void BDR_LOOP_FIXED_UPDATE(void);
BDRLDEF void BDR_LOOP_UPDATE(double elapsed);
BDRLDEF void BDR_LOOP_CHECK_TRIGGERS(void);
BDRLDEF void BDR_LOOP_DRAW(double alpha);
BDRLDEF void BDR_LOOP_UPDATE_DRAW_FRAME(void);

#if !defined(PLATFORM_WEB) || !defined(EMSCRIPTEN)
BDRLDEF bool BDR_LOOP_SHOULD_QUIT(void);
#endif

BDRLDEF double GetUpdateInterval(void);
BDRLDEF void SetUpdateInterval(double seconds);

BDRLDEF double GetRenderInterval(void);
BDRLDEF void SetRenderInterval(double seconds);

#ifdef __cplusplus
}
#endif

// --- Implementation --------------------------------------------------------------------------------------------------------------

#if defined(BDR_LOOP_IMPLEMENTATION)

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

static struct
{
    double t;              // Physics time.
    double updateInterval; // Desired fixed update interval (seconds).
    double renderInterval; // Desired renderer interval (seconds).
    double lastTime;       // When did we last try a fixed update?
    double accumulator;    // How much time was left over?
    double alpha;          // How far into the next fixed update are we?
    double lastDrawTime;   // When did we last draw?
} timing = {.t = 0.0,
            .updateInterval = BDR_LOOP_UPDATE_INTERVAL,
            .renderInterval = BDR_LOOP_RENDER_INTERVAL,
            .lastTime = 0.0,
            .accumulator = 0.0,
            .alpha = 0.0,
            .lastDrawTime = 0.0};

#if defined(IMPLEMENT_UPDATE_DRAW_FRAME)
#undef IMPLEMENT_UPDATE_DRAW_FRAME

BDRLDEF double GetUpdateInterval(void)
{
    return timing.updateInterval;
}

BDRLDEF void SetUpdateInterval(double seconds)
{
    timing.updateInterval = seconds;
}

BDRLDEF double GetRenderInterval(void)
{
    return timing.renderInterval;
}

BDRLDEF void SetRenderInterval(double seconds)
{
    timing.renderInterval = seconds;
}

BDRLDEF void BDR_LOOP_UPDATE_DRAW_FRAME(void)
{
#if defined(__EMSCRIPTEN__)
    // For web builds we only need to check for edge-triggered events once per frame.
    BDR_LOOP_CHECK_TRIGGERS();
#endif

    // See https://gafferongames.com/post/fix_your_timestep/ for more details on how this works.
    double now = GetTime();
    const double delta = fmin(now - timing.lastTime, BDR_MAX_DELTA);

    // Fixed timestep updates.
    timing.lastTime = now;
    timing.accumulator += delta;
    while (timing.accumulator >= timing.updateInterval)
    {
        BDR_LOOP_FIXED_UPDATE();
        timing.t += timing.updateInterval;
        timing.accumulator -= timing.updateInterval;
    }
    timing.alpha = timing.accumulator / timing.updateInterval;

    // Draw, potentially capping the frame rate.
    now = GetTime();
    double drawInterval = now - timing.lastDrawTime;
    if (drawInterval >= timing.renderInterval)
    {
        // Per-frame update.
        BDR_LOOP_UPDATE(fmin(drawInterval, BDR_MAX_DELTA));

        // Draw the frame.
        BDR_LOOP_DRAW(timing.alpha);
        timing.lastDrawTime = now;

#if !defined(__EMSCRIPTEN__)
        // Raylib updates its input events in EndDrawing(), so we check for edge-triggered input events such as key-presses here so
        // that we don't miss them when we have a high frame rate, and so that we don't check for them when nothing has changed.
        BDR_LOOP_CHECK_TRIGGERS();
#endif
    }
}

#endif // IMPLEMENT_UPDATE_DRAW_FRAME

#if defined(IMPLEMENT_QUIT_LOOP)
#undef IMPLEMENT_QUIT_LOOP

BDRLDEF bool BDR_LOOP_SHOULD_QUIT(void)
{
    return WindowShouldClose();
}

#endif // IMPLEMENT_QUIT_LOOP

// Run the main loop until told to quit.
BDRLDEF void BDR_LOOP_RUN_MAIN_LOOP(void)
{
    SetUpdateInterval(GetUpdateInterval());
    SetRenderInterval(GetRenderInterval());

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(BDR_LOOP_UPDATE_DRAW_FRAME, 0, 1);
#else
    while (!BDR_LOOP_SHOULD_QUIT())
    {
        BDR_LOOP_UPDATE_DRAW_FRAME();
    }
#endif
}

#endif // BDR_LOOP_IMPLEMENTATION
