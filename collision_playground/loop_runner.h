/**
 * A header-only implementation of a Raylib game loop that handles both fixed timestep and per-frame updates.
 */
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
// Set BDR_LOOP_STATIC if you want the functions in this header to have static linkage rather than external linkage.
#define BDRLDEF extern
#endif

#if !defined(BDR_LOOP_MAX_FIXED_UPDATE_INTERVAL_SECONDS)
// The largest possible interval between fixed timestep updates.
#define BDR_LOOP_MAX_FIXED_UPDATE_INTERVAL_SECONDS 0.1
#endif

#if !defined(BDR_LOOP_FIXED_UPDATE_INTERVAL_SECONDS)
// The update interval for fixed timestep updates.
#define BDR_LOOP_FIXED_UPDATE_INTERVAL_SECONDS 0.02
#endif

#if !defined(BDR_LOOP_DRAW_INTERVAL_SECONDS)
// The render interval (1/fps).
#define BDR_LOOP_DRAW_INTERVAL_SECONDS 0.0
#endif

#if !defined(BDR_LOOP_FIXED_UPDATE)
// Define this if you want to use your own fixed timestep update function.
#define BDR_LOOP_FIXED_UPDATE FixedUpdate
#define IMPLEMENT_FIXED_UPDATE
#endif

#if !defined(BDR_LOOP_UPDATE)
// Define this if you want to use your own per-frame update function.
#define BDR_LOOP_UPDATE BdrLoopUpdate
#define IMPLEMENT_LOOP_UPDATE
#endif

#if !defined(BDR_LOOP_DRAW)
// Define this if you want to use your own draw function.
#define BDR_LOOP_DRAW Draw
#define IMPLEMENT_LOOP_DRAW
#endif

#if !defined(BDR_LOOP_CHECK_TRIGGERS)
// Define this if you want to use your own check triggers function.
#define BDR_LOOP_CHECK_TRIGGERS BdrLoopCheckTriggers
#define IMPLEMENT_CHECK_TRIGGERS
#endif

#if !defined(BDR_LOOP_SHOULD_QUIT) && !defined(PLATFORM_WEB) && !defined(EMSCRIPTEN)
// Define this if you want to provide a different function to tell the main loop to quit. Not valid on web plaforms or Emscripten.
#define BDR_LOOP_SHOULD_QUIT BdrLoopQuitLoop
#define IMPLEMENT_QUIT_LOOP
#endif

#if !defined(BDR_LOOP_UPDATE_DRAW_FRAME)
// Define this if you want to use your own main loop update function.
#define BDR_LOOP_UPDATE_DRAW_FRAME BdrLoopUpdateDrawFrame
#define IMPLEMENT_UPDATE_DRAW_FRAME
#endif

#if !defined(BDR_LOOP_RUN_MAIN_LOOP)
// Define this if you want to provide your own main loop function.
#define BDR_LOOP_RUN_MAIN_LOOP RunMainLoop
#define IMPLEMENT_RUN_MAIN_LOOP
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

#if defined(IMPLEMENT_UPDATE_DRAW_FRAME)
BDRLDEF double GetUpdateInterval(void);
BDRLDEF void SetUpdateInterval(double seconds);

BDRLDEF double GetDrawInterval(void);
BDRLDEF void SetDrawInterval(double seconds);
#endif // IMPLEMENT_UPDATE_DRAW_FRAME

#ifdef __cplusplus
}
#endif

// --- Implementation --------------------------------------------------------------------------------------------------------------

#if defined(BDR_LOOP_IMPLEMENTATION)

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(IMPLEMENT_FIXED_UPDATE)
#undef IMPLEMENT_FIXED_UPDATE
/**
 * The fixed update function, called once per fixed timestep update interval. Define BDR_LOOP_FIXED_UPDATE if you want to provide
 * your own.
 */
BDRLDEF void FixedUpdate(void)
{
}
#endif // IMPLEMENT_FIXED_UDPATE

#if defined(IMPLEMENT_LOOP_UPDATE)
#undef IMPLEMENT_LOOP_UPDATE
/**
 * The per-frame update function, called once per frame. Define BDR_LOOP_UPDATE to provide your own.
 * @param elapsed the time in seconds since the last call to this function.
 */
BDRLDEF void BdrLoopUpdate(double elapsed)
{
    (void)elapsed;
}
#endif // IMPLEMENT_LOOP_UPDATE

#if defined(IMPLEMENT_LOOP_DRAW)
/**
 * Called by the main loop whenever drawing is required. Define BDR_LOOP_DRAW to provide your own (and you probably should if you
 * don't want to stare at a largely empty window).
 *
 * @param alpha a value from 0.0 to 1.0 indicating how much into the next frame we are. Useful for interpolation.
 */
void BdrLoopDraw(double alpha)
{
    (void)alpha;
    ClearBackground(BLACK);
    BeginDrawing();
    DrawFPS(4, 4);
    EndDrawing();
}
#endif // IMPLEMENT_LOOP_DRAW

#if defined(IMPLEMENT_QUIT_LOOP)
#undef IMPLEMENT_QUIT_LOOP
/**
 * The main loop calls this to check if it should quit.
 *
 * @return true if the main loop should quit, otherwise false.
 */
BDRLDEF bool BdrLoopQuitLoop(void)
{
    return WindowShouldClose();
}
#endif // IMPLEMENT_QUIT_LOOP

#ifdef IMPLEMENT_CHECK_TRIGGERS
#undef IMPLEMENT_CHECK_TRIGGERS
/**
 * Raylib updates its input events in EndDrawing(), so we check for edge-triggered input events such as key-presses here so that we
 * don't miss them when we have a high frame rate, and so that we don't end up checking for them when nothing has changed.
 */
BDRLDEF void BdrLoopCheckTriggers(void)
{
}
#endif // IMPLEMENT_CHECK_TRIGGERS

#if defined(IMPLEMENT_UPDATE_DRAW_FRAME)
#undef IMPLEMENT_UPDATE_DRAW_FRAME
static struct
{
    double t;              // Current physics time.
    double updateInterval; // Desired fixed update interval (seconds).
    double drawInterval;   // Desired draw interval (seconds).
    double lastTime;       // When did we last try a fixed update?
    double accumulator;    // How much time was left over?
    double alpha;          // How far into the next fixed update are we?
    double lastDrawTime;   // When did we last draw?
} timing = {.t = 0.0,
            .updateInterval = BDR_LOOP_FIXED_UPDATE_INTERVAL_SECONDS,
            .drawInterval = BDR_LOOP_DRAW_INTERVAL_SECONDS,
            .lastTime = 0.0,
            .accumulator = 0.0,
            .alpha = 0.0,
            .lastDrawTime = 0.0};

BDRLDEF double GetUpdateInterval(void)
{
    return timing.updateInterval;
}

BDRLDEF void SetUpdateInterval(double seconds)
{
    timing.updateInterval = seconds;
}

BDRLDEF double GetDrawInterval(void)
{
    return timing.drawInterval;
}

BDRLDEF void SetDrawInterval(double seconds)
{
    timing.drawInterval = seconds;
}

/**
 * The main update driver.
 */
BDRLDEF void BdrLoopUpdateDrawFrame(void)
{
#if defined(__EMSCRIPTEN__)
    // For web builds we only need to check for edge-triggered events once per frame.
    BDR_LOOP_CHECK_TRIGGERS();
#endif

    // See https://gafferongames.com/post/fix_your_timestep/ for more details on how this works.
    double now = GetTime();
    const double delta = fmin(now - timing.lastTime, BDR_LOOP_MAX_FIXED_UPDATE_INTERVAL_SECONDS);

    // Perform fixed timestep updates by calling the FixedUpdate() function.
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
    if (drawInterval >= timing.drawInterval)
    {
        // Perform a per-frame update.
        BDR_LOOP_UPDATE(fmin(drawInterval, BDR_LOOP_MAX_FIXED_UPDATE_INTERVAL_SECONDS));

        // Draw the frame.
        BDR_LOOP_DRAW(timing.alpha);
        timing.lastDrawTime = now;

#if !defined(__EMSCRIPTEN__)
        // Raylib updates its input events in EndDrawing() which we're calling from BDR_LOOP_DRAW(), so check for edge-triggered
        // input events such as key-presses here so that we don't miss them when we have a high frame rate, and so that we don't
        // check for them when they can't possibly have changed.
        BDR_LOOP_CHECK_TRIGGERS();
#endif
    }
}
#endif // IMPLEMENT_UPDATE_DRAW_FRAME

#if defined(IMPLEMENT_RUN_MAIN_LOOP)
#undef IMPLEMENT_RUN_MAIN_LOOP
/**
 * Runs the main loop until told to quit.
 */
BDRLDEF void RunMainLoop(void)
{
    SetUpdateInterval(GetUpdateInterval());
    SetDrawInterval(GetDrawInterval());

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(BDR_LOOP_UPDATE_DRAW_FRAME, 0, 1);
#else
    while (!BDR_LOOP_SHOULD_QUIT())
    {
        BDR_LOOP_UPDATE_DRAW_FRAME();
    }
#endif
}
#endif // IMPLEMENT_RUN_MAIN_LOOP

#ifdef __cplusplus
}
#endif

#endif // BDR_LOOP_IMPLEMENTATION
