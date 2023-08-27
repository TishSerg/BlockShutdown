// BlockShutdown.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <shellapi.h>
#include "BlockShutdown.h"

#define MAX_LOADSTRING 100

#define HELP_TEXT L"Usage: BlockShutdown.exe [\"Reason string\" [timeout (sec)]]\n\nUse '%s' placeholder for timeout due time in the reason string else a default one will be appended."

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR szBlockReason[MAX_STR_BLOCKREASON];
long nTimeout;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    // Level 0x100 to block after majority of apps are already closed
    // Level 0x3FF (or even 0x4FF) to block before majority of apps are asked to close
    if (!SetProcessShutdownParameters(0x3FF, 0))
    {
        HANDLE hEventLog = OpenEventLogW(NULL, L"BlockShutdown");
        WCHAR msg[96];
        swprintf_s(msg, ARRAYSIZE(msg), L"SetProcessShutdownParameters failed. Error: 0x%X", GetLastError());
        LPCWSTR lpStrings[] = { msg };
        ReportEventW(hEventLog, EVENTLOG_WARNING_TYPE, 0, 0, NULL, ARRAYSIZE(lpStrings), 0, lpStrings, NULL);
        CloseEventLog(hEventLog);
    }

    if (lpCmdLine[0] != L'\0')
    {
        int argc;
        LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
        if (argc > 2)
        {
            MessageBoxW(NULL, HELP_TEXT, L"BlockShutdown", MB_ICONINFORMATION);
            return 1;
        }
        if (argc > 0)
        {
            if ((argv[0][0] == L'/' || argv[0][0] == L'-') && argv[0][1] == L'?')
            {
                MessageBoxW(NULL, HELP_TEXT, L"BlockShutdown", MB_ICONINFORMATION);
                return 0;
            }
            wcscpy_s(szBlockReason, ARRAYSIZE(szBlockReason), argv[0]);
        }
        if (argc > 1)
        {
            LPWCH lpEnd;
            long timeout = wcstol(argv[1], &lpEnd, 0);
            if (wcschr(L" \t\r\n", *lpEnd) != NULL)
            {
                nTimeout = timeout;
            }
        }
        LocalFree(argv);
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_BLOCKSHUTDOWN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BLOCKSHUTDOWN));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BLOCKSHUTDOWN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_BLOCKSHUTDOWN);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // WS_EX_TOOLWINDOW to hide taskbar button for the window
   HWND hWnd = CreateWindowExW(0 | WS_EX_TOOLWINDOW, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // To block shutdown either the window must be shown or ShutdownBlockReasonCreate must be called (or both)
   ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
   //UpdateWindow(hWnd);

   SetWindowPos(
       hWnd, HWND_BOTTOM, -200, -50, 0, 0,
       SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOSIZE);    // Hide minimized window (if is shown)

   if (!ShutdownBlockReasonCreate(hWnd, szBlockReason))
   {
       HANDLE hEventLog = OpenEventLogW(NULL, L"BlockShutdown");
       WCHAR msg[96];
       swprintf_s(msg, ARRAYSIZE(msg), L"ShutdownBlockReasonCreate failed. Error: 0x%X", GetLastError());
       LPCWSTR lpStrings[] = { msg };
       ReportEventW(hEventLog, EVENTLOG_WARNING_TYPE, 0, 0, NULL, ARRAYSIZE(lpStrings), 0, lpStrings, NULL);
       CloseEventLog(hEventLog);
   }

   NOTIFYICONDATAW nidata = { 0 };
   nidata.cbSize = sizeof(nidata);
   nidata.hWnd = hWnd;
   nidata.uID = IDI_BLOCKSHUTDOWN;
   nidata.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
   nidata.uCallbackMessage = WM_USER;
   nidata.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BLOCKSHUTDOWN));
   swprintf_s(nidata.szTip, ARRAYSIZE(nidata.szTip), L"BlockShutdown");
   Shell_NotifyIconW(NIM_ADD, &nidata);

   return TRUE;
}

DWORD WINAPI QueryEndSessionCallback(LPVOID lpParameter)
{
    LPARAM lParam = (LPARAM)lpParameter;

    HANDLE hEventLog = OpenEventLogW(NULL, L"BlockShutdown");

    if (lParam == 0)
    {
        WCHAR msg[] = L"WM_QUERYENDSESSION: System shutdown or restart attempt";
        LPCWSTR lpStrings[] = { msg };
        ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, ARRAYSIZE(lpStrings), 0, lpStrings, NULL);
    }
    else
    {
        WCHAR msg[] = L"WM_QUERYENDSESSION: User log off attempt";
        WCHAR msg1[] = L"WM_QUERYENDSESSION: Looks like System shutdown or restart enforced";
        WCHAR msg2[] = L"WM_QUERYENDSESSION: Application is forced to shut down";
        WCHAR msg3[] = L"WM_QUERYENDSESSION: Application must shut down";
        LPCWSTR lpStrings[4];
        WORD nStrings = 0;
        if (lParam & ENDSESSION_LOGOFF)
            lpStrings[nStrings++] = msg;
        if (lParam & ENDSESSION_CRITICAL)
        {
            lpStrings[nStrings++] = msg2;
            if (!(lParam & ENDSESSION_LOGOFF))
                lpStrings[nStrings++] = msg1;
        }
        if (lParam & ENDSESSION_CLOSEAPP)
            lpStrings[nStrings++] = msg3;
        ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, nStrings, 0, lpStrings, NULL);
    }

    CloseEventLog(hEventLog);

    return 0;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static BOOL sbIsExiting = FALSE;

    switch (message)
    {
    case WM_QUERYENDSESSION:
    {
        CreateThread(NULL, 0, QueryEndSessionCallback, (LPVOID)lParam, 0, NULL);

        if (nTimeout != 0)
        {
            SYSTEMTIME curTime;
            GetSystemTime(&curTime);
            FILETIME curTimeFiletime;
            SystemTimeToFileTime(&curTime, &curTimeFiletime);
            ULARGE_INTEGER curTimeI64;
            curTimeI64.LowPart = curTimeFiletime.dwLowDateTime;
            curTimeI64.HighPart = curTimeFiletime.dwHighDateTime;
            curTimeI64.QuadPart += nTimeout * 1000 * 1000 * 10; // Unit: 100 nsec
            curTimeFiletime.dwLowDateTime = curTimeI64.LowPart;
            curTimeFiletime.dwHighDateTime = curTimeI64.HighPart;
            FileTimeToSystemTime(&curTimeFiletime, &curTime);
            DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
            GetDynamicTimeZoneInformation(&tzinfo);
            SYSTEMTIME localTime;
            SystemTimeToTzSpecificLocalTimeEx(&tzinfo, &curTime, &localTime);

            WCHAR time[32];
            GetTimeFormatW(LOCALE_INVARIANT, 0 | TIME_NOTIMEMARKER /*| TIME_FORCE24HOURFORMAT*/, &localTime, NULL, time, ARRAYSIZE(time));

            WCHAR msg[MAX_STR_BLOCKREASON];
            if (wcsstr(szBlockReason, L"%s") != NULL)
            {
                swprintf_s(msg, ARRAYSIZE(msg), szBlockReason, time);
            }
            else
            {
                swprintf_s(msg, ARRAYSIZE(msg), L"%s (unblock at %s)", szBlockReason, time);
            }

            if (!ShutdownBlockReasonCreate(hWnd, msg))
            {
                HANDLE hEventLog = OpenEventLogW(NULL, L"BlockShutdown");
                WCHAR msg[96];
                swprintf_s(msg, ARRAYSIZE(msg), L"ShutdownBlockReasonCreate failed. Error: 0x%X", GetLastError());
                LPCWSTR lpStrings[] = { msg };
                ReportEventW(hEventLog, EVENTLOG_WARNING_TYPE, 0, 0, NULL, ARRAYSIZE(lpStrings), 0, lpStrings, NULL);
                CloseEventLog(hEventLog);
            }

            SetTimer(hWnd, Timer_Exit, nTimeout * 1000, NULL);
        }

        return FALSE;   // Block shutdown
    }
    break;
    case WM_ENDSESSION:
        {
            if (!wParam)
            {
                KillTimer(hWnd, Timer_Exit);    // User canceled shutdown - don't exit
            }

            HANDLE hEventLog = OpenEventLogW(NULL, L"BlockShutdown");
            if (hEventLog == NULL)
            {
                MessageBeep(MB_ICONERROR);
                return 1;
            }

            if (lParam == 0)
            {
                if (wParam)
                {
                    WCHAR msg[] = L"WM_ENDSESSION: System shutdown or restart enforced";
                    LPCWSTR lpStrings[] = { msg };
                    ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, ARRAYSIZE(lpStrings), 0, lpStrings, NULL);
                }
                else
                {
                    WCHAR msg[] = L"WM_ENDSESSION: System shutdown or restart canceled";
                    LPCWSTR lpStrings[] = { msg };
                    ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, ARRAYSIZE(lpStrings), 0, lpStrings, NULL);
                }
            }
            else
            {
                if (wParam)
                {
                    WCHAR msg[] = L"WM_ENDSESSION: User log off enforced";
                    WCHAR msg1[] = L"WM_ENDSESSION: Looks like System shutdown or restart enforced";
                    WCHAR msg2[] = L"WM_ENDSESSION: Application is forced to shut down";
                    WCHAR msg3[] = L"WM_ENDSESSION: Application must shut down";
                    LPCWSTR lpStrings[4];
                    WORD nStrings = 0;
                    if (lParam & ENDSESSION_LOGOFF)
                        lpStrings[nStrings++] = msg;
                    if (lParam & ENDSESSION_CRITICAL)
                    {
                        lpStrings[nStrings++] = msg2;
                        if (!(lParam & ENDSESSION_LOGOFF))
                            lpStrings[nStrings++] = msg1;
                    }
                    if (lParam & ENDSESSION_CLOSEAPP)
                        lpStrings[nStrings++] = msg3;
                    ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, nStrings, 0, lpStrings, NULL);
                }
                else
                {
                    WCHAR msg[] = L"WM_ENDSESSION: User log off canceled";
                    WCHAR msg2[] = L"WM_ENDSESSION: Application still forced to shut down";
                    WCHAR msg3[] = L"WM_ENDSESSION: Application should not shut down";
                    LPCWSTR lpStrings[4];
                    WORD nStrings = 0;
                    if (lParam & ENDSESSION_LOGOFF)
                        lpStrings[nStrings++] = msg;
                    if (lParam & ENDSESSION_CRITICAL)
                        lpStrings[nStrings++] = msg2;
                    if (lParam & ENDSESSION_CLOSEAPP)
                        lpStrings[nStrings++] = msg3;
                    ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, nStrings, 0, lpStrings, NULL);
                }
            }
            CloseEventLog(hEventLog);
            return 0;
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
                case Timer_SingleClick:
                {
                    KillTimer(hWnd, Timer_SingleClick); // Will show notification. Disable timer to avoid auto-repeat.
                    NOTIFYICONDATAW nidata = { 0 };
                    nidata.cbSize = sizeof(nidata);
                    nidata.hWnd = hWnd;
                    nidata.uID = IDI_BLOCKSHUTDOWN;
                    nidata.uFlags = NIF_INFO;
                    nidata.dwInfoFlags = NIIF_INFO;
                    swprintf_s(nidata.szInfoTitle, ARRAYSIZE(nidata.szInfoTitle), L"Double click to stop BlockShutdown");
                    if (wcslen(szBlockReason) > 0) 
                        swprintf_s(nidata.szInfo, ARRAYSIZE(nidata.szInfo), szBlockReason);
                    else
                        swprintf_s(nidata.szInfo, ARRAYSIZE(nidata.szInfo), L"BlockShutdown is preventing shutdown.");
                    Shell_NotifyIconW(NIM_MODIFY, &nidata);
                }
                break;
                case Timer_Exit:
                {
                    NOTIFYICONDATA nidata = { 0 };
                    nidata.cbSize = sizeof(nidata);
                    nidata.hWnd = hWnd;
                    nidata.uID = IDI_BLOCKSHUTDOWN;
                    Shell_NotifyIcon(NIM_DELETE, &nidata);
                    PostQuitMessage(0);
                }
                break;
            default:
                break;
            }
        }
        break;
    case WM_USER:
        {
            UINT notifyIconID = (UINT)wParam;
            UINT notifyMsg = (UINT)lParam;
            if (notifyMsg == WM_LBUTTONDOWN)
            {
                KillTimer(hWnd, Timer_SingleClick); // Looks like it's DblClick - no need to show notification
            }
            else if (notifyMsg == WM_LBUTTONUP)
            {
                if (!sbIsExiting) 
                    SetTimer(hWnd, Timer_SingleClick, GetDoubleClickTime(), NULL);  // To show notification if will be not DblClick
            }
            else if (notifyMsg == WM_LBUTTONDBLCLK)
            {
                sbIsExiting = TRUE;
                KillTimer(hWnd, Timer_SingleClick);
                SetTimer(hWnd, Timer_Exit, 5000, NULL);
                NOTIFYICONDATAW nidata = { 0 };
                nidata.cbSize = sizeof(nidata);
                nidata.hWnd = hWnd;
                nidata.uID = IDI_BLOCKSHUTDOWN;
                nidata.uFlags = NIF_INFO;
                nidata.dwInfoFlags = NIIF_INFO;
                swprintf_s(nidata.szInfoTitle, ARRAYSIZE(nidata.szInfoTitle), L"BlockShutdown is exiting...");
                swprintf_s(nidata.szInfo, ARRAYSIZE(nidata.szInfo), L"Shutdown won't be blocked anymore");
                Shell_NotifyIconW(NIM_MODIFY, &nidata);
            }
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
