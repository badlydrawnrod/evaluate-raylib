#include "raylib.h"
#include "spaceships.h"

typedef enum
{
    SHOWING_MENU,
    STARTING
} MenuState;

static int screenWidth;
static int screenHeight;

static bool startRequested;
static MenuState state;

static void CheckKeyboard(KeyboardKey key)
{
    startRequested = startRequested || IsKeyReleased(key);
}

static void CheckGamepad(GamepadNumber gamepad, GamepadButton button)
{
    startRequested = startRequested || (IsGamepadAvailable(gamepad) && IsGamepadButtonReleased(gamepad, button));
}

void InitMenu(void)
{
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    state = SHOWING_MENU;
    startRequested = false;
}

void FinishMenu(void)
{
}

bool IsStartingMenu(void)
{
    return state == STARTING;
}

void DrawMenu(double alpha)
{
    (void)alpha;
    ClearBackground(BLACK);
    BeginDrawing();
    DrawText("MENU", 4, 4, 20, RAYWHITE);
    int width = MeasureText("Press [FIRE] to start", 20);
    DrawText("Press [FIRE] to start", (screenWidth - width) / 2, 7 * screenHeight / 8, 20, RAYWHITE);
    EndDrawing();
}

void UpdateMenu(void)
{
    if (startRequested)
    {
        state = STARTING;
    }
}

void CheckTriggersMenu(void)
{
    CheckKeyboard(KEY_SPACE);
    CheckKeyboard(KEY_ENTER);
    CheckGamepad(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    CheckGamepad(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    CheckGamepad(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    CheckGamepad(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
}
