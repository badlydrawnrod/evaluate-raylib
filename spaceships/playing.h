#pragma once

#include "controllers.h"

#include <stdbool.h>

void InitPlaying(int players, const ControllerId* controllers);
void FinishPlaying(void);
bool IsCancelledPlaying(void);
void DrawPlaying(double alpha);
void UpdatePlaying(void);
void HandleEdgeTriggeredEventsPlaying(void);
