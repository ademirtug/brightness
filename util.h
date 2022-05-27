#pragma once

using namespace std;

void workerfunc();
BOOL				InitInstance(HINSTANCE, int);
void				ShowContextMenu(HWND hWnd);
ULONGLONG			GetDllVersion(LPCTSTR lpszDllName);
INT_PTR CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GlobalKeyHookProc(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
class safethread;

void set_brightness(int val);
int get_brightness();
std::unique_ptr<safethread> workerthread;
HHOOK hKeyboardHook = 0;
DWORD dw;
HMONITOR hMonitor = NULL;
DWORD cPhysicalMonitors;
LPPHYSICAL_MONITOR pPhysicalMonitors = NULL;
POINT pt;
HANDLE pmh;
HINSTANCE		hInst;	// current instance
NOTIFYICONDATA	niData;	// notify icon data
HWND hWnd;

#define SWM_EXIT	WM_APP + 3//	close the window
#define TRAYICONID	1//				ID number for the Notify Icon
#define SWM_TRAYMSG	WM_APP//		the message ID sent to our window


DWORD _isAuto = -1;

class safethread
{
public:
    shared_ptr<thread> th;

    safethread(std::function<void()> f)
    {
        th.reset(new thread(f));
    };
    ~safethread()
    {
        if (th != nullptr && th->joinable())
        {
            th->detach();
        }
    };
};

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

void SetScrollLock(BOOL bState)
{
    BYTE keyState[256];

    GetKeyboardState((LPBYTE)&keyState);
    if ((bState && !(keyState[VK_SCROLL] & 1)) ||
        (!bState && (keyState[VK_SCROLL] & 1)))
    {
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
        keybd_event(VK_SCROLL,  0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,0);
    }
}

std::string GetLastErrorAsString()
{
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::string();

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);

    return message;
}

int altg_active() {
    return (GetKeyState(VK_RCONTROL) & GetKeyState(VK_RSHIFT));
}
int fn_active() {
    return (GetKeyState(VK_RCONTROL) & GetKeyState(VK_RSHIFT));
}

DWORD isAuto() {
    return _isAuto;
}

void isAuto(DWORD val) {
    _isAuto = val;
    registry_key rk(HKEY_CURRENT_USER, "SOFTWARE\\AutoBrightness", "isauto");
    rk.write(_isAuto);    
}

void set_slider(int val)
{
    HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
    SendMessage(sliderConLo, TBM_SETPOS, true, val);
}
DWORD get_slider()
{
    HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
    return SendMessage(sliderConLo, TBM_GETPOS, 0, 0);
}
void set_brightness(int val)
{
    SetMonitorBrightness(pmh, val);
    HWND sliderConLo = GetDlgItem(hWnd, IDC_SLIDER1);
    SendMessage(sliderConLo, TBM_SETPOS, TRUE, val);
}
int get_brightness()
{
    DWORD minb, maxb, cb;
    GetMonitorBrightness(pmh, &minb, &cb, &maxb);
    return cb;
}

int getbrbyhour(int hour)
{
    int br_array[] = {
    0,0,0,0,0,
    /*5*/0,40,50,50,70,
    /*10*/70,100,100,100,100,
    /*15*/100,100,100,100,50,
    /*20*/20,0,0,0 };

    return br_array[hour];
}
void workerfunc()
{
    while (true)
    {
        if (isAuto()) 
        {
            datetime curr_time = datetime::now();
            set_brightness(getbrbyhour(curr_time.hour()));
            this_thread::sleep_for(chrono::seconds(60));
        }

    }
}