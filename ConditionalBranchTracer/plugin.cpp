#include "plugin.h"
#include "resource.h"
#include <fstream>
#include <shlwapi.h>
#include <sstream>
#include <string>
#include <time.h>
#include <iomanip>
#include <codecvt>
#include <queue>


#define TRACE_FILE_NAME_MAX_SIZE 4096


using namespace std;


PLUG_EXPORT bool _dbg_isjumpgoingtoexecute(duint addr);


typedef enum
{
    TRACE_STEP_NONE,
    TRACE_STEP_OUT,
    TRACE_STEP_IN
} TraceType;

typedef struct
{
    duint cip;
    char instruction[64];
    bool executed;
} TraceEntry;

static bool g_traceEnabled = false;
static TraceType g_traceType = TRACE_STEP_NONE;
static wchar_t* g_pTarceFileName = new wchar_t[TRACE_FILE_NAME_MAX_SIZE];
static ofstream g_tarceFile;
static queue<TraceEntry> m_traceQueue;


INT_PTR CALLBACK StartTracingDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        _plugin_logprintf("[PLUGIN] %s v%d StartTracingDialogProc WM_INITDIALOG\n", PLUGIN_NAME, PLUGIN_VERSION);
        WCHAR moduleFileName[TRACE_FILE_NAME_MAX_SIZE] = { 0 };
        if (GetModuleFileName(NULL, moduleFileName, TRACE_FILE_NAME_MAX_SIZE) > 0)
        {
            PathRemoveFileSpec(moduleFileName);
            wstringstream wss;
            time_t now = time(NULL);
            tm ltm;
            localtime_s(&ltm, &now);
            wss << moduleFileName << L"\\" << put_time(&ltm, L"%Y.%m.%dT%H.%M.%S") << L".trace";
            HWND fileNameHwnd = GetDlgItem(hWnd, IDC_START_TRACING_FILENAME);
            SetWindowText(fileNameHwnd, (LPCWSTR)wss.str().c_str());
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_START_TRACING_BROWSE:
        {
            _plugin_logprintf("[PLUGIN] %s v%d StartTracingDialogProc IDC_START_TRACING_BROWSE\n", PLUGIN_NAME, PLUGIN_VERSION);
            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = g_pTarceFileName;
            ofn.nMaxFile = TRACE_FILE_NAME_MAX_SIZE;
            ofn.lpstrFilter = L"All\0*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLEHOOK;

            if (GetSaveFileName(&ofn))
            {
                HWND fileNameHwnd = GetDlgItem(hWnd, IDC_START_TRACING_FILENAME);
                SetWindowText(fileNameHwnd, g_pTarceFileName);
            }
            break;
        }

        case IDC_START_TRACING_START_STEPOVER:
        {
            _plugin_logprintf("[PLUGIN] %s v%d StartTracingDialogProc IDC_START_TRACING_START_STEPOVER\n", PLUGIN_NAME, PLUGIN_VERSION);
            HWND fileNameHwnd = GetDlgItem(hWnd, IDC_START_TRACING_FILENAME);
            GetWindowText(fileNameHwnd, g_pTarceFileName, TRACE_FILE_NAME_MAX_SIZE);
            g_traceType = TRACE_STEP_OUT;
            EndDialog(hWnd, IDOK);
            break;
        }

        case IDC_START_TRACING_START_STEPIN:
        {
            _plugin_logprintf("[PLUGIN] %s v%d StartTracingDialogProc IDC_START_TRACING_START_STEPIN\n", PLUGIN_NAME, PLUGIN_VERSION);
            HWND fileNameHwnd = GetDlgItem(hWnd, IDC_START_TRACING_FILENAME);
            GetWindowText(fileNameHwnd, g_pTarceFileName, TRACE_FILE_NAME_MAX_SIZE);
            g_traceType = TRACE_STEP_IN;
            EndDialog(hWnd, IDOK);
            break;
        }

        case IDC_START_TRACING_CANCEL:
        {
            _plugin_logprintf("[PLUGIN] %s v%d StartTracingDialogProc IDC_START_TRACING_CANCEL\n", PLUGIN_NAME, PLUGIN_VERSION);
            EndDialog(hWnd, IDCANCEL);
            break;
        }

        case WM_DESTROY:
        {
            _plugin_logprintf("[PLUGIN] %s v%d StartTracingDialogProc WM_DESTROY\n", PLUGIN_NAME, PLUGIN_VERSION);
            EndDialog(hWnd, IDCANCEL);
            break;
        }
        }
    }
    }
    return FALSE;
}

void stopTrace();
void startTrace()
{
    if (!g_traceEnabled)
    {
        g_tarceFile.open(g_pTarceFileName, ios::binary);
        if (g_tarceFile.is_open())
        {
            switch (g_traceType)
            {
            case TRACE_STEP_OUT:
                DbgCmdExec("tocnd 0 0");
                break;

            case TRACE_STEP_IN:
                DbgCmdExec("ticnd 0 0");
                break;

            default:
                stopTrace();
                MessageBox(NULL, L"Invalid trace type", L"Error", MB_OK | MB_ICONERROR);
                return;
                break;
            }
            g_traceEnabled = true;
        }
        else
        {
            MessageBox(NULL, L"Unable to open trace file to write", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void flushTrace()
{
    if (g_traceEnabled)
    {
        stringstream ss;
        while (!m_traceQueue.empty())
        {
            TraceEntry traceEntry = m_traceQueue.front();
            ss << hex << traceEntry.cip << ": ";
            ss << traceEntry.instruction;
            if (traceEntry.executed)
                ss << " executed\n";
            else
                ss << " not executed\n";
            m_traceQueue.pop();
        }
        g_tarceFile.write(ss.str().c_str(), ss.str().size());
        g_tarceFile.flush();
    }
}

void stopTrace()
{
    if (g_traceEnabled)
    {
        flushTrace();
        g_tarceFile.close();
        g_traceEnabled = false;
    }
}

PLUG_EXPORT void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
    switch (info->hEntry)
    {
    case MENU_START_TRACING:
    {
        if (DbgIsDebugging() && !g_traceEnabled)
        {
            _plugin_logprintf("[PLUGIN] %s v%d Showing start trace dialog\n", PLUGIN_NAME, PLUGIN_VERSION);
            if (DialogBox(g_hModule, MAKEINTRESOURCE(IDD_START_DIALOGBAR), NULL, &StartTracingDialogProc) == IDOK)
            {
                _plugin_logprintf("[PLUGIN] %s v%d Starting trace: %ws\n", PLUGIN_NAME, PLUGIN_VERSION, g_pTarceFileName);
                startTrace();
            }
            _plugin_logprintf("[PLUGIN] %s v%d Closed start trace dialog\n", PLUGIN_NAME, PLUGIN_VERSION);
        }
        break;
    }

    case MENU_STOP_TRACING:
    {
        _plugin_logprintf("[PLUGIN] %s v%d Stopping trace\n", PLUGIN_NAME, PLUGIN_VERSION);
        stopTrace();
        break;
    }

    case MENU_ABOUT:
    {
        wstring about = L"Conditional Branch Tracer v";
        about += to_wstring(PLUGIN_MAJOR_VERSION) + L"." + to_wstring(PLUGIN_MINOR_VERSION) + L"\n";
        about += L"Compiled at ";
        const char* pTimestamp = __TIMESTAMP__;
        auto pConvertedTimestamp = make_unique<wchar_t[]>(strlen(pTimestamp) + 1);
        size_t outSize;
        mbstowcs_s(&outSize, pConvertedTimestamp.get(), 32, pTimestamp, strlen(pTimestamp));
        about += pConvertedTimestamp.get();
        MessageBox(NULL, about.c_str(), L"About", MB_OK);
        break;
    }
    }
}

PLUG_EXPORT void CBINITDEBUG(CBTYPE cbType, PLUG_CB_INITDEBUG* info)
{
    _plugin_logprintf("[PLUGIN] %s v%d CBINITDEBUG\n", PLUGIN_NAME, PLUGIN_VERSION);
    stopTrace();
}

PLUG_EXPORT void CBSTOPDEBUG(CBTYPE cbType, PLUG_CB_INITDEBUG* info)
{
    _plugin_logprintf("[PLUGIN] %s v%d CBSTOPDEBUG\n", PLUGIN_NAME, PLUGIN_VERSION);
    stopTrace();
}

PLUG_EXPORT void CBSYSTEMBREAKPOINT(CBTYPE cbType, PLUG_CB_INITDEBUG* info)
{
    _plugin_logprintf("[PLUGIN] %s v%d CB_SYSTEMBREAKPOINT\n", PLUGIN_NAME, PLUGIN_VERSION);
    flushTrace();
}

PLUG_EXPORT void CBBREAKPOINT(CBTYPE cbType, PLUG_CB_INITDEBUG* info)
{
    _plugin_logprintf("[PLUGIN] %s v%d CB_BREAKPOINT\n", PLUGIN_NAME, PLUGIN_VERSION);
    flushTrace();
}

static BASIC_INSTRUCTION_INFO g_basicinfo;
static DISASM_INSTR g_instr;
PLUG_EXPORT void CBTRACEEXECUTE(CBTYPE cbType, PLUG_CB_TRACEEXECUTE* info)
{
    if (cbType == CB_TRACEEXECUTE)
    {
        DbgDisasmFastAt(info->cip, &g_basicinfo);
        if (g_basicinfo.branch && !g_basicinfo.call)
        {
            DbgDisasmAt(info->cip, &g_instr);

            TraceEntry traceEntry;
            traceEntry.cip = info->cip;
            memcpy_s(traceEntry.instruction, sizeof(traceEntry), g_instr.instruction, sizeof(g_instr.instruction));
            traceEntry.executed = _dbg_isjumpgoingtoexecute(info->cip);
            m_traceQueue.push(traceEntry);

            if (m_traceQueue.size() > 1024)
                flushTrace();
        }
        info->stop = false;
    }
}

bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
    return true;
}

bool pluginStop()
{
    return true;
}

void pluginSetup()
{
    HRSRC hPngRes = FindResource(g_hModule, MAKEINTRESOURCE(IDB_PNG), L"PNG");
    if (hPngRes)
    {
        HGLOBAL hPng = LoadResource(g_hModule, hPngRes);
        if (hPng)
        {
            ICONDATA iconData;
            iconData.data = LockResource(hPng);
            iconData.size = SizeofResource(g_hModule, hPngRes);
            _plugin_menuseticon(g_hMenu, &iconData);
            _plugin_logprintf("[PLUGIN] %s v%d Icon set with size: %d\n", PLUGIN_NAME, PLUGIN_VERSION, iconData.size);
        }
        else
        {
            _plugin_logprintf("[PLUGIN] %s v%d Unable to load icon\n", PLUGIN_NAME, PLUGIN_VERSION);
        }
    }
    else
    {
        _plugin_logprintf("[PLUGIN] %s v%d Png not found\n", PLUGIN_NAME, PLUGIN_VERSION);
    }

    _plugin_menuaddentry(g_hMenu, MENU_START_TRACING, "&Start tracing");
    _plugin_menuaddentry(g_hMenu, MENU_STOP_TRACING, "&Stop tracing");
    _plugin_menuaddentry(g_hMenu, MENU_ABOUT, "&About");
}
