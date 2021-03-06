#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <highlevelmonitorconfigurationapi.h>
#include <thread>
#include <memory>
#include "datetime.h"
#include <commctrl.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include <memory>
#include <functional>
#include <memory>
#include <iostream>
#include <algorithm>

#include "resource.h"
#include "registry.h"
#include "util.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")


//#define DBOUT( s )            \
//{                             \
//   std::ostringstream os_;    \
//   os_ << s;                   \
//   OutputDebugString( os_.str().c_str() );  \
//}

#define DBOUT( s )            \
{                             \
   std::wostringstream os_;    \
   os_ << s;                   \
   OutputDebugStringW( os_.str().c_str() );  \
}
//int AW_ACTIVATE = 0x00020000;
//int AW_BLEND = 0X80000;
//int AW_CENTER = 0x00000010;
//const int AW_HIDE = 0x00010000;
//const int AW_HOR_POSITIVE = 0x00000001;
//const int AW_HOR_NEGATIVE = 0x00000002;
//const int AW_SLIDE = 0X40000;
//const int AW_VER_POSITIVE = 0x00000004;
//const int AW_VER_NEGATIVE = 0x00000008;


void AnimateDown(HWND hWnd)
{
    bool ok = AnimateWindow(hWnd, 150, AW_SLIDE | AW_HIDE | AW_VER_POSITIVE);
    ShowWindow(hWnd, SW_HIDE);
}
void AnimateUp(HWND hWnd)
{
    bool ok = AnimateWindow(hWnd, 150, AW_SLIDE | AW_VER_NEGATIVE);
    SendMessage(hWnd, WM_SETFOCUS, 0, 0);
    
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    InitCommonControls();
    hInst = hInstance;
    hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_DIALOG), NULL, (DLGPROC)DlgProc);
    if (!hWnd)
        return FALSE;

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, GlobalKeyHookProc, hInstance, 0);
    
    ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
    ULONGLONG ullVersion = GetDllVersion(L"Shell32.dll");
    if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
        niData.cbSize = sizeof(NOTIFYICONDATA);
    else niData.cbSize = NOTIFYICONDATA_V2_SIZE;

    
    niData.uID = TRAYICONID;
    niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    niData.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_SYSICO),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);

    niData.hWnd = hWnd;
    niData.uCallbackMessage = SWM_TRAYMSG;
    lstrcpyn(niData.szTip, L"Parlaklık", sizeof(niData.szTip) / sizeof(TCHAR));
    Shell_NotifyIcon(NIM_ADD, &niData);

    if (niData.hIcon && DestroyIcon(niData.hIcon))
        niData.hIcon = NULL;

    SetScrollLock(TRUE);

    return TRUE;
}

void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (hMenu)
    {
        InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT,L"Exit");
        SetForegroundWindow(hWnd);

        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
        DestroyMenu(hMenu);
    }
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    NOTIFYICONDATA niData;
    ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

    ULONGLONG ullVersion = GetDllVersion(L"Shell32.dll");

    if (ullVersion >= MAKEDLLVERULL(6, 0, 0, 0))
        niData.cbSize = sizeof(NOTIFYICONDATA);
    else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
        niData.cbSize = NOTIFYICONDATA_V2_SIZE;
    else 
        niData.cbSize = NOTIFYICONDATA_V1_SIZE;

    MSG msg;
    HACCEL hAccelTable = NULL;
    hMonitor = MonitorFromPoint({5,5}, 0);

    BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &cPhysicalMonitors);

    if (bSuccess)
    {
        pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(cPhysicalMonitors * sizeof(PHYSICAL_MONITOR));
        bSuccess = GetPhysicalMonitorsFromHMONITOR(hMonitor, cPhysicalMonitors, pPhysicalMonitors);
        pmh = pPhysicalMonitors[0].hPhysicalMonitor;

    }

    registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");
    if (rk.dwDisposition == REG_CREATED_NEW_KEY)
        rk.write(0);
    
    isAuto(rk.readdword());
    workerthread.reset(new safethread(workerfunc));
    
    if (!InitInstance(hInstance, nCmdShow)) 
        return FALSE;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) || !IsDialogMessage(msg.hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_NCACTIVATE:
        {
            if (IsWindowVisible(hWnd))
            {
                RECT hwndrect;

                POINT mouse;
                GetCursorPos(&mouse);
                GetWindowRect(hWnd, &hwndrect);
                if (!PtInRect(&hwndrect, pt))
                    AnimateDown(hWnd);

                //return TRUE;
            }

        }
        break;
        case WM_MOUSEWHEEL:
        {
            set_slider(get_brightness() + (GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? -5 : 5));
        }
        break;
        case WM_HSCROLL:
        {
            isAuto(0);
            
            HWND chk = GetDlgItem(hWnd, IDC_CHECK1);
            SendMessage(chk, BM_SETCHECK, BST_UNCHECKED, 0);

            set_brightness(get_slider());
        }
        break;
        case SWM_TRAYMSG:
        {
            switch (lParam)
            {
                case WM_LBUTTONUP:
                {
                    POINT pt;
                    GetCursorPos(&pt);
                    RECT r;
                    GetWindowRect(hWnd, &r);


                    MONITORINFO mi;
                    mi.cbSize = sizeof(MONITORINFO);
                    GetMonitorInfo(hMonitor, &mi);          

                    RECT desktopRect;
                    if (!GetWindowRect(GetDesktopWindow(), &desktopRect))
                        return FALSE;

                    int windowWidth = (r.right - r.left);
                    int windowHeight = (r.bottom - r.top);
                    int posX = mi.rcWork.right - windowWidth;
                    int posY = mi.rcWork.bottom - windowHeight;

                    SetWindowPos(hWnd, HWND_TOPMOST, posX, posY, -1, -1, SWP_NOSIZE);
                    AnimateUp(hWnd);

                    set_slider(get_brightness());

                    HWND chk = GetDlgItem(hWnd, IDC_CHECK1);
                    SendMessage(chk, BM_SETCHECK, isAuto() ? BST_CHECKED : BST_UNCHECKED, 0);
                }
                break;
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    ShowContextMenu(hWnd);
            }
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_CHECK1:
                {
                    switch (HIWORD(wParam))
                    {
                    case BN_CLICKED:
                        isAuto(SendDlgItemMessage(hWnd, IDC_CHECK1, BM_GETCHECK, 0, 0));
                        break;
                    }
                }
                break;
                case SWM_EXIT:
                {
                    exit(0);
                }
            }  
        }
        return 1;
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
        {
            niData.uFlags = 0;
            workerthread.reset();

            Shell_NotifyIcon(NIM_DELETE, &niData);
            PostQuitMessage(0);
        }
        break;
        case WM_POWERBROADCAST:
        {
            if (wParam == PBT_APMRESUMEAUTOMATIC)
            {                
            }
        }
    }
    return 0;
}


LRESULT CALLBACK GlobalKeyHookProc(int code, WPARAM wParam, LPARAM lParam)
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
            isAuto(0);
            int cb = get_brightness();
            cb = ((cb + 10) < 0 || (cb + 10) > 100) ? 100 : cb + 10;
            set_brightness(cb);
        }
        break;
        case VK_SUBTRACT:
        {
            isAuto(0);
            int cb = get_brightness();
            cb = ((cb - 10) < 0 || (cb - 10) > 100) ? 0 : cb - 10;
            set_brightness(cb);
        }
        break;
        }
    }

    return CallNextHookEx(hKeyboardHook, code, wParam, lParam);
}