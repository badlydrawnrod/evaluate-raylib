#pragma once

#include <stdbool.h>

#define MAX_PLAYERS 4

// Gamepad number (as it seems to have vanished from raylib.h)
typedef enum
{
    GAMEPAD_PLAYER1 = 0,
    GAMEPAD_PLAYER2 = 1,
    GAMEPAD_PLAYER3 = 2,
    GAMEPAD_PLAYER4 = 3
} GamepadNumber;

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

// clang-format off

// Menu screen.
void InitMenuScreen(void);                                      // Initialise the menu screen.
void FinishMenuScreen(void);                                    // Tear down the menu screen.
void UpdateMenuScreen(void);                                    // Update the menu screen.
void DrawMenuScreen(double alpha);                              // Draw the menu screen.
void CheckTriggersMenuScreen(void);                             // Allow the menu screen to handle edge-triggered events.
bool IsStartedMenuScreen(void);                                 // Check if the menu is ready for the game to start.
bool IsCancelledMenuScreen(void);                               // Check if the menu is ready for the program to end.

// Controls selection screen.
void InitControlsScreen(void);                                  // Initialise the controls screen.
void FinishControlsScreen(void);                                // Tear down the controls screen.
void UpdateControlsScreen(void);                                // Update the controls screen.
void DrawControlsScreen(double alpha);                          // Draw the controls screen.
void CheckTriggersControlsScreen(void);                         // Allow the controls screen to handle edge-triggered events.
bool IsCancelledControlsScreen(void);                           // Check if the controls screen is cancelled.
bool IsStartedControlsScreen(void);                             // Check if the controls screen is ready for the game to start.
ControllerId GetControllerAssignment(int player);               // Get the controller assigned to the given player.
int GetNumberOfPlayers(void);                                   // Get the number of players.

// Playing screen.
void InitPlayingScreen(int players, const ControllerId* controllers); // Initialise the playing screen.
void FinishPlayingScreen(void);                                 // Tear down the playing screen.
void UpdatePlayingScreen(void);                                 // Update the playing screen.
void DrawPlayingScreen(double alpha);                           // Draw the playing screen.
void CheckTriggersPlayingScreen(void);                          // Allow the playing screen to handle edge-triggered events.
bool IsCancelledPlayingScreen(void);                            // Check if the playing screen is cancelled.

// clang-format on
