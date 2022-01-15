#include "draw_text_rec/draw_text_rec.h"
#include "raylib.h"
#include "spaceships.h"

#define MAX_KEYBOARDS 2
#define MAX_GAMEPADS 4
#define MAX_CONTROLLERS (MAX_KEYBOARDS + MAX_GAMEPADS)

// Screen states.
typedef enum
{
    WAITING,
    STARTABLE,
    STARTING,
    CANCELLED
} ControlsState;

// Controller assignment status.
typedef enum
{
    UNASSIGNED,
    ASSIGNED_TO_PLAYER,
    CONFIRMED_BY_PLAYER
} AssignmentStatus;

// Player controllers and how they're assigned.
typedef struct
{
    ControllerId controller; // Which controller is assigned to this player, if any?
    AssignmentStatus status; // What's the status of this controller?
} PlayerController;

static int screenWidth;
static int screenHeight;

static int maxPlayers = 0;
static int numPlayers = 0;
static PlayerController playerControllers[MAX_PLAYERS];

static int numControllers = 0;
static int numAssigned = 0;
static int numConfirmed = 0;
static int numActive = 0;

static Font scoreFont;

// Currently available controllers.
static struct
{
    ControllerId controller;       // The controller id, e.g., CONTROLLER_GAMEPAD1.
    const char* description;       // The controller's description, e.g., "Gamepad 1".
    const char* cancelDescription; // Tell the player how to cancel.
    const char* selectDescription; // Tell the player how to select.
} controllers[MAX_CONTROLLERS];

// Keyboard information.
static struct
{
    ControllerId controllerId;     // Which controller is this?
    const char* description;       // What do we display to the player?
    const char* cancelDescription; // Tell the player how to cancel.
    const char* selectDescription; // Tell the player how to select.
} keyboardDescriptors[MAX_KEYBOARDS] = {{CONTROLLER_KEYBOARD1, "Left keyboard", "[W]", "[S]"},
                                        {CONTROLLER_KEYBOARD2, "Right keyboard", "[Up]", "[Down]"}};

// Gamepad information.
static struct
{
    GamepadNumber gamepadNumber;   // Which raylib gamepad is this?
    ControllerId controllerId;     // Which controller is this?
    const char* description;       // What do we display to the player?
    const char* cancelDescription; // Tell the player how to cancel.
    const char* selectDescription; // Tell the player how to select.
} gamepadDescriptors[MAX_GAMEPADS] = {{GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1, "Gamepad 1", "(B)", "(A)"},
                                      {GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2, "Gamepad 2", "(B)", "(A)"},
                                      {GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3, "Gamepad 3", "(B)", "(A)"},
                                      {GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4, "Gamepad 4", "(B)", "(A)"}};

static bool cancellationRequested = false;
static bool startRequested = false;
static bool canCancel = true;
static ControlsState state;

static int MinInt(int a, int b)
{
    return a < b ? a : b;
}

// Determine if a controller is unassigned, assigned, or confirmed.
static AssignmentStatus GetControllerStatus(ControllerId controller)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == controller)
        {
            return playerControllers[i].status;
        }
    }
    return UNASSIGNED;
}

// Assign a controller.
static void AssignController(ControllerId controller)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].status == UNASSIGNED)
        {
            playerControllers[i].controller = controller;
            playerControllers[i].status = ASSIGNED_TO_PLAYER;
            return;
        }
    }
}

// Unassign a controller.
static void UnassignController(ControllerId controller)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status != UNASSIGNED)
        {
            if (playerControllers[i].status == CONFIRMED_BY_PLAYER)
            {
                --numPlayers;
            }

            // Move the controllers that follow this one down by a slot so that controllers always appear in order of assignment.
            for (int j = i + 1; j < maxPlayers; j++)
            {
                playerControllers[j - 1] = playerControllers[j];
            }

            // Unassign the final controller.
            playerControllers[maxPlayers - 1].controller = CONTROLLER_UNASSIGNED;
            playerControllers[maxPlayers - 1].status = UNASSIGNED;
            return;
        }
    }
}

// Confirm an assigned controller.
static void ConfirmController(ControllerId controller)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status == ASSIGNED_TO_PLAYER)
        {
            playerControllers[i].status = CONFIRMED_BY_PLAYER;
            ++numPlayers;
        }
    }
}

// Unconfirm a confirmed controller and put it back to assigned.
static void UnconfirmController(ControllerId controller)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status == CONFIRMED_BY_PLAYER)
        {
            playerControllers[i].status = ASSIGNED_TO_PLAYER;
            --numPlayers;
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
        case UNASSIGNED:
            AssignController(controller);
            break;
        case ASSIGNED_TO_PLAYER:
            ConfirmController(controller);
            break;
        case CONFIRMED_BY_PLAYER:
            startRequested = true;
            break;
        default:
            break;
        }
    }
    if (IsKeyReleased(cancelKey))
    {
        switch (GetControllerStatus(controller))
        {
        case UNASSIGNED:
            cancellationRequested = true;
            break;
        case CONFIRMED_BY_PLAYER:
            UnconfirmController(controller);
            break;
        case ASSIGNED_TO_PLAYER:
            UnassignController(controller);
            break;
        default:
            break;
        }
    }
}

// Check a gamepad for selection / Cancellation.
static void CheckGamepad(GamepadNumber gamepad, ControllerId controller)
{
    if (IsGamepadAvailable(gamepad))
    {
        if (IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
        {
            switch (GetControllerStatus(controller))
            {
            case UNASSIGNED:
                AssignController(controller);
                break;
            case ASSIGNED_TO_PLAYER:
                ConfirmController(controller);
                break;
            case CONFIRMED_BY_PLAYER:
                startRequested = true;
                break;
            default:
                break;
            }
        }
        if (IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
        {
            switch (GetControllerStatus(controller))
            {
            case UNASSIGNED:
                cancellationRequested = true;
                break;
            case CONFIRMED_BY_PLAYER:
                UnconfirmController(controller);
                break;
            case ASSIGNED_TO_PLAYER:
                UnassignController(controller);
                break;
            default:
                break;
            }
        }
    }
}

// Check the keyboard for quitting this screen.
static void CheckKeyboardQuit(KeyboardKey quitKey)
{
    if (IsKeyReleased(quitKey))
    {
        state = CANCELLED;
    }
}

// Check a gamepad for quitting this screen.
static void CheckGamepadQuit(GamepadNumber gamepad, GamepadButton quitButton)
{
    if (IsGamepadAvailable(gamepad) && IsGamepadButtonReleased(gamepad, quitButton))
    {
        state = CANCELLED;
    }
}

// Check which controllers are available as this can change from frame to frame.
static void UpdateAvailableControllers(void)
{
    numControllers = 0;

    // On desktop, the keyboard is always available.
    for (int i = 0; i < MAX_KEYBOARDS; i++)
    {
        controllers[numControllers].controller = keyboardDescriptors[i].controllerId;
        controllers[numControllers].description = keyboardDescriptors[i].description;
        controllers[numControllers].cancelDescription = keyboardDescriptors[i].cancelDescription;
        controllers[numControllers].selectDescription = keyboardDescriptors[i].selectDescription;
        ++numControllers;
    }

    // Check gamepad availability.
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (IsGamepadAvailable(gamepadDescriptors[i].gamepadNumber))
        {
            controllers[numControllers].controller = gamepadDescriptors[i].controllerId;
            controllers[numControllers].description = gamepadDescriptors[i].description;
            controllers[numControllers].cancelDescription = gamepadDescriptors[i].cancelDescription;
            controllers[numControllers].selectDescription = gamepadDescriptors[i].selectDescription;
            ++numControllers;
        }
        else
        {
            UnassignController(gamepadDescriptors[i].controllerId);
        }
    }
}

void InitControlsScreen(void)
{
    scoreFont = LoadFont("assets/Mecha.ttf");
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    state = WAITING;

    numPlayers = 0;
    numControllers = 2;
    numAssigned = 0;
    numConfirmed = 0;
    numActive = 0;
    maxPlayers = MAX_PLAYERS;
    cancellationRequested = false;
    startRequested = false;
    canCancel = true;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        playerControllers[i].controller = CONTROLLER_UNASSIGNED;
        playerControllers[i].status = UNASSIGNED;
    }
}

void FinishControlsScreen(void)
{
    UnloadFont(scoreFont);
}

void UpdateControlsScreen(void)
{
    UpdateAvailableControllers();

    // We can't have more players than controllers.
    maxPlayers = MinInt(MAX_PLAYERS, numControllers);

    // Check if we can start the game.
    numConfirmed = 0;
    numAssigned = 0;
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].status == CONFIRMED_BY_PLAYER)
        {
            ++numConfirmed;
        }
        else if (playerControllers[i].status == ASSIGNED_TO_PLAYER)
        {
            ++numAssigned;
        }
    }
    numActive = numAssigned + numConfirmed;

    const bool canStart = numConfirmed > 0 && numAssigned == 0;
    if (state == WAITING && canStart)
    {
        state = STARTABLE;
    }
    else if (state == STARTABLE && !canStart)
    {
        state = WAITING;
    }

    if (startRequested)
    {
        startRequested = false;
        if (canStart)
        {
            state = STARTING;
        }
    }

    canCancel = numAssigned == 0 && numConfirmed == 0;
    if (cancellationRequested)
    {
        cancellationRequested = false;
        if (canCancel)
        {
            state = CANCELLED;
        }
    }
}

void DrawControlsScreen(double alpha)
{
    (void)alpha;
    ClearBackground(BLACK);
    BeginDrawing();

    DrawText("CONTROLLER SELECTION", 4, 4, 20, RAYWHITE);

    // Draw the controllers and whether they're unassigned, assigned, or confirmed.
    float width = (float)(screenWidth / numControllers);
    float controlWidth = width * 0.75f;
    float margin = (width - controlWidth) / 2;

    DrawTextEx(scoreFont, "Choose your controllers...", (Vector2){margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

    for (int i = 0; i < numControllers; i++)
    {
        // Get the controller's status.
        AssignmentStatus status = UNASSIGNED;
        for (int j = 0; j < MAX_PLAYERS; j++)
        {
            if (playerControllers[j].controller == controllers[i].controller)
            {
                status = playerControllers[j].status;
            }
        }

        // Unassigned controllers are disabled if the number of active controllers matches the maximum number of players.
        const bool isEnabled = status != UNASSIGNED || numActive < maxPlayers;

        // Describe the controller.
        DrawTextRec(scoreFont, TextFormat("%-15s", controllers[i].description),
                    (Rectangle){i * width + margin, (float)screenHeight / 2.0f, controlWidth, (float)screenHeight / 8.0f}, 32, 2,
                    false, isEnabled ? RAYWHITE : GRAY);

        // Draw return to menu / back.
        if (status == UNASSIGNED)
        {
            DrawTextRec(scoreFont, TextFormat("%s Return to menu", controllers[i].cancelDescription),
                        (Rectangle){i * width + margin, screenHeight / 2.0f - 20.0f, controlWidth, screenHeight / 8.0f}, 16, 2,
                        false, canCancel ? RED : GRAY);
        }
        else
        {
            DrawTextRec(scoreFont, TextFormat("%s Back", controllers[i].cancelDescription),
                        (Rectangle){i * width + margin, screenHeight / 2.0f - 20.0f, controlWidth, screenHeight / 8.0f}, 16, 2,
                        false, isEnabled ? ORANGE : GRAY);
        }

        // Draw select.
        switch (status)
        {
        case UNASSIGNED:
            DrawTextRec(scoreFont, TextFormat("%s Select", controllers[i].selectDescription),
                        (Rectangle){i * width + margin, 40.0f + screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2,
                        false, isEnabled ? LIME : GRAY);
            break;
        case ASSIGNED_TO_PLAYER:
            DrawTextRec(scoreFont, TextFormat("%s Confirm", controllers[i].selectDescription),
                        (Rectangle){i * width + margin, 40.0f + screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2,
                        false, isEnabled ? LIME : GRAY);
            break;
        case CONFIRMED_BY_PLAYER:
            DrawTextRec(scoreFont, TextFormat("%s Start\nPlayers %d", controllers[i].selectDescription, numPlayers),
                        (Rectangle){i * width + margin, 40.0f + screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2,
                        false, (isEnabled && state == STARTABLE) ? LIME : GRAY);
            break;
        default:
            break;
        }
    }

    if (state == STARTABLE)
    {
        Vector2 size = MeasureTextEx(scoreFont, TextFormat("Start %d player game", numPlayers), 32, 2);
        Vector2 position = (Vector2){((float)screenWidth - size.x) / 2, 7 * (float)screenHeight / 8};
        DrawTextEx(scoreFont, TextFormat("Start %d player game", numPlayers), position, 32, 2, LIME);
    }
    else if (numConfirmed > 0)
    {
        Vector2 size = MeasureTextEx(scoreFont, TextFormat("Waiting for %d player(s)", numAssigned), 32, 2);
        Vector2 position = (Vector2){((float)screenWidth - size.x) / 2, 7 * (float)screenHeight / 8};
        DrawTextEx(scoreFont, TextFormat("Waiting for %d player(s)", numAssigned), position, 32, 2, ORANGE);
    }

    EndDrawing();
}

void CheckTriggersControlsScreen(void)
{
    // Check player selections.
    CheckKeyboard(KEY_S, KEY_W, CONTROLLER_KEYBOARD1);
    CheckKeyboard(KEY_DOWN, KEY_UP, CONTROLLER_KEYBOARD2);
    CheckGamepad(GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1);
    CheckGamepad(GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2);
    CheckGamepad(GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3);
    CheckGamepad(GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4);

    // Check if this screen should be abandoned.
    CheckKeyboardQuit(KEY_ESCAPE);
    CheckGamepadQuit(GAMEPAD_PLAYER1, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    CheckGamepadQuit(GAMEPAD_PLAYER2, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    CheckGamepadQuit(GAMEPAD_PLAYER3, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    CheckGamepadQuit(GAMEPAD_PLAYER4, GAMEPAD_BUTTON_MIDDLE_RIGHT);
}

bool IsCancelledControlsScreen(void)
{
    return state == CANCELLED;
}

bool IsStartedControlsScreen(void)
{
    return state == STARTING;
}

ControllerId GetControllerAssignment(int player)
{
    return playerControllers[player].controller;
}

int GetNumberOfPlayers(void)
{
    return numPlayers;
}
