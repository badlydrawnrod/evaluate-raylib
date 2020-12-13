#include "config.h"
#include "controllers.h"
#include "menu.h"
#include "playing.h"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "screens.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

namespace my
{
    constexpr int renderFps = 60;
    constexpr int updateFps = 60;
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

#if defined(EMSCRIPTEN)
    bool capFrameRate = false;
#else
    bool capFrameRate = true;
#endif

    struct
    {
        double t = 0.0;                                               // Now.
        double updateInterval = 1.0 / updateFps;                      // Desired update interval (seconds).
        double renderInterval = capFrameRate ? 1.0 / renderFps : 0.0; // Desired renderer interval (seconds).
        double lastTime = GetTime();                                  // When did we last try an update/draw?
        double accumulator = 0.0;                                     // What was left over.
        double lastDrawTime = GetTime();                              // When did we last draw?
    } timing;

    void FixedUpdate()
    {
        ScreenState state = MENU;
        switch (screenState)
        {
        case MENU:
            state = menu::FixedUpdate();
            break;
        case PLAYING:
            state = playing::FixedUpdate();
            break;
        default:
            break;
        }

        if (state != screenState)
        {
            if (state == PLAYING)
            {
                ControllerId controllers[4];
                for (int i = 0; i < menu::numPlayers; i++)
                {
                    controllers[i] = menu::GetControllerAssignment(i);
                }
                playing::Start(menu::numPlayers, controllers);
            }

            if (state == MENU)
            {
                menu::Start();
            }
            screenState = state;
        }
    }

    void Update()
    {
    }

    void HandleEdgeTriggeredEvents()
    {
        switch (screenState)
        {
        case PLAYING:
            playing::HandleEdgeTriggeredEvents();
            break;
        default:
            break;
        }
    }

    void Draw()
    {
        ClearBackground(BLACK);
        BeginDrawing();
        switch (screenState)
        {
        case MENU:
            menu::Draw();
            break;
        case PLAYING:
            playing::Draw();
            break;
        default:
            break;
        }
        EndDrawing();
    }

    void UpdateDrawFrame()
    {
        if (IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        // Update using: https://gafferongames.com/post/fix_your_timestep/
        double now = GetTime();
        double delta = now - timing.lastTime;
        if (delta >= 0.1)
        {
            delta = 0.1;
        }
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

        Update();

        // Draw, potentially capping the frame rate.
        now = GetTime();
        double drawInterval = now - timing.lastDrawTime;
        if (drawInterval >= timing.renderInterval)
        {
            timing.lastDrawTime = now;
            Draw(); // Make like a gunslinger.
#if !defined(EMSCRIPTEN)
            HandleEdgeTriggeredEvents();
#endif
        }
    }
} // namespace my

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(my::screenWidth, my::screenHeight, "Spaceships");
    SetTargetFPS(my::renderFps);

    GuiLoadStyle("assets/terminal/terminal.rgs");
    scoreFont = LoadFont("assets/terminal/Mecha.ttf");

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(my::UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose())
    {
        my::UpdateDrawFrame();
    }
#endif
    CloseWindow();

    return 0;
}
