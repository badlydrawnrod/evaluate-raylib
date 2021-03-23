#define BDR_LOOP_IMPLEMENTATION
#define BDR_LOOP_SHOULD_QUIT ShouldQuit
#include "spaced.h"

#include "bdr/loop.h"
#include "raylib.h"

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
            InitPlayingScreen();
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
        // Draw, so that we continue to process events.
        BeginDrawing();
        ClearBackground(DARKGRAY);
        EndDrawing();
        break;
    }
}

#if !defined(PLATFORM_WEB) && !defined(EMSCRIPTEN)
BDRLDEF bool ShouldQuit(void)
{
    return WindowShouldClose() || currentScreen == QUIT;
}
#endif

int main(void)
{
#if !defined(NO_MSAA)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#endif
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spaced!");
    SetTargetFPS(renderFps);
    SetExitKey(0);
    InitScreens();

    RunMainLoop();

    CloseWindow();

    return 0;
}
