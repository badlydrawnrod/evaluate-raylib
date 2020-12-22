#include "raylib.h"
#include "spaceships.h"

#define MAX_CONTROLLERS 6
#define MAX_GAMEPADS 4

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

static int screenWidth;
static int screenHeight;

static int maxPlayers = 0;
static int numPlayers = 0;
static PlayerController playerControllers[MAX_PLAYERS];

static int numControllers = 0;
static int numActive = 0;

static Font scoreFont;

// Currently available controllers.
static struct
{
    ControllerId controller;           // The controller id, e.g., CONTROLLER_GAMEPAD1.
    const char* description;           // The controller's description, e.g., "Gamepad 1".
    const char* cancelDescription;     // Tell the player how to cancel.
    const char* unassignedDescription; // The description to show when the controller is not assigned.
    const char* assignedDescription;   // The description to show when the controller is assigned.
    const char* confirmedDescription;  // The description to show when the controller is confirmed.
} controllers[MAX_CONTROLLERS];

// Gamepad information.
static struct
{
    GamepadNumber gamepadNumber;       // Which raylib gamepad is this?
    ControllerId controllerId;         // Which controller is this?
    const char* description;           // What do we display to the player?
    const char* cancelDescription;     // Tell the player how to cancel.
    const char* unassignedDescription; // The description to show when the gamepad is not assigned.
    const char* assignedDescription;   // The description to show when the gamepad is assigned.
    const char* confirmedDescription;  // The description to show when the gamepad is confirmed.
} gamepadDescriptors[MAX_GAMEPADS] = {
        {GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1, "Gamepad 1", "[B]", "[A] Select", "[A] Confirm", "[A] Start\nPlayers = %d"},
        {GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2, "Gamepad 2", "[B]", "[A] Select", "[A] Confirm", "[A] Start\nPlayers = %d"},
        {GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3, "Gamepad 3", "[B]", "[A] Select", "[A] Confirm", "[A] Start\nPlayers = %d"},
        {GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4, "Gamepad 4", "[B]", "[A] Select", "[A] Confirm", "[A] Start\nPlayers = %d"}};

static bool cancellationRequested = false;
static bool startRequested = false;
static bool canCancel = true;
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

// Check a gamepad for selection / Cancellation.
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
    controllers[numControllers].cancelDescription = "[W]";
    controllers[numControllers].unassignedDescription = "[S] Select";
    controllers[numControllers].assignedDescription = "[S] Confirm";
    controllers[numControllers].confirmedDescription = "[S] Start\nPlayers = %d";
    ++numControllers;

    controllers[numControllers].controller = CONTROLLER_KEYBOARD2;
    controllers[numControllers].description = "Right Keyboard";
    controllers[numControllers].cancelDescription = "[Up]";
    controllers[numControllers].unassignedDescription = "[Down] Select";
    controllers[numControllers].assignedDescription = "[Down] Confirm";
    controllers[numControllers].confirmedDescription = "[Down] Start\nPlayers = %d";
    ++numControllers;

    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (IsGamepadAvailable(gamepadDescriptors[i].gamepadNumber))
        {
            controllers[numControllers].controller = gamepadDescriptors[i].controllerId;
            controllers[numControllers].description = gamepadDescriptors[i].description;
            controllers[numControllers].cancelDescription = gamepadDescriptors[i].cancelDescription;
            controllers[numControllers].unassignedDescription = gamepadDescriptors[i].unassignedDescription;
            controllers[numControllers].assignedDescription = gamepadDescriptors[i].assignedDescription;
            controllers[numControllers].confirmedDescription = gamepadDescriptors[i].confirmedDescription;
            ++numControllers;
        }
        else
        {
            UnassignController(gamepadDescriptors[i].controllerId);
        }
    }
}

void InitControls(void)
{
    scoreFont = LoadFont("assets/terminal/Mecha.ttf");
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    state = WAITING;

    numPlayers = 0;
    numControllers = 2;
    numActive = 0;
    maxPlayers = MAX_PLAYERS;
    cancellationRequested = false;
    startRequested = false;
    canCancel = true;

    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        playerControllers[i].controller = CONTROLLER_UNASSIGNED;
        playerControllers[i].status = Unassigned;
        playerControllers[i].description[0] = '\0';
    }
}

void FinishControls(void)
{
    UnloadFont(scoreFont);
}

bool IsCancelledControls(void)
{
    return state == CANCELLED;
}

bool IsStartedControls(void)
{
    return state == STARTING;
}

void CheckTriggersControls(void)
{
    // Check player selections.
    CheckKeyboard(KEY_S, KEY_W, CONTROLLER_KEYBOARD1);
    CheckKeyboard(KEY_DOWN, KEY_UP, CONTROLLER_KEYBOARD2);
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

void UpdateControls(void)
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
            TraceLog(LOG_INFO, TextFormat("Starting game with %d players", numPlayers));
        }
    }

    canCancel = numAssigned == 0 && numConfirmed == 0;
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

void DrawControls(double alpha)
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
        AssignmentStatus status = Unassigned;
        for (int j = 0; j < MAX_PLAYERS; j++)
        {
            if (playerControllers[j].controller == controllers[i].controller)
            {
                status = playerControllers[j].status;
            }
        }

        // Unassigned controllers are disabled if the number of controllers active matches the maximum number of players.
        const bool isEnabled = status != Unassigned || numActive < maxPlayers;

        // Describe the controller.
        DrawTextRec(scoreFont, TextFormat("%-15s", controllers[i].description),
                    (Rectangle){i * width + margin, (float)screenHeight / 2.0f, controlWidth, (float)screenHeight / 8.0f}, 32, 2,
                    false, isEnabled ? RAYWHITE : GRAY);

        if (status == Unassigned)
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

        switch (status)
        {
        case Unassigned:
            DrawTextRec(scoreFont, controllers[i].unassignedDescription,
                        (Rectangle){i * width + margin, 40.0f + screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2,
                        false, isEnabled ? GREEN : GRAY);
            break;
        case AssignedToPlayer:
            DrawTextRec(scoreFont, controllers[i].assignedDescription,
                        (Rectangle){i * width + margin, 40.0f + screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2,
                        false, isEnabled ? GREEN : GRAY);
            break;
        case ConfirmedByPlayer:
            DrawTextRec(scoreFont, TextFormat(controllers[i].confirmedDescription, numPlayers),
                        (Rectangle){i * width + margin, 40.0f + screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, 32, 2,
                        false, (isEnabled && state == STARTABLE) ? GREEN : GRAY);
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

    EndDrawing();
}
