#include "raylib.h"
#include "raymath.h"

namespace my
{
    constexpr int targetFps = 60;
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    constexpr Vector2 shipLines[] = {{-1, 1}, {0, -1}, {1, 1}, {0, 0.5f}, {-1, 1}};

    Vector2 shipPos[2] = {{screenWidth / 4, screenHeight / 2}, {3 * screenWidth / 4, screenHeight / 2}};
    Vector2 shipVel[2] = {{0, 0}, {0, 0}};
    float shipHeading[2] = {45.0f, -45.0f};

    GamepadNumber pads[2] = {GAMEPAD_PLAYER1, GAMEPAD_PLAYER2};

    constexpr float shipScale = 16.0f;
    constexpr float shipOverlap = 2 * shipScale;
    constexpr float maxRotationSpeed = 4.0f;
    constexpr float speed = 0.1f;

    int numShips = 2;

    void MoveShip(int id)
    {
        // Which gamepad are we using?
        const GamepadNumber pad = pads[id];

        // Rotate the ship.
        float& heading = shipHeading[id];
        const float axis = GetGamepadAxisMovement(pad, 0);
        heading += axis * maxRotationSpeed;

        // Accelerate the ship.
        Vector2& vel = shipVel[id];
        if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
        {
            vel.x += cosf((heading - 90) * DEG2RAD) * speed;
            vel.y += sinf((heading - 90) * DEG2RAD) * speed;
        }

        // Move the ship.
        Vector2& pos = shipPos[id];
        pos = Vector2Add(pos, vel);

        // Wrap the ship around the play area.
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

    void Update()
    {
        for (int i = 0; i < numShips; i++)
        {
            MoveShip(i);
        }
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

    void DrawShip(int id)
    {
        const Vector2 pos = shipPos[id];
        const float heading = shipHeading[id];

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

    void Draw()
    {
        ClearBackground(BLACK);
        BeginDrawing();
        for (int i = 0; i < numShips; i++)
        {
            DrawShip(i);
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
