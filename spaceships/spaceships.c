#include "spaceships.h"

#include "raylib.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#include <math.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define UPDATE_FPS 50

#define SLOW_FPS 60
#define FAST_FPS 240

#define MAX_DELTA 0.1f

#if defined(EMSCRIPTEN)
#define CAP_FRAME_RATE 0
#else
#define CAP_FRAME_RATE 1
#endif

typedef enum
{
    NONE,
    MENU,
    CONTROLLER_SELECTION,
    PLAYING,
    QUIT
} Screen;

static struct
{
    double t;              // Physics time.
    double updateInterval; // Desired fixed update interval (seconds).
    double renderInterval; // Desired renderer interval (seconds).
    double lastTime;       // When did we last try a fixed update?
    double accumulator;    // How much time was left over?
    double alpha;          // How far into the next fixed update are we?
    double lastDrawTime;   // When did we last draw?
} timing;

static int renderFps = FAST_FPS;
static Screen currentScreen;

void InitScreens(void)
{
    currentScreen = NONE;
}

static void InitTiming(void)
{
    timing.t = 0.0;
    timing.updateInterval = 1.0 / UPDATE_FPS;
#if defined(CAP_FRAME_RATE)
    timing.renderInterval = 1.0 / renderFps;
#else
    timing.renderInterval = 0.0;
#endif
    timing.accumulator = 0.0;
    timing.alpha = 0.0;
    timing.lastTime = GetTime();
    timing.lastDrawTime = timing.lastTime;
}

static void FixedUpdate(void)
{
    switch (currentScreen)
    {
    case NONE:
        currentScreen = MENU;
        InitMenuScreen();
        break;
    case MENU:
        UpdateMenuScreen();
        if (IsStartedMenuScreen())
        {
            FinishMenuScreen();
            currentScreen = CONTROLLER_SELECTION;
            InitControlsScreen();
        }
        else if (IsCancelledMenuScreen())
        {
            FinishMenuScreen();
            currentScreen = QUIT;
        }
        break;
    case CONTROLLER_SELECTION:
        UpdateControlsScreen();
        if (IsStartedControlsScreen())
        {
            FinishControlsScreen();
            currentScreen = PLAYING;
            ControllerId controllers[MAX_PLAYERS];
            int numPlayers = GetNumberOfPlayers();
            for (int i = 0; i < numPlayers; i++)
            {
                controllers[i] = GetControllerAssignment(i);
            }
            InitPlayingScreen(numPlayers, controllers);
        }
        else if (IsCancelledControlsScreen())
        {
            FinishControlsScreen();
            currentScreen = MENU;
            InitMenuScreen();
        }
        break;
    case PLAYING:
        UpdatePlayingScreen();
        if (IsCancelledPlayingScreen())
        {
            FinishPlayingScreen();
            currentScreen = MENU;
            InitMenuScreen();
        }
        break;
    default:
        break;
    }
}

static void Update(double elapsed)
{
    (void)elapsed;
}

static void CheckTriggers(void)
{
    if (IsKeyPressed(KEY_F11))
    {
        ToggleFullscreen();
    }

    if (IsKeyPressed(KEY_F10))
    {
        renderFps = (renderFps == FAST_FPS) ? SLOW_FPS : FAST_FPS;
        SetTargetFPS(renderFps);
    }

    switch (currentScreen)
    {
    case MENU:
        CheckTriggersMenuScreen();
        break;
    case CONTROLLER_SELECTION:
        CheckTriggersControlsScreen();
        break;
    case PLAYING:
        CheckTriggersPlayingScreen();
        break;
    default:
        break;
    }
}

static void Draw(double alpha)
{
    switch (currentScreen)
    {
    case MENU:
        DrawMenuScreen(alpha);
        break;
    case CONTROLLER_SELECTION:
        DrawControlsScreen(alpha);
        break;
    case PLAYING:
        DrawPlayingScreen(alpha);
        break;
    default:
        break;
    }
}

static void UpdateDrawFrame(void)
{
#if defined(__EMSCRIPTEN__)
    // For web builds we only need to check for edge-triggered events once per frame.
    CheckTriggers();
#endif

    // See https://gafferongames.com/post/fix_your_timestep/ for more details on how this works.
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

static void Unload(void)
{
    // Workaround until I make a decision on resource management.
    switch (currentScreen)
    {
    case MENU:
        FinishMenuScreen();
        break;
    case CONTROLLER_SELECTION:
        FinishControlsScreen();
        break;
    case PLAYING:
        FinishPlayingScreen();
    default:
        break;
    }
}

int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spaceships");
    SetTargetFPS(renderFps);
    SetExitKey(0);

    InitTiming();
    InitScreens();

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (currentScreen != QUIT && !WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif

    Unload();
    CloseWindow();

    return 0;
}
