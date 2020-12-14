#include "playing.h"

#include "config.h"
#include "controllers.h"
#include "raylib.h"
#include "raymath.h"

#include <array>

namespace playing
{
    using Position = Vector2;
    using Velocity = Vector2;
    using Heading = float;

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

    float screenWidth;
    float screenHeight;

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
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

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
