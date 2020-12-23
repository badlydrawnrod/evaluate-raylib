#include "raylib.h"
#include "spaceships.h"

typedef enum
{
    SHOWING_MENU,
    STARTING,
    CANCELLED
} MenuState;

static int screenWidth;
static int screenHeight;

static bool startRequested;
static bool quitRequested;
static MenuState state;

static void CheckKeyboard(KeyboardKey selectKey, KeyboardKey cancelKey)
{
    startRequested = startRequested || IsKeyReleased(selectKey);
    quitRequested = quitRequested || IsKeyReleased(cancelKey);
}

static void CheckGamepad(GamepadNumber gamepad, GamepadButton selectButton, GamepadButton cancelButton)
{
    if (!IsGamepadAvailable(gamepad))
    {
        return;
    }

    startRequested = startRequested || IsGamepadButtonReleased(gamepad, selectButton);
    quitRequested = quitRequested || IsGamepadButtonReleased(gamepad, cancelButton);
}

void InitMenuScreen(void)
{
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    state = SHOWING_MENU;
    startRequested = false;
    quitRequested = false;
}

void FinishMenuScreen(void)
{
}

void UpdateMenuScreen(void)
{
    if (startRequested)
    {
        startRequested = false;
        state = STARTING;
    }

    if (quitRequested)
    {
        quitRequested = false;
        state = CANCELLED;
    }
}

void DrawMenuScreen(double alpha)
{
    (void)alpha;
    ClearBackground(BLACK);
    BeginDrawing();
    DrawText("MENU", 4, 4, 20, RAYWHITE);
    int width = MeasureText("Press [FIRE] to start", 20);
    DrawText("Press [FIRE] to start", (screenWidth - width) / 2, 7 * screenHeight / 8, 20, RAYWHITE);
    EndDrawing();
}

void CheckTriggersMenuScreen(void)
{
    CheckKeyboard(KEY_SPACE, KEY_ESCAPE);
    CheckKeyboard(KEY_ENTER, KEY_ESCAPE);
    CheckGamepad(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    CheckGamepad(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    CheckGamepad(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    CheckGamepad(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_MIDDLE_RIGHT);
}

bool IsStartedMenuScreen(void)
{
    return state == STARTING;
}

bool IsCancelledMenuScreen(void)
{
    return state == CANCELLED;
}
