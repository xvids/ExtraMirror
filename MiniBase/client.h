#pragma once
#include "main.h"
#include <map>

extern bool FirstFrame;
extern GameInfo_s BuildInfo;
void ConsolePrintColor(BYTE R, BYTE G, BYTE B, char* string);
void HookUserMessages();
void HookEngineMessages();
void HookFunction();