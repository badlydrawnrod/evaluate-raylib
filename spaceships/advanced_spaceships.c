#define BDR_LOOP_IMPLEMENTATION
#define BDR_QUIT_LOOP ShouldQuit
#include "bdr/loop.h"
#include "raylib.h"
#include "spaceships.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

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

static int renderFps = FAST_FPS;
static Screen currentScreen;

void InitScreens(void)
{
    currentScreen = NONE;
}

static void InitTiming(void)
{
    bdr_loopTiming.t = 0.0;
    bdr_loopTiming.updateInterval = 1.0 / UPDATE_FPS;
#if defined(CAP_FRAME_RATE)
    bdr_loopTiming.renderInterval = 1.0 / renderFps;
#else
    timing.renderInterval = 0.0;
#endif
    bdr_loopTiming.accumulator = 0.0;
    bdr_loopTiming.alpha = 0.0;
    bdr_loopTiming.lastTime = GetTime();
    bdr_loopTiming.lastDrawTime = bdr_loopTiming.lastTime;
}

void FixedUpdate(void)
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

void Update(double elapsed)
{
    (void)elapsed;
}

void CheckTriggers(void)
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

void Draw(double alpha)
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

#if !defined(PLATFORM_WEB) && !defined(EMSCRIPTEN)
BDRLDEF bool ShouldQuit(void)
{
    return currentScreen == QUIT || WindowShouldClose();
}
#endif

int main(void)
{
#if !defined(NO_MSAA)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#endif
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Advanced Spaceships ");
    SetTargetFPS(renderFps);
    SetExitKey(0);

    InitTiming();
    InitScreens();

    RunMainLoop();
    CloseWindow();

    return 0;
}
