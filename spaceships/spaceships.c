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

int renderFps = FAST_FPS;

#if defined(EMSCRIPTEN)
#define CAP_FRAME_RATE false
#else
#define CAP_FRAME_RATE true
#endif

typedef enum
{
    NONE,
    MENU,
    CONTROLLER_SELECTION,
    PLAYING
} Screen;

static Screen currentScreen;

void InitScreens(void)
{
    currentScreen = NONE;
}

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

void InitTiming(void)
{
    timing.t = 0.0;
    timing.updateInterval = 1.0 / UPDATE_FPS;
    timing.renderInterval = CAP_FRAME_RATE ? 1.0 / renderFps : 0.0;
    timing.accumulator = 0.0;
    timing.alpha = 0.0;
    timing.lastTime = GetTime();
    timing.lastDrawTime = timing.lastTime;
}

void FixedUpdate(void)
{
    switch (currentScreen)
    {
    case NONE:
        currentScreen = MENU;
        InitMenu();
        break;
    case MENU:
        UpdateMenu();
        if (IsStartingMenu())
        {
            FinishMenu();
            currentScreen = CONTROLLER_SELECTION;
            InitControls();
        }
        break;
    case CONTROLLER_SELECTION:
        UpdateControls();
        if (IsStartedControls())
        {
            FinishControls();
            currentScreen = PLAYING;
            ControllerId controllers[4];
            int numPlayers = GetNumberOfPlayers();
            for (int i = 0; i < numPlayers; i++)
            {
                controllers[i] = GetControllerAssignment(i);
            }
            InitPlaying(numPlayers, controllers);
        }
        else if (IsCancelledControls())
        {
            FinishControls();
            currentScreen = MENU;
            InitMenu();
        }
        break;
    case PLAYING:
        UpdatePlaying();
        if (IsCancelledPlaying())
        {
            FinishPlaying();
            currentScreen = MENU;
            InitMenu();
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
        CheckTriggersMenu();
        break;
    case CONTROLLER_SELECTION:
        CheckTriggersControls();
        break;
    case PLAYING:
        CheckTriggersPlaying();
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
        DrawMenu(alpha);
        break;
    case CONTROLLER_SELECTION:
        DrawControls(alpha);
        break;
    case PLAYING:
        DrawPlaying(alpha);
        break;
    default:
        break;
    }
}

void UpdateDrawFrame(void)
{
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
#if defined(EMSCRIPTEN)
        CheckTriggers();
#endif
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
#if !defined(EMSCRIPTEN)
        CheckTriggers(); // As raylib updates input events in EndDrawing(), we update here so as to not miss
                         // anything edge triggered such as a keypress when we have a high frame rate.
#endif
    }
}

int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Spaceships");
    SetTargetFPS(renderFps);

    InitTiming();
    InitScreens();

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif

    // Workaround until I make a decision on resource management.
    switch (currentScreen)
    {
    case MENU:
        FinishMenu();
        break;
    case CONTROLLER_SELECTION:
        FinishControls();
        break;
    case PLAYING:
        FinishPlaying();
    default:
        break;
    }

    CloseWindow();

    return 0;
}
