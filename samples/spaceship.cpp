#include "raylib.h"
#include "raymath.h"

#include <array>

namespace my
{
    constexpr int targetFps = 60;
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

    using Position = Vector2;
    using Velocity = Vector2;
    using Heading = float;

    struct Ship
    {
        bool alive;
        int player;
        Position pos;
        Velocity vel;
        Heading heading;
        GamepadNumber pad;
    };

    struct Shot
    {
        int alive;
        Position pos;
        Velocity vel;
        Heading heading;
    };

    std::array<Ship, 2> ships{Ship{true, 1, {screenWidth / 4, screenHeight / 2}, {0, 0}, -45, GAMEPAD_PLAYER1},
                              Ship{true, 2, {3 * screenWidth / 4, screenHeight / 2}, {0, 0}, 45, GAMEPAD_PLAYER2}};

    using Shots = std::array<Shot, 10>;
    Shots shots;
    constexpr int numShots = shots.size();

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
        const float axis = GetGamepadAxisMovement(ship.pad, 0);
        ship.heading += axis * maxRotationSpeed;

        // Accelerate the ship.
        if (IsGamepadButtonDown(ship.pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
        {
            ship.vel.x += cosf((ship.heading - 90) * DEG2RAD) * speed;
            ship.vel.y += sinf((ship.heading - 90) * DEG2RAD) * speed;
        }

        // Fire.
        if (IsGamepadButtonPressed(ship.pad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))
        {
            const int baseStart = (ship.player == 1) ? 0 : numShots / 2;
            const int baseEnd = baseStart + numShots / 2;
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

        // Move the ship.
        Move(ship);
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

    void Update()
    {
        for (auto& ship : ships)
        {
            Update(ship);
        }
        for (auto& shot : shots)
        {
            Update(shot);
        }

        Collide(ships[0], shots.begin() + shots.size() / 2, shots.end());
        Collide(ships[1], shots.begin(), shots.begin() + shots.size() / 2);
    }

    void DrawShipAt(Vector2 pos, float heading)
    {
        Vector2 points[5];
        for (int i = 0; i < 5; i++)
        {
            points[i] = Vector2Add(Vector2Scale(Vector2Rotate(shipLines[i], heading), shipScale), pos);
        }
        DrawLineStrip(points, 5, RAYWHITE);
    }

    void DrawShotAt(Vector2 pos, float heading)
    {
        Vector2 points[2];
        for (int i = 0; i < 2; i++)
        {
            points[i] = Vector2Add(Vector2Scale(Vector2Rotate(shotLines[i], heading), shipScale), pos);
        }
        DrawLineStrip(points, 2, RAYWHITE);
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

        DrawShipAt(pos, heading);

        if (overlapsTop)
        {
            DrawShipAt(Vector2Add(pos, {0, screenHeight}), heading);
        }
        if (overlapsBottom)
        {
            DrawShipAt(Vector2Add(pos, {0, screenHeight}), heading);
        }
        if (overlapsLeft)
        {
            DrawShipAt(Vector2Add(pos, {screenWidth, 0}), heading);
        }
        if (overlapsRight)
        {
            DrawShipAt(Vector2Add(pos, {-screenWidth, 0}), heading);
        }
    }

    void Draw(const Shot& shot)
    {
        DrawShotAt(shot.pos, shot.heading);
    }

    void Draw()
    {
        ClearBackground(BLACK);
        BeginDrawing();
        for (const auto& ship : ships)
        {
            if (ship.alive)
            {
                Draw(ship);
            }
        }
        for (const auto& shot : shots)
        {
            if (shot.alive > 0)
            {
                Draw(shot);
            }
        }
        DrawFPS(4, 4);
        EndDrawing();
    }

} // namespace my

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(my::screenWidth, my::screenHeight, "Spaceships");
    SetTargetFPS(my::targetFps);

    while (!WindowShouldClose())
    {
        my::Update();
        my::Draw();
    }
    CloseWindow();

    return 0;
}
