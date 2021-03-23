/*
 * Spaced - a game about a perfect planet, as long as you obey the law.
 *
 * TODO:
 * + create a planet
 * + make it spin
 * - make it spin based on the physics rate, not the frame rate
 * - put people on it, and make sure they spin with it
 * - apply gravity, to keep the people on the planet
 * - remove gravity and see the people float into space
 * - turn it into a game (spot bad people and launch them into the path of a UFO?)
 * - spaced people eventually form a message
 * - draw some stars
 * - remove the rest of the crap
 */

#include "raylib.h"
#include "raymath.h"
#include "spaced.h"

#include <stdio.h>

#define TANK_SCALE 16.0f
#define TANK_OVERLAP (2 * TANK_SCALE)
#define MAX_ROTATION_SPEED 2.0f
#define TANK_ACCEL 0.05f
#define MAX_SPEED 2.0f
#define MAX_REVERSE_SPEED -1.0f
#define SHOT_SPEED 6.0f
#define SHOT_DURATION 90
#define TANK_COLLISION_RADIUS TANK_SCALE
#define SHOT_COLLISION_RADIUS (TANK_SCALE * 0.5f)

#define SHOTS_PER_PLAYER 5
#define MAX_SHOTS (SHOTS_PER_PLAYER * MAX_PLAYERS)

#define MAX_LINES 12

typedef Vector2 Position;
typedef Vector2 Velocity;
typedef float Heading;
typedef float Speed;

typedef enum
{
    PLAYING,
    PAUSED,
    CANCELLED
} PlayingState;

typedef struct
{
    bool alive;
    int player;
    Position pos;
    Velocity vel;
    Heading heading;
    Heading gunHeading;
    Speed speed;
    ControllerId controller;
    int index;
} Tank;

typedef struct
{
    int alive;
    Position pos;
    Velocity vel;
    Heading heading;
} Shot;

// Types of draw command.
typedef enum
{
    END,  // Indicates the last command.
    MOVE, // Move to a given position.
    LINE  // Draw a line from the current position to the given position. If there is no current position, start from the origin.
} CommandType;

// A draw command.
typedef struct
{
    CommandType type;
    Vector2 pos;
} Command;

static Tank tanks[MAX_PLAYERS];
static int numPlayers = 0;

static Shot shots[MAX_PLAYERS * SHOTS_PER_PLAYER];

static Color tankColours[MAX_PLAYERS];

// Shot appearance (+x is right, +y is down).
const Vector2 shotLines[] = {{0, -0.25f}, {0, 0.25f}};

// Tank appearances (+x is right, +y is down). Currently they're all identical.
const Command tankCommands[][MAX_LINES] = {
        // Tank 0.
        {{MOVE, {-0.67f, -1}},
         {LINE, {0.67f, -1}},
         {LINE, {1.0f, -0.67f}},
         {LINE, {1, 1}},
         {LINE, {-1, 1}},
         {LINE, {-1, -0.67f}},
         {LINE, {-0.67f, -1}},
         {END, {0, 0}}},
        // Tank 1.
        {{MOVE, {-0.67f, -1}},
         {LINE, {0.67f, -1}},
         {LINE, {1.0f, -0.67f}},
         {LINE, {1, 1}},
         {LINE, {-1, 1}},
         {LINE, {-1, -0.67f}},
         {LINE, {-0.67f, -1}},
         {END, {0, 0}}},
        // Tank 2.
        {{MOVE, {-0.67f, -1}},
         {LINE, {0.67f, -1}},
         {LINE, {1.0f, -0.67f}},
         {LINE, {1, 1}},
         {LINE, {-1, 1}},
         {LINE, {-1, -0.67f}},
         {LINE, {-0.67f, -1}},
         {END, {0, 0}}},
        // Tank 3.
        {{MOVE, {-0.67f, -1}},
         {LINE, {0.67f, -1}},
         {LINE, {1.0f, -0.67f}},
         {LINE, {1, 1}},
         {LINE, {-1, 1}},
         {LINE, {-1, -0.67f}},
         {LINE, {-0.67f, -1}},
         {END, {0, 0}}},
};

// Gun appearances. Currently they're all identical.
const Command gunCommands[][MAX_LINES] = {
        // Gun 0.
        {{MOVE, {-0.125f, -1}},
         {LINE, {0.125f, -1}},
         {LINE, {0.125f, 0.125f}},
         {LINE, {-0.125f, 0.125f}},
         {LINE, {-0.125f, -1}},
         {END, {0, 0}}},
        // Gun 1.
        {{MOVE, {-0.125f, -1}},
         {LINE, {0.125f, -1}},
         {LINE, {0.125f, 0.125f}},
         {LINE, {-0.125f, 0.125f}},
         {LINE, {-0.125f, -1}},
         {END, {0, 0}}},
        // Gun 2.
        {{MOVE, {-0.125f, -1}},
         {LINE, {0.125f, -1}},
         {LINE, {0.125f, 0.125f}},
         {LINE, {-0.125f, 0.125f}},
         {LINE, {-0.125f, -1}},
         {END, {0, 0}}},
        // Gun 3.
        {{MOVE, {-0.125f, -1}},
         {LINE, {0.125f, -1}},
         {LINE, {0.125f, 0.125f}},
         {LINE, {-0.125f, 0.125f}},
         {LINE, {-0.125f, -1}},
         {END, {0, 0}}},
};

static const char* pausedText = "Paused - Press [R] to resume";

static int screenWidth;
static int screenHeight;

static bool pauseOrQuitRequested;
static bool resumeRequested;
static PlayingState state;

static bool IsControllerThrustDown(int controller)
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

static bool IsControllerReverseDown(int controller)
{
    switch (controller)
    {
    case CONTROLLER_GAMEPAD1:
        return IsGamepadButtonDown(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    case CONTROLLER_GAMEPAD2:
        return IsGamepadButtonDown(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    case CONTROLLER_GAMEPAD3:
        return IsGamepadButtonDown(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    case CONTROLLER_GAMEPAD4:
        return IsGamepadButtonDown(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    case CONTROLLER_KEYBOARD1:
        return IsKeyDown(KEY_S);
    case CONTROLLER_KEYBOARD2:
        return IsKeyDown(KEY_DOWN);
    default:
        return false;
    }
}

static bool IsControllerFirePressed(int controller)
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

static float GetControllerTurnRate(int controller)
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

static float GetGunTurnRate(int controller)
{
    switch (controller)
    {
    case CONTROLLER_GAMEPAD1:
        return GetGamepadAxisMovement(GAMEPAD_PLAYER1, GAMEPAD_AXIS_RIGHT_X);
    case CONTROLLER_GAMEPAD2:
        return GetGamepadAxisMovement(GAMEPAD_PLAYER2, GAMEPAD_AXIS_RIGHT_X);
    case CONTROLLER_GAMEPAD3:
        return GetGamepadAxisMovement(GAMEPAD_PLAYER3, GAMEPAD_AXIS_RIGHT_X);
    case CONTROLLER_GAMEPAD4:
        return GetGamepadAxisMovement(GAMEPAD_PLAYER4, GAMEPAD_AXIS_RIGHT_X);
    case CONTROLLER_KEYBOARD1:
        return (IsKeyDown(KEY_E) ? 1.0f : 0.0f) - (IsKeyDown(KEY_Q) ? 1.0f : 0.0f);
    case CONTROLLER_KEYBOARD2:
        return (IsKeyDown(KEY_PERIOD) ? 1.0f : 0.0f) - (IsKeyDown(KEY_COMMA) ? 1.0f : 0.0f);
    default:
        return 0.0f;
    }
}

static Vector2 Move(Position pos, Velocity vel)
{
    pos = Vector2Add(pos, vel);

    // Wrap the position around the play area.
    if (pos.x >= (float)screenWidth)
    {
        pos.x -= (float)screenWidth;
    }
    if (pos.x < 0)
    {
        pos.x += (float)screenWidth;
    }
    if (pos.y >= (float)screenHeight)
    {
        pos.y -= (float)screenHeight;
    }
    if (pos.y < 0)
    {
        pos.y += (float)screenHeight;
    }

    return pos;
}

static void MoveTank(Tank* tank)
{
    tank->pos = Move(tank->pos, tank->vel);
}

static void MoveShot(Shot* shot)
{
    shot->pos = Move(shot->pos, shot->vel);
}

static void CollideTankShot(Tank* tank, Shot* shot)
{
    if (CheckCollisionCircles(tank->pos, TANK_COLLISION_RADIUS, shot->pos, SHOT_COLLISION_RADIUS))
    {
        tank->alive = false;
        shot->alive = 0;
    }
}

static void CollideTankTank(Tank* tank1, Tank* tank2)
{
    if (!tank1->alive || !tank2->alive)
    {
        return;
    }

    if (CheckCollisionCircles(tank1->pos, TANK_COLLISION_RADIUS, tank2->pos, TANK_COLLISION_RADIUS))
    {
        tank1->alive = false;
        tank2->alive = false;
    }
}

static void CollideTankShots(Tank* tank, int start, int end)
{
    if (!tank->alive)
    {
        return;
    }

    for (int i = start; i != end; i++)
    {
        if (shots[i].alive > 0)
        {
            CollideTankShot(tank, &shots[i]);
        }
    }
}

static void UpdateTank(Tank* tank)
{
    if (!tank->alive)
    {
        return;
    }

    // Rotate the tank.
    float axis = GetControllerTurnRate(tank->controller);
    tank->heading += axis * MAX_ROTATION_SPEED;

    // Accelerate the tank.
    if (IsControllerThrustDown(tank->controller))
    {
        tank->speed += TANK_ACCEL;
        tank->speed = fminf(MAX_SPEED, tank->speed);
    }
    else if (IsControllerReverseDown(tank->controller))
    {
        tank->speed -= TANK_ACCEL;
        tank->speed = fmaxf(MAX_REVERSE_SPEED, tank->speed);
    }
    else
    {
        tank->speed *= 0.9f;
    }

    // The tank's velocity is in its direction of travel.
    tank->vel.x = cosf((tank->heading - 90) * DEG2RAD) * tank->speed;
    tank->vel.y = sinf((tank->heading - 90) * DEG2RAD) * tank->speed;

    // Rotate the gun.
    float gunAxis = GetGunTurnRate(tank->controller);
    tank->gunHeading += gunAxis * MAX_ROTATION_SPEED;

    // Move the tank.
    MoveTank(tank);
}

static void CheckForFire(Tank* tank)
{
    if (!tank->alive)
    {
        return;
    }

    // Fire.
    if (IsControllerFirePressed(tank->controller))
    {
        const int baseStart = tank->index * SHOTS_PER_PLAYER;
        const int baseEnd = baseStart + SHOTS_PER_PLAYER;
        for (int i = baseStart; i < baseEnd; i++)
        {
            if (shots[i].alive == 0)
            {
                Shot* shot = &shots[i];
                shot->alive = SHOT_DURATION;
                shot->heading = tank->heading + tank->gunHeading;
                Vector2 angle = {cosf((shot->heading - 90) * DEG2RAD), sinf((shot->heading - 90) * DEG2RAD)};
                shot->pos = Vector2Add(tank->pos, Vector2Scale(angle, TANK_SCALE));
                shot->vel = Vector2Add(Vector2Scale(angle, SHOT_SPEED), tank->vel);
                break;
            }
        }
    }
}

static void UpdateShot(Shot* shot)
{
    if (shot->alive == 0)
    {
        return;
    }
    MoveShot(shot);
    --(shot->alive);
}

static void DrawCommands(const Command* commands, Vector2 pos, float heading, Color colour)
{
    Vector2 points[MAX_LINES];

    Vector2 here = Vector2Add(Vector2Scale(Vector2Rotate((Vector2){0, 0}, heading), TANK_SCALE), pos);
    int numPoints = 0;
    for (int i = 0; commands[i].type != END; i++)
    {
        const Vector2 coord = Vector2Add(Vector2Scale(Vector2Rotate(commands[i].pos, heading), TANK_SCALE), pos);
        if (commands[i].type == LINE)
        {
            if (numPoints == 0)
            {
                points[0] = here;
                ++numPoints;
            }
            points[numPoints] = coord;
            ++numPoints;
        }
        else if (commands[i].type == MOVE)
        {
            if (numPoints > 0)
            {
                DrawLineStrip(points, numPoints, colour);
                numPoints = 0;
            }
        }
        here = coord;
    }
    if (numPoints > 0)
    {
        DrawLineStrip(points, numPoints, colour);
    }
}

static void DrawTankAt(int tankType, Vector2 pos, float heading, float gunHeading, Color colour)
{
    DrawCommands(tankCommands[tankType], pos, heading, colour);
    DrawCommands(gunCommands[tankType], pos, heading + gunHeading, colour);
}

static void DrawTank(const Tank* tank, double alpha)
{
    // Interpolate the tank's drawing position with its velocity to reduce stutter.
    const Vector2 pos = Vector2Add(tank->pos, Vector2Scale(tank->vel, (float)alpha));

    const float heading = tank->heading;
    const float gunHeading = tank->gunHeading;

    // Which edges of the play area does the tank overlap?
    const bool overlapsTop = pos.y - TANK_OVERLAP < 0;                       // Going off the top of the screen.
    const bool overlapsBottom = pos.y + TANK_OVERLAP >= (float)screenHeight; // Going off the bottom of the screen.
    const bool overlapsLeft = pos.x - TANK_OVERLAP < 0;                      // Going off the left of the screen.
    const bool overlapsRight = pos.x + TANK_OVERLAP >= (float)screenWidth;   // Going off the right of the screen.

    const Color tankColour = tankColours[tank->index];
    const int tankType = tank->index;

    DrawTankAt(tankType, pos, heading, gunHeading, tankColour);

    if (overlapsTop)
    {
        DrawTankAt(tankType, Vector2Add(pos, (Vector2){0, (float)screenHeight}), heading, gunHeading, tankColour);
    }
    if (overlapsBottom)
    {
        DrawTankAt(tankType, Vector2Add(pos, (Vector2){0, (float)-screenHeight}), heading, gunHeading, tankColour);
    }
    if (overlapsLeft)
    {
        DrawTankAt(tankType, Vector2Add(pos, (Vector2){(float)screenWidth, 0}), heading, gunHeading, tankColour);
        if (overlapsTop)
        {
            DrawTankAt(tankType, Vector2Add(pos, (Vector2){(float)screenWidth, (float)screenHeight}), heading, gunHeading,
                       tankColour);
        }
        else if (overlapsBottom)
        {
            DrawTankAt(tankType, Vector2Add(pos, (Vector2){(float)screenWidth, (float)-screenHeight}), heading, gunHeading,
                       tankColour);
        }
    }
    if (overlapsRight)
    {
        DrawTankAt(tankType, Vector2Add(pos, (Vector2){(float)-screenWidth, 0}), heading, gunHeading, tankColour);
        if (overlapsTop)
        {
            DrawTankAt(tankType, Vector2Add(pos, (Vector2){(float)-screenWidth, (float)screenHeight}), heading, gunHeading,
                       tankColour);
        }
        else if (overlapsBottom)
        {
            DrawTankAt(tankType, Vector2Add(pos, (Vector2){(float)-screenWidth, (float)-screenHeight}), heading, gunHeading,
                       tankColour);
        }
    }
}

static void DrawShotAt(Vector2 pos, float heading, Color colour)
{
    Vector2 points[2];
    for (int i = 0; i < 2; i++)
    {
        points[i] = Vector2Add(Vector2Scale(Vector2Rotate(shotLines[i], heading), TANK_SCALE), pos);
    }
    DrawLineStrip(points, 2, colour);
}

static void DrawShot(const Shot* shot, Color colour, double alpha)
{
    // Interpolate the shot's drawing position with its velocity to reduce stutter.
    const Vector2 pos = Vector2Add(shot->pos, Vector2Scale(shot->vel, (float)alpha));

    DrawShotAt(pos, shot->heading, colour);
}

static void CheckKeyboard(KeyboardKey selectKey, KeyboardKey cancelKey)
{
    pauseOrQuitRequested = pauseOrQuitRequested || IsKeyReleased(selectKey);
    resumeRequested = resumeRequested || IsKeyReleased(cancelKey);
}

static void CheckGamepad(GamepadNumber gamepad, GamepadButton selectButton, GamepadButton cancelButton)
{
    if (!IsGamepadAvailable(gamepad))
    {
        return;
    }

    pauseOrQuitRequested = pauseOrQuitRequested || IsGamepadButtonReleased(gamepad, selectButton);
    resumeRequested = resumeRequested || IsGamepadButtonReleased(gamepad, cancelButton);
}

void InitPlayingScreen(int players, const ControllerId* controllers)
{
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    state = PLAYING;
    pauseOrQuitRequested = false;
    resumeRequested = false;

    tankColours[0] = GREEN;
    tankColours[1] = YELLOW;
    tankColours[2] = PINK;
    tankColours[3] = SKYBLUE;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        tanks[i].alive = false;
        tanks[i].index = i;
    }

    numPlayers = players;
    for (int i = 0; i < players; i++)
    {
        float angle = (float)i * (2 * 3.141592654f) / (float)numPlayers;
        tanks[i].controller = controllers[i];
        tanks[i].alive = true;
        tanks[i].pos.x = (float)screenWidth / 2.0f + cosf(angle) * (float)screenHeight / 3;
        tanks[i].pos.y = (float)screenHeight / 2.0f + sinf(angle) * (float)screenHeight / 3;
        tanks[i].heading =
                180.0f + RAD2DEG * atan2f((float)screenHeight / 2.0f - tanks[i].pos.y, (float)screenWidth / 2.0f - tanks[i].pos.x);
        tanks[i].gunHeading = 0.0f;
        tanks[i].vel = (Vector2){0, 0};
    }

    for (int i = 0; i < MAX_SHOTS; i++)
    {
        shots[i].alive = false;
    }
}

void FinishPlayingScreen(void)
{
}

void UpdatePlayingScreen(void)
{
    // Check for internal state changes.
    if (state == PLAYING)
    {
        if (pauseOrQuitRequested)
        {
            pauseOrQuitRequested = false;
            state = PAUSED;
        }
    }
    else if (state == PAUSED)
    {
        if (pauseOrQuitRequested)
        {
            pauseOrQuitRequested = false;
            state = CANCELLED;
        }
        else if (resumeRequested)
        {
            resumeRequested = false;
            state = PLAYING;
        }
    }

    // Only update the game state when playing.
    if (state == PLAYING)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            UpdateTank(&tanks[i]);
        }

        for (int i = 0; i < MAX_SHOTS; i++)
        {
            UpdateShot(&shots[i]);
        }

        // Collide each player with the other players' shots.
        for (int i = 0; i < numPlayers; i++)
        {
            for (int j = 0; j < numPlayers; j++)
            {
                if (j != i)
                {
                    CollideTankShots(&tanks[i], j * SHOTS_PER_PLAYER, j * SHOTS_PER_PLAYER + SHOTS_PER_PLAYER);
                }
            }
        }

        // Collide each player with the other players.
        for (int i = 0; i < numPlayers - 1; i++)
        {
            for (int j = i + 1; j < numPlayers; j++)
            {
                CollideTankTank(&tanks[i], &tanks[j]);
            }
        }
    }
}

static float baseAngle = 0.0f; // TODO: move this!

void DrawPlayingScreen(double alpha)
{
    BeginDrawing();

    ClearBackground(BLACK);
    DrawText("PLAYING", 4, 4, 20, RAYWHITE);
    if (state == PAUSED)
    {
        int width = MeasureText(pausedText, 20);
        DrawText(pausedText, (screenWidth - width) / 2, 7 * screenHeight / 8, 20, RAYWHITE);
    }

    if (state == PAUSED)
    {
        alpha = 0.0;
    }

    // Draw the tanks.
    for (int i = 0; i < numPlayers; i++)
    {
        if (tanks[i].alive)
        {
            DrawTank(&tanks[i], alpha);
        }
    }

    // Draw the shots.
    for (int i = 0; i < numPlayers * SHOTS_PER_PLAYER; i++)
    {
        const Shot* shot = &shots[i];
        if (shot->alive > 0)
        {
            Color colour = tankColours[i / SHOTS_PER_PLAYER];
            DrawShot(shot, colour, alpha);
        }
    }

    Vector2 planetCentre = (Vector2){(float)screenWidth / 2, 2 * (float)screenHeight / 3 - 4.0f};

    // Draw the planet.
    DrawCircleSector(planetCentre, (float)screenHeight / 3.0f, 0.0f, 360.0f, 72, GREEN);

    // Draw the rotating part of the planet.
    baseAngle = fmodf(baseAngle + 0.1f, 30.0f);
    for (float startAngle = 0.0f; startAngle < 360.0f; startAngle += 30.0f)
    {
        DrawRing(planetCentre, (float)screenHeight / 3.0f - 12.0f, (float)screenHeight / 3.0f, baseAngle + startAngle,
                 baseAngle + startAngle + 15.0f, 15, BLUE);
    }

    DrawFPS(screenWidth / 2 - 16, screenHeight - 24);

    EndDrawing();
}

void CheckTriggersPlayingScreen(void)
{
    // Check for player(s) choosing to pause / resume / quit.
    CheckKeyboard(KEY_ESCAPE, KEY_R);
    CheckGamepad(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);
    CheckGamepad(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);
    CheckGamepad(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);
    CheckGamepad(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);

    if (state == PLAYING)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            CheckForFire(&tanks[i]);
        }
    }
}

bool IsCancelledPlayingScreen(void)
{
    return state == CANCELLED;
}
