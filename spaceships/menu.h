#pragma once

#include <stdbool.h>

void InitMenu(void);
void FinishMenu(void);
bool IsStartingMenu(void);
void DrawMenu(double alpha);
void UpdateMenu(void);
void HandleEdgeTriggeredEventsMenu(void);
