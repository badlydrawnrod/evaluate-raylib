#include "controller_selection.h"

#include "controllers.h"
#include "raylib.h"

#define MAX_PLAYERS 4
#define MAX_CONTROLLERS 6

typedef enum
{
    WAITING,
    STARTABLE,
    STARTING,
    CANCELLED
} ControllerSelectionState;

typedef enum AssignmentStatus
{
    Unassigned,
    AssignedToPlayer,
    ConfirmedByPlayer
} AssignmentStatus;

// Player controllers and how they're assigned.
typedef struct
{
    ControllerId controller; // Which controller is assigned to this player, if any?
    AssignmentStatus status; // What's the status of this controller?
    char description[16];    // Description of the controller's status and assignment.
} PlayerController;

static float screenWidth;
static float screenHeight;

static int maxPlayers = 0;
static int numPlayers = 0;
PlayerController playerControllers[MAX_PLAYERS];

static int numControllers = 0;

static Font scoreFont;

static struct
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
static bool startRequested = false;
static ControllerSelectionState state;

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
    return Unassigned;
}

// Assign a controller.
static void AssignController(ControllerId controller)
{
    for (int i = 0; i < maxPlayers; i++)
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
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status != Unassigned)
        {
            if (playerControllers[i].status == ConfirmedByPlayer)
            {
                --numPlayers;
                TraceLog(LOG_INFO, TextFormat("There are now %d players", numPlayers));
            }
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
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status == AssignedToPlayer)
        {
            playerControllers[i].status = ConfirmedByPlayer;
            TextCopy(playerControllers[i].description, TextFormat("Confirmed [P%d]", i + 1));
            ++numPlayers;
            TraceLog(LOG_INFO, TextFormat("There are now %d players", numPlayers));
        }
    }
}

// Unconfirm a confirmed controller and put it back to assigned.
static void UnconfirmController(ControllerId controller)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == controller && playerControllers[i].status == ConfirmedByPlayer)
        {
            playerControllers[i].status = AssignedToPlayer;
            TextCopy(playerControllers[i].description, TextFormat("Assigned [P%d]", i + 1));
            --numPlayers;
            TraceLog(LOG_INFO, TextFormat("There are now %d players", numPlayers));
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
        case ConfirmedByPlayer:
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
        case Unassigned:
            cancellationRequested = true;
            break;
        case ConfirmedByPlayer:
            UnconfirmController(controller);
            break;
        case AssignedToPlayer:
            UnassignController(controller);
            break;
        default:
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
            case ConfirmedByPlayer:
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
            case Unassigned:
                cancellationRequested = true;
                break;
            case ConfirmedByPlayer:
                UnconfirmController(controller);
                break;
            case AssignedToPlayer:
                UnassignController(controller);
                break;
            default:
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

void InitControllerSelection(void)
{
    scoreFont = LoadFont("assets/terminal/Mecha.ttf");
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    state = WAITING;

    numPlayers = 0;
    numControllers = 2;
    maxPlayers = MAX_PLAYERS;
    cancellationRequested = false;
    startRequested = false;

    for (int i = 0; i < sizeof(playerControllers) / sizeof(*playerControllers); i++)
    {
        playerControllers[i].controller = CONTROLLER_UNASSIGNED;
        playerControllers[i].status = Unassigned;
        playerControllers[i].description[0] = '\0';
    }
}

void FinishControllerSelection(void)
{
    UnloadFont(scoreFont);
}

bool IsCancelledControllerSelection(void)
{
    return state == CANCELLED;
}

bool IsStartedControllerSelection(void)
{
    return state == STARTING;
}

void HandleEdgeTriggeredEventsControllerSelection(void)
{
    // Check player selections.
    CheckKeyboard(KEY_SPACE, KEY_W, CONTROLLER_KEYBOARD1);
    CheckKeyboard(KEY_ENTER, KEY_UP, CONTROLLER_KEYBOARD2);
    CheckGamepad(GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1);
    CheckGamepad(GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2);
    CheckGamepad(GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3);
    CheckGamepad(GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4);
}

ControllerId GetControllerAssignment(int player)
{
    return playerControllers[player].controller;
}

int GetNumberOfPlayers(void)
{
    return numPlayers;
}

void UpdateControllerSelection(void)
{
    int oldNumControllers = numControllers;
    int oldMaxPlayers = maxPlayers;

    UpdateAvailableControllers();

    // We can't have more players than controllers.
    maxPlayers = MinInt(MAX_PLAYERS, numControllers);

    if (numControllers != oldNumControllers)
    {
        TraceLog(LOG_INFO, TextFormat("Number of controllers changed from %d to %d", oldNumControllers, numControllers));
    }
    if (maxPlayers != oldMaxPlayers)
    {
        TraceLog(LOG_INFO, TextFormat("Max players changed from %d to %d", oldMaxPlayers, maxPlayers));
    }

    // Check if we can start the game.
    int numConfirmed = 0;
    int numUnassigned = 0;
    int numAssigned = 0;
    for (int i = 0; i < maxPlayers; i++)
    {
        if (playerControllers[i].controller == CONTROLLER_UNASSIGNED)
        {
            ++numUnassigned;
        }
        else if (playerControllers[i].status == ConfirmedByPlayer)
        {
            ++numConfirmed;
        }
        else if (playerControllers[i].status == AssignedToPlayer)
        {
            ++numAssigned;
        }
    }

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
            TraceLog(LOG_INFO, TextFormat("Starting game with %d players", numPlayers));
        }
    }

    const bool canCancel = numAssigned == 0 && numConfirmed == 0;
    if (cancellationRequested)
    {
        cancellationRequested = false;
        if (canCancel)
        {
            state = CANCELLED;
            TraceLog(LOG_INFO, "Cancelling");
        }
    }
}

void DrawControllerSelection(double alpha)
{
    ClearBackground(BLACK);
    BeginDrawing();

    DrawText("CONTROLLER SELECTION", 4, 4, 20, RAYWHITE);

    // Draw the controllers and whether they're unassigned, assigned, or confirmed.
    float width = screenWidth / numControllers;
    float controlWidth = width * 0.75f;
    float margin = (width - controlWidth) / 2;

    DrawTextEx(scoreFont, "Choose your controllers...", (Vector2){margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

    for (int i = 0; i < numControllers; i++)
    {
        for (int j = 0; j < sizeof(playerControllers) / sizeof(*playerControllers); j++)
        {
            if (playerControllers[j].controller == controllers[i].controller)
            {
                DrawTextRec(scoreFont, playerControllers[j].description,
                            (Rectangle){i * width + margin, 40.0f + screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2,
                            false, YELLOW);
            }
        }
        DrawTextRec(scoreFont, TextFormat("%-15s", controllers[i].description),
                    (Rectangle){i * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2, false,
                    RAYWHITE);
    }

    if (state == STARTABLE)
    {
        Vector2 size = MeasureTextEx(scoreFont, TextFormat("Start %d player game", numPlayers), 32, 2);
        Vector2 position = (Vector2){(screenWidth - size.x) / 2, 7 * screenHeight / 8};
        DrawTextEx(scoreFont, TextFormat("Start %d player game", numPlayers), position, 32, 2, YELLOW);
    }

    EndDrawing();
}
