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
    
}
void AnimateUp(HWND hWnd)
{
    bool ok = AnimateWindow(hWnd, 150, AW_SLIDE | AW_VER_NEGATIVE);

}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    InitCommonControls();
    hInst = hInstance;
    HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_DIALOG), NULL, (DLGPROC)DlgProc);
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
    niData.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_STEALTHDLG),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);
    niData.hWnd = hWnd;
    niData.uCallbackMessage = SWM_TRAYMSG;
    lstrcpyn(niData.szTip, L"Parlaklık", sizeof(niData.szTip) / sizeof(TCHAR));
    Shell_NotifyIcon(NIM_ADD, &niData);

    if (niData.hIcon && DestroyIcon(niData.hIcon))
        niData.hIcon = NULL;

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
    
    if(rk.readdword())
        workerthread.reset(new safethread(workerfunc));
    

    registry_key rk2(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run\\", "AutoBrightness");
    if (rk.dwDisposition == REG_CREATED_NEW_KEY)
    {
        char szFileName[MAX_PATH];
        GetModuleFileNameA(NULL, szFileName, MAX_PATH);
        rk2.write(szFileName);
    }   

    DBOUT(L"winmain")


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

INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        //case WM_CTLCOLORDLG:
        //    return (INT_PTR)GetStockObject(BLACK_BRUSH);
        case WM_NCACTIVATE:
        {
            if (wParam == 0)
            {
                ShowWindow(hWnd, SW_HIDE);
            }
        }
        break;
        case WM_ACTIVATE:
        {
            if (wParam == WA_ACTIVE)
            {
            }
            else if (wParam == WA_INACTIVE)
            {
                AnimateDown(hWnd);
            }
        }
        break;
        case WM_HSCROLL:
        {
            workerthread.reset();

            registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");
            rk.write(0);
            HWND chk = GetDlgItem(hWnd, IDC_CHECK1);
            SendMessage(chk, BM_SETCHECK, BST_UNCHECKED, 0);

            HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
            int p = SendMessage(sliderConLo, TBM_GETPOS, 0, 0);
            set_brightness(p);
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
                    
                    HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
                    SendMessage(sliderConLo, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(0, 100));
                    SendMessage(sliderConLo, TBM_SETPOS, TRUE, get_brightness());
                    DWORD isauto = 0;

                    registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");

                    HWND chk = GetDlgItem(hWnd, IDC_CHECK1);
                    if (rk.readdword())
                        SendMessage(chk, BM_SETCHECK, BST_CHECKED, 0);
                    else
                        SendMessage(chk, BM_SETCHECK, BST_UNCHECKED, 0);

                    AnimateUp(hWnd);
                    //ShowWindow(hWnd, SW_RESTORE);
                }
                break;
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    ShowContextMenu(hWnd);
            }
            break;
        }
        case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE)
        {
            ShowWindow(hWnd, SW_HIDE);
            return 1;
        }
            break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case SWM_SHOW:
                ShowWindow(hWnd, SW_RESTORE);
                break;
            case SWM_HIDE:
            case SWM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDC_CHECK1:
            {
                switch (HIWORD(wParam))
                {
                case BN_CLICKED:
                    registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");
                    if (SendDlgItemMessage(hWnd, IDC_CHECK1, BM_GETCHECK, 0, 0))
                    {
                        datetime curr_time = datetime::now();
                        HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
                        SendMessage(sliderConLo, TBM_SETPOS, TRUE, getbrbyhour(curr_time.hour()));

                        workerthread.reset(new safethread(workerfunc));
                        rk.write(1);
                    }
                    else
                    {
                        workerthread.reset();
                        rk.write(0);
                    }
                        
                    break;
                }
            }
            break;
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
                registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");
                if( rk.readdword() )
                    workerthread.reset(new safethread(workerfunc));
                
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
            registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");
            rk.write(0);
            int cb = get_brightness();
            cb = ((cb + 10) < 0 || (cb + 10) > 100) ? 100 : cb + 10;
            set_brightness(cb);
        }
        break;
        case VK_SUBTRACT:
        {
            registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");
            rk.write(0);
            int cb = get_brightness();
            cb = ((cb - 10) < 0 || (cb - 10) > 100) ? 0 : cb - 10;
            set_brightness(cb);
        }
        break;
        }
    }

    return CallNextHookEx(hKeyboardHook, code, wParam, lParam);
}