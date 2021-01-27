#pragma once

#include "pluginmain.h"

#define PLUGIN_NAME "Conditional Branch Tracer"
#define PLUGIN_MAJOR_VERSION 1
#define PLUGIN_MINOR_VERSION 0
#define PLUGIN_VERSION ((PLUGIN_MAJOR_VERSION * 10) + PLUGIN_MINOR_VERSION)

#define MENU_START_TRACING 0
#define MENU_STOP_TRACING 1
#define MENU_ABOUT 2


bool pluginInit(PLUG_INITSTRUCT* initStruct);
bool pluginStop();
void pluginSetup();
