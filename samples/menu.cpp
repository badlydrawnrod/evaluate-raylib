#include "menu.h"

#include "config.h"
#include "controllers.h"
#include "raygui.h"
#include "screens.h"

namespace menu
{
    enum AssignmentStatus
    {
        Unassigned,
        AssignedToPlayer,
        ConfirmedByPlayer
    };

    float screenWidth;
    float screenHeight;

    constexpr int maxPlayers = 4;
    int numPlayers = 0;

    // Player controllers and how they're assigned.
    struct PlayerController
    {
        ControllerId controller; // Which controller is assigned to this player, if any?
        AssignmentStatus status; // What's the status of this controller?
        char description[16];    // Description of the controller's status and assignment.
    } playerControllers[maxPlayers];

    int numControllers;
    struct ControllerInfo
    {
        ControllerId controller;
        const char* description;
    } controllers[6];

    constexpr struct
    {
        GamepadNumber gamepadNumber;
        ControllerId controllerId;
        const char* description;
    } gamepadDescriptors[4] = {{GAMEPAD_PLAYER1, CONTROLLER_GAMEPAD1, "Gamepad 1"},
                               {GAMEPAD_PLAYER2, CONTROLLER_GAMEPAD2, "Gamepad 2"},
                               {GAMEPAD_PLAYER3, CONTROLLER_GAMEPAD3, "Gamepad 3"},
                               {GAMEPAD_PLAYER4, CONTROLLER_GAMEPAD4, "Gamepad 4"}};

    bool requestCancellation;

    void Start()
    {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        numPlayers = 0;
        numControllers = 2;
        requestCancellation = false;

        for (auto& playerController : playerControllers)
        {
            playerController.controller = CONTROLLER_UNASSIGNED;
            playerController.status = Unassigned;
            playerController.description[0] = '\0';
        }
    }

    ControllerId GetControllerAssignment(int player)
    {
        return playerControllers[player].controller;
    }

    int GetNumberOfPlayers()
    {
        return numPlayers;
    }

    AssignmentStatus GetControllerStatus(ControllerId controller)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            const auto& controllerAssignment = playerControllers[i];
            if (controllerAssignment.controller == controller)
            {
                return controllerAssignment.status;
            }
        }
        return Unassigned;
    }

    void AssignController(ControllerId controller)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            auto& controllerAssignment = playerControllers[i];
            if (controllerAssignment.status == Unassigned)
            {
                controllerAssignment.controller = controller;
                controllerAssignment.status = AssignedToPlayer;
                TextCopy(controllerAssignment.description, TextFormat("Assigned [P%d]", i + 1));
                return;
            }
        }
    }

    void UnassignController(ControllerId controller)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            auto& controllerAssignment = playerControllers[i];
            if (controllerAssignment.controller == controller && controllerAssignment.status != Unassigned)
            {
                controllerAssignment.controller = CONTROLLER_UNASSIGNED;
                controllerAssignment.status = Unassigned;
                TextCopy(controllerAssignment.description, "");
                return;
            }
        }
    }

    void ConfirmController(ControllerId controller)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            auto& controllerAssignment = playerControllers[i];
            if (controllerAssignment.controller == controller && controllerAssignment.status == AssignedToPlayer)
            {
                controllerAssignment.status = ConfirmedByPlayer;
                TextCopy(controllerAssignment.description, TextFormat("Confirmed [P%d]", i + 1));
            }
        }
    }

    void UnconfirmController(ControllerId controller)
    {
        for (int i = 0; i < numPlayers; i++)
        {
            auto& controllerAssignment = playerControllers[i];
            if (controllerAssignment.controller == controller && controllerAssignment.status == ConfirmedByPlayer)
            {
                controllerAssignment.status = AssignedToPlayer;
                TextCopy(controllerAssignment.description, TextFormat("Assigned [P%d]", i + 1));
            }
        }
    }

    void CheckKeyboard(KeyboardKey selectKey, KeyboardKey cancelKey, ControllerId controller)
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
                requestCancellation = true;
                break;
            }
        }
    }

    void CheckGamepad(GamepadNumber gamepad, ControllerId controller)
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
                    requestCancellation = true;
                    break;
                }
            }
        }
    }

    void UpdateAvailableControllers()
    {
        // Always check which controllers are available as this can change from frame to frame.
        numControllers = 0;
        controllers[numControllers].controller = CONTROLLER_KEYBOARD1;
        controllers[numControllers].description = "Left Keyboard";
        ++numControllers;

        controllers[numControllers].controller = CONTROLLER_KEYBOARD2;
        controllers[numControllers].description = "Right Keyboard";
        ++numControllers;

        for (const auto& gamepad : gamepadDescriptors)
        {
            if (IsGamepadAvailable(gamepad.gamepadNumber))
            {
                controllers[numControllers].controller = gamepad.controllerId;
                controllers[numControllers].description = gamepad.description;
                ++numControllers;
            }
            else
            {
                UnassignController(gamepad.controllerId);
            }
        }
    }

    ScreenState FixedUpdate()
    {
        UpdateAvailableControllers();

        // If the number of confirmed controllers matches the number of players then start the game.
        int numConfirmed = 0;
        int numUnassigned = 0;
        for (int i = 0; i < numPlayers; i++)
        {
            const auto& controllerAssignment = playerControllers[i];
            if (controllerAssignment.controller == CONTROLLER_UNASSIGNED)
            {
                ++numUnassigned;
            }
            if (controllerAssignment.controller != CONTROLLER_UNASSIGNED && controllerAssignment.status == ConfirmedByPlayer)
            {
                if (++numConfirmed == numPlayers)
                {
                    return PLAYING;
                }
            }
        }

        if (requestCancellation && numUnassigned == numPlayers)
        {
            requestCancellation = false;
            numPlayers = 0;
        }

        return MENU;
    }

    void DrawControllerSelection()
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

        DrawTextEx(scoreFont, "Pick controls", {margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

        for (int i = 0; i < numControllers; i++)
        {
            char description[128];
            int bytes = TextCopy(description, TextFormat("%-15s", controllers[i].description));

            for (const auto& p : playerControllers)
            {
                if (p.controller == controllers[i].controller)
                {
                    bytes += TextCopy(description + bytes, "\n");
                    TextCopy(description + bytes, p.description);
                    GuiLabel({i * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, p.description);
                }
            }
            GuiButton({i * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, description);
        }
    }

    void DrawPlayerSelection()
    {
        int playerCap = numControllers >= 4 ? 4 : numControllers;
        float width = screenWidth / (playerCap - 1);
        float controlWidth = width * 0.75f;
        float margin = (width - controlWidth) / 2;

        DrawTextEx(scoreFont, "How many players?", {margin, screenHeight / 4.0f}, 32, 2, RAYWHITE);

        if (GuiButton({0 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "2"))
        {
            numPlayers = 2;
        }
        if (playerCap > 2)
        {
            if (GuiButton({1 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "3"))
            {
                numPlayers = 3;
            }
        }
        if (playerCap > 3)
        {
            if (GuiButton({2 * width + margin, screenHeight / 2.0f, controlWidth, screenHeight / 8.0f}, "4"))
            {
                numPlayers = 4;
            }
        }
    }

    void Draw()
    {
        if (numPlayers == 0)
        {
            DrawPlayerSelection();
        }
        else
        {
            DrawControllerSelection();
        }
    }
} // namespace menu
