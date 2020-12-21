#pragma once

#include "controllers.h"

#include <stdbool.h>

void InitControllerSelection(void);
void FinishControllerSelection(void);
bool IsCancelledControllerSelection(void);
bool IsStartedControllerSelection(void);
void DrawControllerSelection(double alpha);
void UpdateControllerSelection(void);
void HandleEdgeTriggeredEventsControllerSelection(void);
ControllerId GetControllerAssignment(int player);
int GetNumberOfPlayers(void);
