#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <highlevelmonitorconfigurationapi.h>
#include <thread>
#include <memory>
#include <string>
#include "datetime.h"
// Windows Header Files:
#include <Windowsx.h>
#include <commctrl.h>
#include <Shellapi.h>
#include <Shlwapi.h>

#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")
#include "resource.h"
#include "registry.h"

using namespace std;

static TCHAR szWindowClass[] = _T("AutoBrightness");
static TCHAR szTitle[] = _T("Auto Brightness v0.1");
HINSTANCE		hInst;	// current instance
NOTIFYICONDATA	niData;	// notify icon data
#define TRAYICONID	1//				ID number for the Notify Icon
#define SWM_TRAYMSG	WM_APP//		the message ID sent to our window

#define SWM_SHOW	WM_APP + 1//	show the window
#define SWM_HIDE	WM_APP + 2//	hide the window
#define SWM_EXIT	WM_APP + 3//	close the window

#define IDR_MANIFEST                    1
#define IDD_DLG_DIALOG                  102
#define IDD_ABOUTBOX                    103
#define IDM_ABOUT                       104
#define IDC_MYICON						105
#define IDC_STEALTHDIALOG				106
#define IDI_STEALTHDLG                  107
#define IDC_STATIC                      -1

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
std::shared_ptr<std::thread> workerthread;

void workerfunc();
BOOL				InitInstance(HINSTANCE, int);
void				ShowContextMenu(HWND hWnd);
ULONGLONG			GetDllVersion(LPCTSTR lpszDllName);
INT_PTR CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);

void set_brightness(int val);
int get_brightness();


HHOOK hKeyboardHook = 0;
DWORD dw;
HMONITOR hMonitor = NULL;
DWORD cPhysicalMonitors;
LPPHYSICAL_MONITOR pPhysicalMonitors = NULL;
POINT pt;
HANDLE pmh;


void SetNumLock(BOOL bState)
{
    BYTE keyState[256];

    GetKeyboardState((LPBYTE)&keyState);
    if ((bState && !(keyState[VK_SCROLL] & 1)) ||
        (!bState && (keyState[VK_SCROLL] & 1)))
    {
        // Simulate a key press
        keybd_event(VK_SCROLL,
            0x45,
            KEYEVENTF_EXTENDEDKEY | 0,
            0);

        // Simulate a key release
        keybd_event(VK_SCROLL,
            0x45,
            KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
            0);
    }
}



std::string GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

int altg_active() {
    return (GetKeyState(VK_RMENU));
}

LRESULT CALLBACK KeyboardProc(int code,       // hook code
    WPARAM wParam,  // virtual-key code
    LPARAM lParam)  // keystroke-message information
{
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
    static std::string str;

    if (!altg_active())
        return CallNextHookEx(hKeyboardHook, code, wParam, lParam);

    if (wParam == WM_KEYDOWN)
    {
        switch (p->vkCode)
        {
            case VK_ADD:
            {
                int cb = get_brightness();
                cb = ((cb + 10) < 0 || (cb + 10) > 100) ? 100 : cb + 10;
                set_brightness(cb);
                
            }
            break;
            case VK_SUBTRACT:
            {
                int cb = get_brightness();
                cb = ((cb - 10) < 0 || (cb - 10) > 100) ? 0 : cb - 10;
                set_brightness(cb);

            }
            break;
            default:
            break;
        }
    }

    return CallNextHookEx(hKeyboardHook, code, wParam, lParam);
}



BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // prepare for XP style controls
    InitCommonControls();


    // store instance handle and create dialog
    hInst = hInstance;
    HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_DIALOG), NULL, (DLGPROC)DlgProc);
    string err = GetLastErrorAsString();
    if (!hWnd)
        return FALSE;

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

    ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
    if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
        niData.cbSize = sizeof(NOTIFYICONDATA);
    else niData.cbSize = NOTIFYICONDATA_V2_SIZE;

    // the ID number can be anything you choose
    niData.uID = TRAYICONID;

    // state which structure members are valid
    niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

    // load the icon
    niData.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_STEALTHDLG),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);

    // the window to send messages to and the message to send
    //		note:	the message value should be in the
    //				range of WM_APP through 0xBFFF
    niData.hWnd = hWnd;
    niData.uCallbackMessage = SWM_TRAYMSG;

    // tooltip message
    lstrcpyn(niData.szTip, _T("Parlaklık"), sizeof(niData.szTip) / sizeof(TCHAR));

    Shell_NotifyIcon(NIM_ADD, &niData);

    // free icon handle
    if (niData.hIcon && DestroyIcon(niData.hIcon))
        niData.hIcon = NULL;

    // call ShowWindow here to make the dialog initially visible

    return TRUE;
}


// Name says it all
void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (hMenu)
    {
        //if (IsWindowVisible(hWnd))
        //    InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_HIDE, _T("Hide"));
        //else
        //    InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_SHOW, _T("Show"));
        InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));

        // note:	must set window to the foreground or the
        //			menu won't disappear when it should
        SetForegroundWindow(hWnd);

        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
            pt.x, pt.y, 0, hWnd, NULL);
        DestroyMenu(hMenu);
    }
}

// Get dll version number
ULONGLONG GetDllVersion(LPCTSTR lpszDllName)
{
    ULONGLONG ullVersion = 0;
    HINSTANCE hinstDll;
    hinstDll = LoadLibrary(lpszDllName);
    if (hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
        if (pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;
            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);
            hr = (*pDllGetVersion)(&dvi);
            if (SUCCEEDED(hr))
                ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion, 0, 0);
        }
        FreeLibrary(hinstDll);
    }
    return ullVersion;
}

// Message handler for the app
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;

    switch (message)
    {
    case WM_NCACTIVATE:
    {
        if (wParam == 0)
        {
            ShowWindow(hWnd, SW_HIDE);
        }
    }
    break;
    case WM_VSCROLL:
    {
        HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
        int p = 100 - SendMessage(sliderConLo, TBM_GETPOS, 0, 0);
        set_brightness(p);
    }
    break;
    case SWM_TRAYMSG:
        switch (lParam)
        {
            case WM_LBUTTONUP:
            {
                POINT pt;
                GetCursorPos(&pt);
                RECT r;
                GetWindowRect(hWnd, &r);
                SetWindowPos(hWnd, HWND_TOPMOST, pt.x - (r.right - r.left), pt.y - (r.bottom - r.top)-20, -1, -1, SWP_NOSIZE);
                HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
                SendMessage(sliderConLo, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(0, 100));
                SendMessage(sliderConLo, TBM_SETPOS, TRUE, 100 - get_brightness());
                DWORD isauto = 0;

                registry_key rk(HKEY_CURRENT_USER, "AutoBrightness", "isauto");
                
                HWND chk = GetDlgItem(hWnd, IDC_CHECK1);
                if (rk.readdword())
                    SendMessage(chk, BM_SETCHECK, BST_CHECKED, 0);
                else
                    SendMessage(chk, BM_SETCHECK, BST_UNCHECKED, 0);
                
                ShowWindow(hWnd, SW_RESTORE);
                SetFocus(sliderConLo);
            }
                break;
            case WM_RBUTTONDOWN:
            case WM_CONTEXTMENU:
                ShowContextMenu(hWnd);
        }
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE)
        {
            ShowWindow(hWnd, SW_HIDE);
            return 1;
        }
        break;
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId)
        {
        case SWM_SHOW:
            ShowWindow(hWnd, SW_RESTORE);
            break;
        case SWM_HIDE:
        case IDOK:
            ShowWindow(hWnd, SW_HIDE);
            break;
        case SWM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDC_CHECK1:
        {
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                registry_key rk(HKEY_CURRENT_USER, "AutoBrightness", "isauto");
                if (SendDlgItemMessage(hWnd, IDC_CHECK1, BM_GETCHECK, 0, 0))
                    rk.write(1);
                else
                    rk.write(0);
                break;
            }
        }
        break;
        }

        return 1;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        niData.uFlags = 0;
        if(workerthread != NULL )
            workerthread->detach();
        Shell_NotifyIcon(NIM_DELETE, &niData);
        PostQuitMessage(0);
        break;
    }
    return 0;
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    NOTIFYICONDATA niData;
    ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

    ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));

    if (ullVersion >= MAKEDLLVERULL(6, 0, 0, 0))
        niData.cbSize = sizeof(NOTIFYICONDATA);

    else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
        niData.cbSize = NOTIFYICONDATA_V2_SIZE;

    else niData.cbSize = NOTIFYICONDATA_V1_SIZE;


    MSG msg;
    HACCEL hAccelTable = NULL;

    pt.x = 5;
    pt.y = 5;
    hMonitor = MonitorFromPoint(pt, 0);

    BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &cPhysicalMonitors);
    if (bSuccess)
    {
        pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(cPhysicalMonitors * sizeof(PHYSICAL_MONITOR));
        bSuccess = GetPhysicalMonitorsFromHMONITOR(hMonitor, cPhysicalMonitors, pPhysicalMonitors);
        pmh = pPhysicalMonitors[0].hPhysicalMonitor;
    }

    registry_key rk(HKEY_CURRENT_USER, "AutoBrightness", "isauto");
    if (rk.dwDisposition == REG_CREATED_NEW_KEY)
        rk.write(0);
    else
        workerthread.reset(new thread(workerfunc));
    

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;
    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_STEALTHDIALOG);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) ||
            !IsDialogMessage(msg.hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

void set_brightness(int val)
{
    SetMonitorBrightness(pmh, val);
}
int get_brightness()
{
    DWORD minb, maxb, cb;
    GetMonitorBrightness(pmh, &minb, &cb, &maxb);
    return cb;
}

void workerfunc()
{
    SetNumLock(TRUE);

    int br_array[] = {
        0,0,0,0,0,
        /*5*/0,0,0,0,20,
        /*10*/35,40,50,60,60,
        /*15*/100,100,100,100,70,
        /*20*/0,0,0,0};

    while (true)
    {
        datetime curr_time = datetime::now();

        //SetMonitorBrightness(pmh, br_array[curr_time.hour()]);
        set_brightness(br_array[curr_time.hour()]);
        this_thread::sleep_for(chrono::seconds(60));
    }
}

