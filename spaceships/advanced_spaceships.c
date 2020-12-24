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
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Advanced Spaceships");
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
