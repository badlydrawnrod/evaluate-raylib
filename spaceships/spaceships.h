#pragma once

#include <stdbool.h>

#define MAX_PLAYERS 4

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

// Menu screen.
void InitMenu(void);          // Initialise the menu screen.
void FinishMenu(void);        // Tear down the menu screen.
bool IsStartingMenu(void);    // Check if the menu is ready for the game to start.
void DrawMenu(double alpha);  // Draw the menu screen.
void UpdateMenu(void);        // Update the menu screen.
void CheckTriggersMenu(void); // Allow the menu screen to handle edge-triggered events.

// Controls selection screen.
void InitControls(void);                          // Initialise the controls screen.
void FinishControls(void);                        // Tear down the controls screen.
bool IsCancelledControls(void);                   // Check if the controls screen is cancelled.
bool IsStartedControls(void);                     // Check if the controls screen is ready for the game to start.
void DrawControls(double alpha);                  // Draw the controls screen.
void UpdateControls(void);                        // Update the controls screen.
void CheckTriggersControls(void);                 // Allow the controls screen to handle edge-triggered events.
ControllerId GetControllerAssignment(int player); // Get the controller assigned to the given player.
int GetNumberOfPlayers(void);                     // Get the number of players.

// Playing screen.
void InitPlaying(int players, const ControllerId* controllers); // Initialise the playing screen.
void FinishPlaying(void);                                       // Tear down the playing screen.
bool IsCancelledPlaying(void);                                  // Check if the playing screen is cancelled.
void DrawPlaying(double alpha);                                 // Draw the playing screen.
void UpdatePlaying(void);                                       // Update the playing screen.
void CheckTriggersPlaying(void);                                // Allow the playing screen to handle edge-triggered events.
