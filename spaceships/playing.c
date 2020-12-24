#include "raylib.h"
#include "raymath.h"
#include "spaceships.h"

#define SHIP_SCALE 16.0f
#define SHIP_OVERLAP (2 * SHIP_SCALE)
#define MAX_ROTATION_SPEED 4.0f
#define SPEED 0.1f
#define SHOT_SPEED 6.0f
#define SHOT_DURATION 90
#define SHIP_COLLISION_RADIUS SHIP_SCALE
#define SHOT_COLLISION_RADIUS (SHIP_SCALE * 0.5f)

#define SHOTS_PER_PLAYER 5
#define MAX_SHOTS (SHOTS_PER_PLAYER * MAX_PLAYERS)

#define MAX_LINES 12

typedef Vector2 Position;
typedef Vector2 Velocity;
typedef float Heading;

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
    ControllerId controller;
    int index;
} Ship;

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

static Ship ships[MAX_PLAYERS];
static int numPlayers = 0;

static Shot shots[MAX_PLAYERS * SHOTS_PER_PLAYER];

static Color shipColours[MAX_PLAYERS];

// Shot appearance.
const Vector2 shotLines[] = {{0, -0.25f}, {0, 0.25f}};

// Ship appearance.
const Command shipCommands[][MAX_LINES] = {
        // Ship 0.
        {{MOVE, {-1, 1}}, {LINE, {0, -1}}, {LINE, {1, 1}}, {LINE, {0, 0.5f}}, {LINE, {-1, 1}}, {END}},
        // Ship 1.
        {{MOVE, {0, -1}}, {LINE, {1, 0.5f}}, {LINE, {0, 1}}, {LINE, {-1, 0.5f}}, {LINE, {0, -1}}, {END}},
        // Ship 2.
        {{MOVE, {0, -1}},
         {LINE, {0.5f, 0}},
         {LINE, {1, 0.3f}},
         {LINE, {0.25f, 1}},
         {LINE, {-0.25f, 1}},
         {LINE, {-1, 0.3f}},
         {LINE, {-0.5f, 0}},
         {LINE, {0, -1}},
         {END}},
        // Ship 3.
        {{MOVE, {0, -1}},
         {LINE, {0.25f, 0.25f}},
         {LINE, {1, 0}},
         {LINE, {1, 1}},
         {LINE, {0.5f, 0.75f}},
         {LINE, {-0.5f, 0.75f}},
         {LINE, {-1, 1}},
         {LINE, {-1, 0}},
         {LINE, {-0.25f, 0.25f}},
         {LINE, {0, -1}},
         {END}}};

static const char* pausedText = "Paused";

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

static void MoveShip(Ship* ship)
{
    ship->pos = Move(ship->pos, ship->vel);
}

static void MoveShot(Shot* shot)
{
    shot->pos = Move(shot->pos, shot->vel);
}

static void CollideShipShot(Ship* ship, Shot* shot)
{
    if (CheckCollisionCircles(ship->pos, SHIP_COLLISION_RADIUS, shot->pos, SHOT_COLLISION_RADIUS))
    {
        ship->alive = false;
        shot->alive = 0;
    }
}

static void CollideShipShip(Ship* ship1, Ship* ship2)
{
    if (!ship1->alive || !ship2->alive)
    {
        return;
    }

    if (CheckCollisionCircles(ship1->pos, SHIP_COLLISION_RADIUS, ship2->pos, SHIP_COLLISION_RADIUS))
    {
        ship1->alive = false;
        ship2->alive = false;
    }
}

static void CollideShipShots(Ship* ship, int start, int end)
{
    if (!ship->alive)
    {
        return;
    }

    for (int i = start; i != end; i++)
    {
        if (shots[i].alive > 0)
        {
            CollideShipShot(ship, &shots[i]);
        }
    }
}

static void UpdateShip(Ship* ship)
{
    if (!ship->alive)
    {
        return;
    }

    // Rotate the ship.
    float axis = GetControllerTurnRate(ship->controller);
    ship->heading += axis * MAX_ROTATION_SPEED;

    // Accelerate the ship.
    if (IsControllerThrustDown(ship->controller))
    {
        ship->vel.x += cosf((ship->heading - 90) * DEG2RAD) * SPEED;
        ship->vel.y += sinf((ship->heading - 90) * DEG2RAD) * SPEED;
    }

    // Move the ship.
    MoveShip(ship);
}

static void CheckForFire(Ship* ship)
{
    if (!ship->alive)
    {
        return;
    }

    // Fire.
    if (IsControllerFirePressed(ship->controller))
    {
        const int baseStart = ship->index * SHOTS_PER_PLAYER;
        const int baseEnd = baseStart + SHOTS_PER_PLAYER;
        for (int i = baseStart; i < baseEnd; i++)
        {
            if (shots[i].alive == 0)
            {
                Shot* shot = &shots[i];
                shot->alive = SHOT_DURATION;
                shot->heading = ship->heading;
                Vector2 angle = {cosf((ship->heading - 90) * DEG2RAD), sinf((ship->heading - 90) * DEG2RAD)};
                shot->pos = Vector2Add(ship->pos, Vector2Scale(angle, SHIP_SCALE));
                shot->vel = Vector2Add(Vector2Scale(angle, SHOT_SPEED), ship->vel);
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

static void DrawShipAt(int shipType, Vector2 pos, float heading, Color colour)
{
    Vector2 points[MAX_LINES];

    // Default to starting at the origin.
    Vector2 here = Vector2Add(Vector2Scale(Vector2Rotate((Vector2){0, 0}, heading), SHIP_SCALE), pos);

    int numPoints = 0;
    const Command* commands = shipCommands[shipType];
    for (int i = 0; commands[i].type != END; i++)
    {
        const Vector2 coord = Vector2Add(Vector2Scale(Vector2Rotate(commands[i].pos, heading), SHIP_SCALE), pos);
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

static void DrawShip(const Ship* ship, double alpha)
{
    // Interpolate the ship's drawing position with its velocity to reduce stutter.
    const Vector2 pos = Vector2Add(ship->pos, Vector2Scale(ship->vel, (float)alpha));

    const float heading = ship->heading;

    // Which edges of the play area does the ship overlap?
    const bool overlapsTop = pos.y - SHIP_OVERLAP < 0;
    const bool overlapsBottom = pos.y + SHIP_OVERLAP >= (float)screenHeight;
    const bool overlapsLeft = pos.x - SHIP_OVERLAP < 0;
    const bool overlapsRight = pos.x + SHIP_OVERLAP >= (float)screenWidth;

    const Color shipColour = shipColours[ship->index];
    const int shipType = ship->index;

    DrawShipAt(shipType, pos, heading, shipColour);

    if (overlapsTop)
    {
        DrawShipAt(shipType, Vector2Add(pos, (Vector2){0, (float)screenHeight}), heading, shipColour);
    }
    if (overlapsBottom)
    {
        DrawShipAt(shipType, Vector2Add(pos, (Vector2){0, (float)-screenHeight}), heading, shipColour);
    }
    if (overlapsLeft)
    {
        DrawShipAt(shipType, Vector2Add(pos, (Vector2){(float)screenWidth, 0}), heading, shipColour);
    }
    if (overlapsRight)
    {
        DrawShipAt(shipType, Vector2Add(pos, (Vector2){(float)-screenWidth, 0}), heading, shipColour);
    }
}

static void DrawShotAt(Vector2 pos, float heading, Color colour)
{
    Vector2 points[2];
    for (int i = 0; i < 2; i++)
    {
        points[i] = Vector2Add(Vector2Scale(Vector2Rotate(shotLines[i], heading), SHIP_SCALE), pos);
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

    shipColours[0] = GREEN;
    shipColours[1] = YELLOW;
    shipColours[2] = PINK;
    shipColours[3] = SKYBLUE;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        ships[i].alive = false;
        ships[i].index = i;
    }

    numPlayers = players;
    for (int i = 0; i < players; i++)
    {
        float angle = (float)i * (2 * 3.141592654f) / (float)numPlayers;
        ships[i].controller = controllers[i];
        ships[i].alive = true;
        ships[i].pos.x = (float)screenWidth / 2.0f + cosf(angle) * (float)screenHeight / 3;
        ships[i].pos.y = (float)screenHeight / 2.0f + sinf(angle) * (float)screenHeight / 3;
        ships[i].heading =
                RAD2DEG * atan2f((float)screenHeight / 2.0f - ships[i].pos.y, (float)screenWidth / 2.0f - ships[i].pos.x);
        ships[i].vel = (Vector2){0, 0};
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
            UpdateShip(&ships[i]);
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
                    CollideShipShots(&ships[i], j * SHOTS_PER_PLAYER, j * SHOTS_PER_PLAYER + SHOTS_PER_PLAYER);
                }
            }
        }

        // Collide each player with the other players.
        for (int i = 0; i < numPlayers - 1; i++)
        {
            for (int j = i + 1; j < numPlayers; j++)
            {
                CollideShipShip(&ships[i], &ships[j]);
            }
        }
    }
}

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

    // Draw the ships.
    for (int i = 0; i < numPlayers; i++)
    {
        if (ships[i].alive)
        {
            DrawShip(&ships[i], alpha);
        }
    }

    // Draw the shots.
    for (int i = 0; i < numPlayers * SHOTS_PER_PLAYER; i++)
    {
        const Shot* shot = &shots[i];
        if (shot->alive > 0)
        {
            Color colour = shipColours[i / SHOTS_PER_PLAYER];
            DrawShot(shot, colour, alpha);
        }
    }

    DrawFPS(screenWidth / 2 - 16, screenHeight - 24);

    EndDrawing();
}

void CheckTriggersPlayingScreen(void)
{
    // Check for player(s) choosing to pause / resume / quit.
    CheckKeyboard(KEY_P, KEY_R);
    CheckGamepad(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);
    CheckGamepad(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);
    CheckGamepad(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);
    CheckGamepad(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT);

    if (state == PLAYING)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            CheckForFire(&ships[i]);
        }
    }
}

bool IsCancelledPlayingScreen(void)
{
    return state == CANCELLED;
}
