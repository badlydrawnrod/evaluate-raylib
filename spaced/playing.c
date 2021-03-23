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
 * - spaced people eventually form a message?
 * - draw some stars
 * - remove the rest of the crap
 * - have people emerge from a door on the top of the world and run around
 *      - if they reach the door again then they're safe
 *      - if the player clicks on them before they reach the door then they're spaced
 * - have the game inform the player who they need to space
 *      - space "Communists"
 *      - space anti-semites
 *      - space polluters
 * - at the end, have the game tell the player what their political affiliation is based on who they spaced
 */

#include "raylib.h"
#include "raymath.h"
#include "spaced.h"

#include <stdio.h>

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

static const char* pausedText = "Paused - Press [R] to resume";

static int screenWidth;
static int screenHeight;

static bool pauseOrQuitRequested;
static bool resumeRequested;
static PlayingState state;

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

void InitPlayingScreen(void)
{
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    state = PLAYING;
    pauseOrQuitRequested = false;
    resumeRequested = false;
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
    }
}

static void DrawPlanet()
{
    Vector2 planetCentre = (Vector2){(float)screenWidth / 2, 2 * (float)screenHeight / 3 - 32.0f};
    Vector2 houseCentre = Vector2Add(planetCentre, (Vector2){0.0f, -screenHeight / 3.0f});

    // Draw the house.
    DrawRectangle((int)(houseCentre.x - 16.0f), (int)(houseCentre.y - 16.0f), 32, 32, YELLOW);
    DrawRectangle((int)(houseCentre.x - 16.0f), (int)(houseCentre.y - 24.0f), 32, 8, ORANGE);

    // Draw the planet.
    DrawCircleSector(planetCentre, (float)screenHeight / 3.0f, 0.0f, 360.0f, 72, DARKGREEN);
}

void DrawPlayingScreen(double alpha)
{
    BeginDrawing();

    ClearBackground(BLACK);
    DrawText("PLAYING", 4, 4, 20, RAYWHITE);

    if (state == PAUSED)
    {
        alpha = 0.0;
    }

    DrawPlanet();

    if (state == PAUSED)
    {
        int width = MeasureText(pausedText, 20);
        DrawText(pausedText, (screenWidth - width) / 2, screenHeight / 2, 20, RAYWHITE);
    }

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
    }
}

bool IsCancelledPlayingScreen(void)
{
    return state == CANCELLED;
}
