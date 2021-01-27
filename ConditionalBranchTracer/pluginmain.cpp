#include "pluginmain.h"
#include "plugin.h"


HMODULE g_hModule = NULL;
int g_pluginHandle = 0;
HWND g_hwndDlg = NULL;
int g_hMenu = 0;
int g_hMenuDisasm = 0;
int g_hMenuDump = 0;
int g_hMenuStack = 0;


PLUG_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    _plugin_logprintf("[PLUGIN] %s v%d Initialized!\n", PLUGIN_NAME, PLUGIN_VERSION);
    initStruct->pluginVersion = PLUGIN_VERSION;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, PLUGIN_NAME, _TRUNCATE);
    g_pluginHandle = initStruct->pluginHandle;
    return pluginInit(initStruct);
}

PLUG_EXPORT bool plugstop()
{
    _plugin_logprintf("[PLUGIN] %s v%d Stopped!\n", PLUGIN_NAME, PLUGIN_VERSION);
    return pluginStop();
}

PLUG_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setupStruct)
{
    _plugin_logprintf("[PLUGIN] %s v%d Setup!\n", PLUGIN_NAME, PLUGIN_VERSION);
    g_hwndDlg = setupStruct->hwndDlg;
    g_hMenu = setupStruct->hMenu;
    g_hMenuDisasm = setupStruct->hMenuDisasm;
    g_hMenuDump = setupStruct->hMenuDump;
    g_hMenuStack = setupStruct->hMenuStack;
    pluginSetup();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    g_hModule = hModule;
    return TRUE;
}
