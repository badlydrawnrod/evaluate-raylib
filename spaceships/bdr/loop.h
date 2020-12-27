#pragma once

#include "raylib.h"

#include <math.h>
#include <stdbool.h>

#if defined(BDR_LOOP_STATIC)
#define BDRLDEF static
#else
#define BDRLDEF extern
#endif

#if !defined(BDR_MAX_DELTA)
#define BDR_MAX_DELTA 0.1f
#endif

// Allow overriding the names of the functions required by the update function.
#if !defined(BDR_FIXED_UPDATE)
#define BDR_FIXED_UPDATE FixedUpdate
#endif

#if !defined(BDR_UPDATE)
#define BDR_UPDATE Update
#endif

#if !defined(BDR_DRAW)
#define BDR_DRAW Draw
#endif

#if !defined(BDR_CHECK_TRIGGERS)
#define BDR_CHECK_TRIGGERS CheckTriggers
#endif

// The update function itself.
#if !defined(BDR_UPDATE_DRAW_FRAME)
#define BDR_UPDATE_DRAW_FRAME UpdateDrawFrame
#define IMPLEMENT_UPDATE_DRAW_FRAME
#endif

// The function that tells the loop to quit.
#if !defined(BDR_QUIT_LOOP) && !defined(PLATFORM_WEB) && !defined(EMSCRIPTEN)
#define BDR_QUIT_LOOP QuitLoop
#define IMPLEMENT_QUIT_LOOP
#endif

// The main loop function.
#if !defined(BDR_MAIN_LOOP)
#define BDR_MAIN_LOOP RunMainLoop
#endif

#if !defined(BDR_LOOP_STATIC)
BDRLDEF struct _bdr_loop_timing bdr_loopTiming;
#endif

// Forward declare the functions used by the update function and the main loop function.
BDRLDEF void BDR_FIXED_UPDATE(void);
BDRLDEF void BDR_UPDATE(double elapsed);
BDRLDEF void BDR_CHECK_TRIGGERS(void);
BDRLDEF void BDR_DRAW(double alpha);
BDRLDEF void BDR_UPDATE_DRAW_FRAME(void);

#if !defined(PLATFORM_WEB) || !defined(EMSCRIPTEN)
BDRLDEF bool BDR_QUIT_LOOP(void);
#endif

#if defined(BDR_LOOP_IMPLEMENTATION)

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

struct _bdr_loop_timing
{
    double t;              // Physics time.
    double updateInterval; // Desired fixed update interval (seconds).
    double renderInterval; // Desired renderer interval (seconds).
    double lastTime;       // When did we last try a fixed update?
    double accumulator;    // How much time was left over?
    double alpha;          // How far into the next fixed update are we?
    double lastDrawTime;   // When did we last draw?
};

struct _bdr_loop_timing bdr_loopTiming;

#if defined(IMPLEMENT_UPDATE_DRAW_FRAME)
#undef IMPLEMENT_UPDATE_DRAW_FRAME

BDRLDEF void BDR_UPDATE_DRAW_FRAME(void)
{
#if defined(__EMSCRIPTEN__)
    // For web builds we only need to check for edge-triggered events once per frame.
    BDR_CHECK_TRIGGERS();
#endif

    // See https://gafferongames.com/post/fix_your_timestep/ for more details on how this works.
    double now = GetTime();
    const double delta = fmin(now - bdr_loopTiming.lastTime, BDR_MAX_DELTA);

    // Fixed timestep updates.
    bdr_loopTiming.lastTime = now;
    bdr_loopTiming.accumulator += delta;
    while (bdr_loopTiming.accumulator >= bdr_loopTiming.updateInterval)
    {
        BDR_FIXED_UPDATE();
        bdr_loopTiming.t += bdr_loopTiming.updateInterval;
        bdr_loopTiming.accumulator -= bdr_loopTiming.updateInterval;
    }
    bdr_loopTiming.alpha = bdr_loopTiming.accumulator / bdr_loopTiming.updateInterval;

    // Draw, potentially capping the frame rate.
    now = GetTime();
    double drawInterval = now - bdr_loopTiming.lastDrawTime;
    if (drawInterval >= bdr_loopTiming.renderInterval)
    {
        // Per-frame update.
        BDR_UPDATE(fmin(drawInterval, BDR_MAX_DELTA));

        // Draw the frame.
        BDR_DRAW(bdr_loopTiming.alpha);
        bdr_loopTiming.lastDrawTime = now;

#if !defined(__EMSCRIPTEN__)
        // Raylib updates its input events in EndDrawing(), so we check for edge-triggered input events such as key-presses here so
        // that we don't miss them when we have a high frame rate.
        BDR_CHECK_TRIGGERS();
#endif
    }
}

#endif // IMPLEMENT_UPDATE_DRAW_FRAME

#if defined(IMPLEMENT_QUIT_LOOP)
#undef IMPLEMENT_QUIT_LOOP

BDRLDEF bool BDR_QUIT_LOOP(void)
{
    return WindowShouldClose();
}

#endif // IMPLEMENT_QUIT_LOOP

// Run the main loop until told to quit.
BDRLDEF void BDR_MAIN_LOOP(void)
{
#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(BDR_UPDATE_DRAW_FRAME, 0, 1);
#else
    while (!BDR_QUIT_LOOP())
    {
        BDR_UPDATE_DRAW_FRAME();
    }
#endif
}

#endif
