#include "raygui.h"
#include "raylib.h"
#include "raymath.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#include <array>

namespace my
{
    constexpr int renderFps = 60;
    constexpr int updateFps = 60;
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    constexpr Vector2 shipLines[] = {{-1, 1}, {0, -1}, {1, 1}, {0, 0.5f}, {-1, 1}};
    constexpr Vector2 shotLines[] = {{0, -0.25f}, {0, 0.25f}};

    constexpr float shipScale = 16.0f;
    constexpr float shipOverlap = 2 * shipScale;
    constexpr float maxRotationSpeed = 4.0f;
    constexpr float speed = 0.1f;
    constexpr float shotSpeed = 6.0f;
    constexpr float shotDuration = 90.0f;
    constexpr float shipCollisionRadius = shipScale;
    constexpr float shotCollisionRadius = shipScale * 0.5f;

    Font scoreFont;

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

    using Position = Vector2;
    using Velocity = Vector2;
    using Heading = float;

    enum ControllerId
    {
        CONTROLLER_UNASSIGNED = -1,
        CONTROLLER_KEYBOARD1,
        CONTROLLER_KEYBOARD2,
        CONTROLLER_GAMEPAD1,
        CONTROLLER_GAMEPAD2,
        CONTROLLER_GAMEPAD3,
        CONTROLLER_GAMEPAD4
    };

    enum ScreenState
    {
        INIT,
        MENU,
        PLAYING
    };

    ScreenState screenState{INIT};

    namespace playing
    {
        bool IsControllerThrustDown(int controller)
        {
            switch (controller)
            {
            case CONTROLLER_GAMEPAD1:
                return IsGamepadButtonDown(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
            case CONTROLLER_GAMEPAD2:
                return IsGamepadButtonDown(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
            case CONTROLLER_GAMEPAD3:
                return IsGamepadButtonDown(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
            case CONTROLLER_GAMEPAD4:
                return IsGamepadButtonDown(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
            case CONTROLLER_KEYBOARD1:
                return IsKeyDown(KEY_W);
            case CONTROLLER_KEYBOARD2:
                return IsKeyDown(KEY_UP);
            default:
                return false;
            }
        }

        bool IsControllerFirePressed(int controller)
        {
            switch (controller)
            {
            case CONTROLLER_GAMEPAD1:
                return IsGamepadButtonPressed(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
            case CONTROLLER_GAMEPAD2:
                return IsGamepadButtonPressed(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
            case CONTROLLER_GAMEPAD3:
                return IsGamepadButtonPressed(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
            case CONTROLLER_GAMEPAD4:
                return IsGamepadButtonPressed(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
            case CONTROLLER_KEYBOARD1:
                return IsKeyPressed(KEY_SPACE);
            case CONTROLLER_KEYBOARD2:
                return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
            default:
                return false;
            }
        }

        float GetControllerTurnRate(int controller)
        {
            switch (controller)
            {
            case CONTROLLER_GAMEPAD1:
                return GetGamepadAxisMovement(GAMEPAD_PLAYER1, GAMEPAD_AXIS_LEFT_X);
            case CONTROLLER_GAMEPAD2:
                return GetGamepadAxisMovement(GAMEPAD_PLAYER2, GAMEPAD_AXIS_LEFT_X);
            case CONTROLLER_GAMEPAD3:
                return GetGamepadAxisMovement(GAMEPAD_PLAYER3, GAMEPAD_AXIS_LEFT_X);
            case CONTROLLER_GAMEPAD4:
                return GetGamepadAxisMovement(GAMEPAD_PLAYER4, GAMEPAD_AXIS_LEFT_X);
            case CONTROLLER_KEYBOARD1:
                return (IsKeyDown(KEY_D) ? 1.0f : 0.0f) - (IsKeyDown(KEY_A) ? 1.0f : 0.0f);
            case CONTROLLER_KEYBOARD2:
                return (IsKeyDown(KEY_RIGHT) ? 1.0f : 0.0f) - (IsKeyDown(KEY_LEFT) ? 1.0f : 0.0f);
            default:
                return 0.0f;
            }
        }

        struct Ship
        {
            bool alive;
            int player;
            Position pos;
            Velocity vel;
            Heading heading;
            ControllerId controller;
            int index;
        };

        struct Shot
        {
            int alive;
            Position pos;
            Velocity vel;
            Heading heading;
        };

        constexpr int maxPlayers = 4;

        std::array<Ship, maxPlayers> ships;
        int numPlayers = 0;

        constexpr int shotsPerPlayer = 5;
        using Shots = std::array<Shot, maxPlayers * shotsPerPlayer>;
        Shots shots;

        constexpr Color shipColours[] = {GREEN, YELLOW, PINK, SKYBLUE};

        void Start(int players, const ControllerId* controllers)
        {
            for (int i = 0; i < ships.size(); i++)
            {
                auto& ship = ships[i];
                ship.alive = false;
                ship.index = i;
            }

            numPlayers = players;
            for (int i = 0; i < players; i++)
            {
                float angle = i * (2 * 3.141592654f) / numPlayers;
                ships[i].controller = controllers[i];
                ships[i].alive = true;
                ships[i].pos.x = screenWidth / 2.0f + cosf(angle) * screenHeight / 3;
                ships[i].pos.y = screenHeight / 2.0f + sinf(angle) * screenHeight / 3;
                ships[i].heading = RAD2DEG * atan2f(screenHeight / 2.0f - ships[i].pos.y, screenWidth / 2.0f - ships[i].pos.x);
            }

            for (auto& shot : shots)
            {
                shot.alive = false;
            }
        }

        void Move(Position& pos, Velocity vel)
        {
            pos = Vector2Add(pos, vel);

            // Wrap the position around the play area.
            if (pos.x >= screenWidth)
            {
                pos.x -= screenWidth;
            }
            if (pos.x < 0)
            {
                pos.x += screenWidth;
            }
            if (pos.y >= screenHeight)
            {
                pos.y -= screenHeight;
            }
            if (pos.y < 0)
            {
                pos.y += screenHeight;
            }
        }

        void Move(Ship& ship)
        {
            Move(ship.pos, ship.vel);
        }

        void Move(Shot& shot)
        {
            Move(shot.pos, shot.vel);
        }

        void Collide(Ship& ship, Shot& shot)
        {
            if (CheckCollisionCircles(ship.pos, shipCollisionRadius, shot.pos, shotCollisionRadius))
            {
                ship.alive = false;
                shot.alive = 0;
            }
        }

        void Collide(Ship& ship1, Ship& ship2)
        {
            if (!ship1.alive || !ship2.alive)
            {
                return;
            }

            if (CheckCollisionCircles(ship1.pos, shipCollisionRadius, ship2.pos, shipCollisionRadius))
            {
                ship1.alive = false;
                ship2.alive = false;
            }
        }

        void Collide(Ship& ship, Shots::iterator start, Shots::iterator end)
        {
            if (!ship.alive)
            {
                return;
            }

            for (auto shotIt = start; shotIt != end; ++shotIt)
            {
                if (shotIt->alive > 0)
                {
                    Collide(ship, *shotIt);
                }
            }
        }

        void Update(Ship& ship)
        {
            if (!ship.alive)
            {
                return;
            }

            // Rotate the ship.
            float axis = GetControllerTurnRate(ship.controller);
            ship.heading += axis * maxRotationSpeed;

            // Accelerate the ship.
            if (IsControllerThrustDown(ship.controller))
            {
                ship.vel.x += cosf((ship.heading - 90) * DEG2RAD) * speed;
                ship.vel.y += sinf((ship.heading - 90) * DEG2RAD) * speed;
            }

            // Move the ship.
            Move(ship);
        }

        void CheckForFire(Ship& ship)
        {
            if (!ship.alive)
            {
                return;
            }

            // Fire.
            if (IsControllerFirePressed(ship.controller))
            {
                const int baseStart = ship.index * shotsPerPlayer;
                const int baseEnd = baseStart + shotsPerPlayer;
                for (int i = baseStart; i < baseEnd; i++)
                {
                    if (shots[i].alive == 0)
                    {
                        Shot& shot = shots[i];
                        shot.alive = shotDuration;
                        shot.heading = ship.heading;
                        Vector2 angle{cosf((ship.heading - 90) * DEG2RAD), sinf((ship.heading - 90) * DEG2RAD)};
                        shot.pos = Vector2Add(ship.pos, Vector2Scale(angle, shipScale));
                        shot.vel = Vector2Add(Vector2Scale(angle, shotSpeed), ship.vel);
                        break;
                    }
                }
            }
        }

        void Update(Shot& shot)
        {
            if (shot.alive == 0)
            {
                return;
            }
            Move(shot);
            --shot.alive;
        }

        ScreenState FixedUpdate()
        {
            for (int i = 0; i < numPlayers; i++)
            {
                auto& ship = ships[i];
                Update(ship);
            }
            for (auto& shot : shots)
            {
                Update(shot);
            }

            // Collide each player with the other players' shots.
            for (int i = 0; i < numPlayers; i++)
            {
                for (int j = 0; j < numPlayers; j++)
                {
                    if (j != i)
                    {
                        Collide(ships[i], shots.begin() + j * shotsPerPlayer, shots.begin() + j * shotsPerPlayer + shotsPerPlayer);
                    }
                }
            }

            // Collide each player with the other players.
            for (int i = 0; i < numPlayers - 1; i++)
            {
                for (int j = i + 1; j < numPlayers; j++)
                {
                    Collide(ships[i], ships[j]);
                }
            }

            return PLAYING;
        }

        void HandleEdgeTriggeredEvents()
        {
            for (auto& ship : ships)
            {
                CheckForFire(ship);
            }
        }

        void DrawShipAt(Vector2 pos, float heading, Color colour)
        {
            Vector2 points[5];
            for (int i = 0; i < 5; i++)
            {
                points[i] = Vector2Add(Vector2Scale(Vector2Rotate(shipLines[i], heading), shipScale), pos);
            }
            DrawLineStrip(points, 5, colour);
        }

        void DrawShotAt(Vector2 pos, float heading, Color colour)
        {
            Vector2 points[2];
            for (int i = 0; i < 2; i++)
            {
                points[i] = Vector2Add(Vector2Scale(Vector2Rotate(shotLines[i], heading), shipScale), pos);
            }
            DrawLineStrip(points, 2, colour);
        }

        void Draw(const Ship& ship)
        {
            const Vector2 pos = ship.pos;
            const float heading = ship.heading;

            // Which edges of the play area does the ship overlap?
            const bool overlapsTop = pos.y - shipOverlap < 0;
            const bool overlapsBottom = pos.y + shipOverlap >= screenHeight;
            const bool overlapsLeft = pos.x - shipOverlap < 0;
            const bool overlapsRight = pos.x + shipOverlap >= screenWidth;

            Color shipColour = shipColours[ship.index];

            DrawShipAt(pos, heading, shipColour);

            if (overlapsTop)
            {
                DrawShipAt(Vector2Add(pos, {0, screenHeight}), heading, shipColour);
            }
            if (overlapsBottom)
            {
                DrawShipAt(Vector2Add(pos, {0, screenHeight}), heading, shipColour);
            }
            if (overlapsLeft)
            {
                DrawShipAt(Vector2Add(pos, {screenWidth, 0}), heading, shipColour);
            }
            if (overlapsRight)
            {
                DrawShipAt(Vector2Add(pos, {-screenWidth, 0}), heading, shipColour);
            }
        }

        void Draw(const Shot& shot, Color colour)
        {
            DrawShotAt(shot.pos, shot.heading, colour);
        }

        void Draw()
        {
            for (int i = 0; i < numPlayers; i++)
            {
                const auto& ship = ships[i];
                if (ship.alive)
                {
                    Draw(ship);
                }
            }
            for (int i = 0; i < numPlayers * shotsPerPlayer; i++)
            {
                const auto& shot = shots[i];
                if (shot.alive > 0)
                {
                    Color colour = shipColours[i / shotsPerPlayer];
                    Draw(shot, colour);
                }
            }

            // TODO: draw this properly (or use something from raygui if that works better) based on the number of players.
            float fontSize = scoreFont.baseSize;
            DrawTextEx(scoreFont, "PLAYER 1", {8, 4}, fontSize, 2, shipColours[0]);
            DrawTextEx(scoreFont, " 000100", {8, 40}, fontSize, 2, shipColours[0]);
            DrawTextEx(scoreFont, "PLAYER 2", {screenWidth - 128, 4}, fontSize, 2, shipColours[1]);
            DrawTextEx(scoreFont, " 000200", {screenWidth - 128, 40}, fontSize, 2, shipColours[1]);
            if (numPlayers >= 3)
            {
                DrawTextEx(scoreFont, "PLAYER 3", {8, screenHeight - 76}, fontSize, 2, shipColours[2]);
                DrawTextEx(scoreFont, " 000300", {8, screenHeight - 40}, fontSize, 2, shipColours[2]);
            }
            if (numPlayers == 4)
            {
                DrawTextEx(scoreFont, "PLAYER 4", {screenWidth - 128, screenHeight - 76}, fontSize, 2, shipColours[3]);
                DrawTextEx(scoreFont, " 000400", {screenWidth - 128, screenHeight - 40}, fontSize, 2, shipColours[3]);
            }

            DrawFPS(screenWidth / 2 - 16, screenHeight - 24);
        }
    } // namespace playing

    namespace menu
    {
        enum AssignmentStatus
        {
            Unassigned,
            AssignedToPlayer,
            ConfirmedByPlayer
        };

        constexpr int maxPlayers = 4;
        int numPlayers = 0;

        // Player controllers and how they're assigned.
        struct PlayerController
        {
            ControllerId controller; // Which controller is assigned to this player, if any?
            AssignmentStatus status; // What's the status of this controller?
            char description[16];    // Description of the controller's status and assignment.
        } playerControllers[maxPlayers];

        int numControllers;
        struct ControllerInfo
        {
            ControllerId controller;
            const char* description;
        } controllers[6];

        constexpr struct
        {
            GamepadNumber gamepadNumber;
            ControllerId controllerId;
            const char* description;
        } gamepadDescriptors[4] = {{GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1, "Gamepad 1"},
                                   {GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2, "Gamepad 2"},
                                   {GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3, "Gamepad 3"},
                                   {GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4, "Gamepad 4"}};

        bool requestCancellation;

        void Start()
        {
            numPlayers = 0;
            numControllers = 2;
            requestCancellation = false;

            for (auto& playerController : playerControllers)
            {
                playerController.controller = CONTROLLER_UNASSIGNED;
                playerController.status = Unassigned;
                playerController.description[0] = '\0';
            }
        }

        ControllerId GetControllerAssignment(int player)
        {
            return playerControllers[player].controller;
        }

        AssignmentStatus GetControllerStatus(ControllerId controller)
        {
            for (int i = 0; i < numPlayers; i++)
            {
                const auto& controllerAssignment = playerControllers[i];
                if (controllerAssignment.controller == controller)
                {
                    return controllerAssignment.status;
                }
            }
            return Unassigned;
        }

        void AssignController(ControllerId controller)
        {
            for (int i = 0; i < numPlayers; i++)
            {
                auto& controllerAssignment = playerControllers[i];
                if (controllerAssignment.status == Unassigned)
                {
                    controllerAssignment.controller = controller;
                    controllerAssignment.status = AssignedToPlayer;
                    TextCopy(controllerAssignment.description, TextFormat("Assigned [P%d]", i + 1));
                    return;
                }
            }
        }

        void UnassignController(ControllerId controller)
        {
            for (int i = 0; i < numPlayers; i++)
            {
                auto& controllerAssignment = playerControllers[i];
                if (controllerAssignment.controller == controller && controllerAssignment.status != Unassigned)
                {
                    controllerAssignment.controller = CONTROLLER_UNASSIGNED;
                    controllerAssignment.status = Unassigned;
                    TextCopy(controllerAssignment.description, "");
                    return;
                }
            }
        }

        void ConfirmController(ControllerId controller)
        {
            for (int i = 0; i < numPlayers; i++)
            {
                auto& controllerAssignment = playerControllers[i];
                if (controllerAssignment.controller == controller && controllerAssignment.status == AssignedToPlayer)
                {
                    controllerAssignment.status = ConfirmedByPlayer;
                    TextCopy(controllerAssignment.description, TextFormat("Confirmed [P%d]", i + 1));
                }
            }
        }

        void UnconfirmController(ControllerId controller)
        {
            for (int i = 0; i < numPlayers; i++)
            {
                auto& controllerAssignment = playerControllers[i];
                if (controllerAssignment.controller == controller && controllerAssignment.status == ConfirmedByPlayer)
                {
                    controllerAssignment.status = AssignedToPlayer;
                    TextCopy(controllerAssignment.description, TextFormat("Assigned [P%d]", i + 1));
                }
            }
        }

        void CheckKeyboard(KeyboardKey selectKey, KeyboardKey cancelKey, ControllerId controller)
        {
            if (IsKeyReleased(selectKey))
            {
                switch (GetControllerStatus(controller))
                {
                case Unassigned:
                    AssignController(controller);
                    break;
                case AssignedToPlayer:
                    ConfirmController(controller);
                    break;
                default:
                    break;
                }
            }
            if (IsKeyReleased(cancelKey))
            {
                switch (GetControllerStatus(controller))
                {
                case ConfirmedByPlayer:
                    UnconfirmController(controller);
                    break;
                case AssignedToPlayer:
                    UnassignController(controller);
                    break;
                default:
                    requestCancellation = true;
                    break;
                }
            }
        }

        void CheckGamepad(GamepadNumber gamepad, ControllerId controller)
        {
            if (IsGamepadAvailable(gamepad))
            {
                if (IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                {
                    switch (GetControllerStatus(controller))
                    {
                    case Unassigned:
                        AssignController(controller);
                        break;
                    case AssignedToPlayer:
                        ConfirmController(controller);
                        break;
                    default:
                        break;
                    }
                }
                if (IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
                {
                    switch (GetControllerStatus(controller))
                    {
                    case ConfirmedByPlayer:
                        UnconfirmController(controller);
                        break;
                    case AssignedToPlayer:
                        UnassignController(controller);
                        break;
                    default:
                        requestCancellation = true;
                        break;
                    }
                }
            }
        }

        void UpdateAvailableControllers()
        {
            // Always check which controllers are available as this can change from frame to frame.
            numControllers = 0;
            controllers[numControllers].controller = CONTROLLER_KEYBOARD1;
            controllers[numControllers].description = "Left Keyboard";
            ++numControllers;

            controllers[numControllers].controller = CONTROLLER_KEYBOARD2;
            controllers[numControllers].description = "Right Keyboard";
            ++numControllers;

            for (const auto& gamepad : gamepadDescriptors)
            {
                if (IsGamepadAvailable(gamepad.gamepadNumber))
                {
                    controllers[numControllers].controller = gamepad.controllerId;
                    controllers[numControllers].description = gamepad.description;
                    ++numControllers;
                }
                else
                {
                    UnassignController(gamepad.controllerId);
                }
            }
        }

        ScreenState FixedUpdate()
        {
            UpdateAvailableControllers();

            // If the number of confirmed controllers matches the number of players then start the game.
            int numConfirmed = 0;
            int numUnassigned = 0;
            for (int i = 0; i < numPlayers; i++)
            {
                const auto& controllerAssignment = playerControllers[i];
                if (controllerAssignment.controller == CONTROLLER_UNASSIGNED)
                {
                    ++numUnassigned;
                }
                if (controllerAssignment.controller != CONTROLLER_UNASSIGNED && controllerAssignment.status == ConfirmedByPlayer)
                {
                    if (++numConfirmed == numPlayers)
                    {
                        return PLAYING;
                    }
                }
            }

            if (requestCancellation && numUnassigned == numPlayers)
            {
                requestCancellation = false;
                numPlayers = 0;
            }

            return MENU;
        }

        void DrawControllerSelection()
        {
            // Check player selections.
            CheckKeyboard(KEY_SPACE, KEY_W, CONTROLLER_KEYBOARD1);
            CheckKeyboard(KEY_ENTER, KEY_UP, CONTROLLER_KEYBOARD2);
            CheckGamepad(GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1);
            CheckGamepad(GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2);
            CheckGamepad(GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3);
            CheckGamepad(GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4);

            // Draw the controllers and whether they're unassigned, assigned, or confirmed.
            float width = screenWidth / numControllers;
            float controlWidth = width * 0.75f;
            float margin = (width - controlWidth) / 2;

            DrawTextEx(scoreFont, "Pick controls", {margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

            for (int i = 0; i < numControllers; i++)
            {
                char description[128];
                int bytes = TextCopy(description, TextFormat("%-15s", controllers[i].description));

                for (const auto& p : playerControllers)
                {
                    if (p.controller == controllers[i].controller)
                    {
                        bytes += TextCopy(description + bytes, "\n");
                        TextCopy(description + bytes, p.description);
                        GuiLabel({i * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, p.description);
                    }
                }
                GuiButton({i * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, description);
            }
        }

        void DrawPlayerSelection()
        {
            int playerCap = numControllers >= 4 ? 4 : numControllers;
            float width = screenWidth / (playerCap - 1);
            float controlWidth = width * 0.75f;
            float margin = (width - controlWidth) / 2;

            DrawTextEx(scoreFont, "How many players?", {margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

            if (GuiButton({0 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "2"))
            {
                numPlayers = 2;
            }
            if (playerCap > 2)
            {
                if (GuiButton({1 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "3"))
                {
                    numPlayers = 3;
                }
            }
            if (playerCap > 3)
            {
                if (GuiButton({2 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "4"))
                {
                    numPlayers = 4;
                }
            }
        }

        void Draw()
        {
            if (numPlayers == 0)
            {
                DrawPlayerSelection();
            }
            else
            {
                DrawControllerSelection();
            }
        }
    } // namespace menu

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
    my::scoreFont = LoadFont("assets/terminal/Mecha.ttf");

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
