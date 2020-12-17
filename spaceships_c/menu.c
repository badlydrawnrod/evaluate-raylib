#include "menu.h"

#include "constants.h"
#include "raygui.h"
#include "raylib.h"

#define MAX_PLAYERS 4
#define MAX_CONTROLLERS 6

typedef enum ControllerId
{
    CONTROLLER_UNASSIGNED = -1,
    CONTROLLER_KEYBOARD1,
    CONTROLLER_KEYBOARD2,
    CONTROLLER_GAMEPAD1,
    CONTROLLER_GAMEPAD2,
    CONTROLLER_GAMEPAD3,
    CONTROLLER_GAMEPAD4
} ControllerId;

typedef enum AssignmentStatus
{
    Unassigned,
    AssignedToPlayer,
    ConfirmedByPlayer
} AssignmentStatus;

static float screenWidth;
static float screenHeight;

static int numPlayers = 0;

// Player controllers and how they're assigned.
typedef struct PlayerController
{
    ControllerId controller; // Which controller is assigned to this player, if any?
    AssignmentStatus status; // What's the status of this controller?
    char description[16];    // Description of the controller's status and assignment.
} PlayerController;

PlayerController playerControllers[MAX_PLAYERS];

static int numControllers = 0;
static Font scoreFont;

static struct ControllerInfo
{
    ControllerId controller; // The controller id, e.g., CONTROLLER_GAMEPAD1.
    const char* description; // The controller's description, e.g., "Gamepad 1".
} controllers[MAX_CONTROLLERS];

static struct
{
    GamepadNumber gamepadNumber; // Which raylib gamepad is this?
    ControllerId controllerId;   // Which controller is this?
    const char* description;     // What do we display to the player?
} gamepadDescriptors[4] = {{GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1, "Gamepad 1"},
                           {GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2, "Gamepad 2"},
                           {GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3, "Gamepad 3"},
                           {GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4, "Gamepad 4"}};

static bool cancellationRequested = false;

// Determine if a controller is unassigned, assigned, or confirmed.
static AssignmentStatus GetControllerStatus(ControllerId controller)
{
    for (int i = 0; i < numPlayers; i++)
    {
        if (playerControllers[i].controller == controller)
        {
            return playerControllers[i].status;
        }
    }
    return Unassigned;
}

// Assign a controller.
static void AssignController(ControllerId controller)
{
    for (int i = 0; i < numPlayers; i++)
    {
        if (playerControllers[i].status == Unassigned)
        {
            playerControllers[i].controller = controller;
            playerControllers[i].status = AssignedToPlayer;
            TextCopy(playerControllers[i].description, TextFormat("Assigned [P%d]", i + 1));
            return;
        }
    }
}

// Unassign a controller.
static void UnassignController(ControllerId controller)
{
    for (int i = 0; i < numPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status != Unassigned)
        {
            playerControllers[i].controller = CONTROLLER_UNASSIGNED;
            playerControllers[i].status = Unassigned;
            TextCopy(playerControllers[i].description, "");
            return;
        }
    }
}

// Confirm an assigned controller.
static void ConfirmController(ControllerId controller)
{
    for (int i = 0; i < numPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status == AssignedToPlayer)
        {
            playerControllers[i].status = ConfirmedByPlayer;
            TextCopy(playerControllers[i].description, TextFormat("Confirmed [P%d]", i + 1));
        }
    }
}

// Unconfirm a confirmed controller and put it back to assigned.
static void UnconfirmController(ControllerId controller)
{
    for (int i = 0; i < numPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status == ConfirmedByPlayer)
        {
            playerControllers[i].status = AssignedToPlayer;
            TextCopy(playerControllers[i].description, TextFormat("Assigned [P%d]", i + 1));
        }
    }
}

// Check the keyboard for selection / cancellation.
static void CheckKeyboard(KeyboardKey selectKey, KeyboardKey cancelKey, ControllerId controller)
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
            cancellationRequested = true;
            break;
        }
    }
}

// Check a gamepad for selection / cancellation.
static void CheckGamepad(GamepadNumber gamepad, ControllerId controller)
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
                cancellationRequested = true;
                break;
            }
        }
    }
}

// Check which controllers are available as this can change from frame to frame.
static void UpdateAvailableControllers(void)
{
    numControllers = 0;
    controllers[numControllers].controller = CONTROLLER_KEYBOARD1;
    controllers[numControllers].description = "Left Keyboard";
    ++numControllers;

    controllers[numControllers].controller = CONTROLLER_KEYBOARD2;
    controllers[numControllers].description = "Right Keyboard";
    ++numControllers;

    for (int i = 0; i < sizeof(gamepadDescriptors) / sizeof(*gamepadDescriptors); i++)
    {
        if (IsGamepadAvailable(gamepadDescriptors[i].gamepadNumber))
        {
            controllers[numControllers].controller = gamepadDescriptors[i].controllerId;
            controllers[numControllers].description = gamepadDescriptors[i].description;
            ++numControllers;
        }
        else
        {
            UnassignController(gamepadDescriptors[i].controllerId);
        }
    }
}

// Draw the controller selection screen.
static void DrawControllerSelection(void)
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

    DrawTextEx(scoreFont, "Pick controls", (Vector2){margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

    for (int i = 0; i < numControllers; i++)
    {
        char description[128];
        int bytes = TextCopy(description, TextFormat("%-15s", controllers[i].description));

        for (int j = 0; j < sizeof(playerControllers) / sizeof(*playerControllers); j++)
        {
            if (playerControllers[j].controller == controllers[i].controller)
            {
                bytes += TextCopy(description + bytes, "\n");
                TextCopy(description + bytes, playerControllers[j].description);
                GuiLabel((Rectangle){i * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f},
                         playerControllers[j].description);
            }
        }
        GuiButton((Rectangle){i * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, description);
    }
}

// Draw the screen that allows the player to determine the number of players.
static void DrawPlayerSelection(void)
{
    int playerCap = numControllers >= 4 ? 4 : numControllers;
    float width = screenWidth / (playerCap - 1);
    float controlWidth = width * 0.75f;
    float margin = (width - controlWidth) / 2;

    DrawTextEx(scoreFont, "How many players?", (Vector2){margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

    if (GuiButton((Rectangle){0 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "2"))
    {
        numPlayers = 2;
    }
    if (playerCap > 2)
    {
        if (GuiButton((Rectangle){1 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "3"))
        {
            numPlayers = 3;
        }
    }
    if (playerCap > 3)
    {
        if (GuiButton((Rectangle){2 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "4"))
        {
            numPlayers = 4;
        }
    }
}

void InitMenu(void)
{
    scoreFont = LoadFont("assets/terminal/Mecha.ttf");
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    numPlayers = 0;
    numControllers = 2;
    cancellationRequested = false;

    for (int i = 0; i < sizeof(playerControllers) / sizeof(*playerControllers); i++)
    {
        playerControllers[i].controller = CONTROLLER_UNASSIGNED;
        playerControllers[i].status = Unassigned;
        playerControllers[i].description[0] = '\0';
    }
}

void FiniMenu(void)
{
    UnloadFont(scoreFont);
}

void HandleEdgeTriggeredEventsMenu(void)
{
}

ControllerId GetControllerAssignment(int player)
{
    return playerControllers[player].controller;
}

int GetNumberOfPlayers(void)
{
    return numPlayers;
}

void UpdateMenu(void)
{
    UpdateAvailableControllers();

    // If the number of confirmed controllers matches the number of players then start the game.
    int numConfirmed = 0;
    int numUnassigned = 0;
    for (int i = 0; i < numPlayers; i++)
    {
        if (playerControllers[i].controller == CONTROLLER_UNASSIGNED)
        {
            ++numUnassigned;
        }
        if (playerControllers[i].controller != CONTROLLER_UNASSIGNED && playerControllers[i].status == ConfirmedByPlayer)
        {
            if (++numConfirmed == numPlayers)
            {
                return; // PLAYING
            }
        }
    }

    if (cancellationRequested && numUnassigned == numPlayers)
    {
        cancellationRequested = false;
        numPlayers = 0;
    }

    return; // MENU;
}

void DrawMenu(double alpha)
{
    ClearBackground(BLACK);
    BeginDrawing();
    DrawText("MENU", SCREEN_WIDTH / 2, 4, 20, RAYWHITE);
    if (numPlayers == 0)
    {
        DrawPlayerSelection();
    }
    else
    {
        DrawControllerSelection();
    }
    EndDrawing();
}
