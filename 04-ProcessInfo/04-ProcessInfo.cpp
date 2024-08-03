// 04-ProcessInfo.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "04-ProcessInfo.h"
#include "Toolhelp.h"
#include <shlobj.h>  
#include <StrSafe.h> 
#include <Windowsx.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <shlwapi.h>    // for StrFormatKBSize.

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// static variables
TOKEN_ELEVATION_TYPE s_elevationType = TokenElevationTypeDefault;
BOOL                 s_bIsAdmin = FALSE;
const int			 s_cchAddress = sizeof(PVOID) * 2;


static const int ID_COMBOBOX = 3;
static const int IDC_RESULT = 4;

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
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Enabling the debug privilege allows the application to see information about service applications
    CToolhelp::EnablePrivilege(SE_DEBUG_NAME, true);
    CToolhelp::EnablePrivilege(SE_SECURITY_NAME, true);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY04PROCESSINFO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY04PROCESSINFO));

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

    CToolhelp::EnablePrivilege(SE_DEBUG_NAME, false);
    CToolhelp::EnablePrivilege(SE_SECURITY_NAME, false);
     

    return (int) msg.wParam;
}

static void populateProcessList(HWND hwnd) {
    CToolhelp thProcesses(TH32CS_SNAPPROCESS);
    PROCESSENTRY32 pe = { sizeof(pe) };
    BOOL fOk = thProcesses.ProcessFirst(&pe);

    auto hWndComboBox = GetDlgItem(hwnd, ID_COMBOBOX);
    for (; fOk; fOk = thProcesses.ProcessNext(&pe)) {
        TCHAR sz[1024];

        // place the process name & ID
        PCTSTR pszExeFile = _tcsrchr(pe.szExeFile, TEXT('\\'));
        if (pszExeFile == NULL) {
            pszExeFile = pe.szExeFile;
        }
        else {
            pszExeFile++;
        }

        StringCchPrintf(sz, _countof(sz), TEXT("%s     (0x%08X)"),
            pszExeFile, pe.th32ProcessID);
        auto dwIndex = SendMessage(hWndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)sz);

        SendMessage(hWndComboBox, CB_SETITEMDATA, dwIndex, (LPARAM)pe.th32ProcessID);

    }

    SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);


    InvalidateRect(hWndComboBox, NULL, FALSE);

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY04PROCESSINFO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY04PROCESSINFO);
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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

static bool GetProcessElevation(TOKEN_ELEVATION_TYPE* pElevationType, BOOL* pIsAdmin) {
    HANDLE hToken = NULL;
    DWORD dwSize;

    // Get current process token
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    BOOL bResult = FALSE;

    // Retrieve elevation type infomation
    if (GetTokenInformation(hToken, TokenElevationType, pElevationType, sizeof(TOKEN_ELEVATION_TYPE), &dwSize)) {

        char adminSID[SECURITY_MAX_SID_SIZE];
        dwSize = sizeof(adminSID);
        CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSID, &dwSize);

        if (*pElevationType == TokenElevationTypeLimited) {
            // Get handle to linked token (will have one if we are lua
            HANDLE hUnfilteredToken = NULL;
            GetTokenInformation(hToken, TokenLinkedToken, (VOID*)&hUnfilteredToken, sizeof(HANDLE), &dwSize);

            // Check if this original token contains admin SID
            if (CheckTokenMembership(hUnfilteredToken, &adminSID, pIsAdmin)) {
                bResult = TRUE;
            }

            // Don't forget to close the unfiltered token
            CloseHandle(hUnfilteredToken);
        }
        else {
            *pIsAdmin = IsUserAnAdmin();
            bResult = TRUE;
        }
    }

    // Don't forget to close the process token
    CloseHandle(hToken);

    return bResult;
}

static bool initWindow(HWND hwnd) {
    BOOL bCanReadSystemProcesses = FALSE;

    // Show if we are running with filtered token or not
    if (GetProcessElevation(&s_elevationType, &s_bIsAdmin)) {
        // prefix title with elevation
        TCHAR szTitle[64];

        switch (s_elevationType) {
            // Default user or UAC is disabled
        case TokenElevationTypeDefault:
            if (IsUserAnAdmin()) {
                _tcscpy_s(szTitle, _countof(szTitle),
                    TEXT("Default Administrator: "));
                bCanReadSystemProcesses = true;
            }
            else {
                _tcscpy_s(szTitle, _countof(szTitle),
                    TEXT("Default: "));
            }
            break;

            // Process has been successfully elevated
        case TokenElevationTypeFull:
            if (IsUserAnAdmin()) {
                _tcscpy_s(szTitle, _countof(szTitle),
                    TEXT("Elevated Administrator: "));
                bCanReadSystemProcesses = true;
            }
            else {
                _tcscpy_s(szTitle, _countof(szTitle),
                    TEXT("Elevated: "));
            }
            break;

            // Process is running with limited privileges
        case TokenElevationTypeLimited:
            if (s_bIsAdmin) {
                _tcscpy_s(szTitle, _countof(szTitle),
                    TEXT("Filtered Administrator: "));
            }
            else {
                _tcscpy_s(szTitle, _countof(szTitle),
                    TEXT("Filtered: "));
            }
            break;
        }

        // Update the dialog title based on the elevation level
        GetWindowText(hwnd, _tcschr(szTitle, TEXT('\0')),
            _countof(szTitle) - _tcslen(szTitle));
        SetWindowText(hwnd, szTitle);

        
        //
        CreateWindow(WC_COMBOBOX, TEXT(""), CBS_DROPDOWN | CBS_HASSTRINGS | CBS_AUTOHSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL, 100, 30, 600, 250, hwnd, (HMENU)ID_COMBOBOX, hInst, NULL);

        CreateWindow(WC_EDIT, TEXT(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL , 100, 80, 600, 300, hwnd, (HMENU)IDC_RESULT, hInst, NULL);

    }

    populateProcessList(hwnd);
    return true;

}

// Add a string to an edit control
void AddText(HWND hwnd, PCTSTR pszFormat, ...) {

    va_list argList;
    va_start(argList, pszFormat);

    TCHAR sz[20 * 1024];
    Edit_GetText(hwnd, sz, _countof(sz));
    _vstprintf_s(_tcschr(sz, TEXT('\0')), _countof(sz) - _tcslen(sz),
        pszFormat, argList);
    Edit_SetText(hwnd, sz);
    va_end(argList);
}


BOOL GetProcessOwner(HANDLE hProcess, LPTSTR szOwner, size_t cchSize) {

    // Sanity checks
    if ((szOwner == NULL) || (cchSize == 0))
        return(FALSE);

    // Default value
    szOwner[0] = TEXT('\0');

    // Gget process token
    HANDLE hToken = NULL;
    CToolhelp::EnablePrivilege(SE_TCB_NAME, TRUE);
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        CToolhelp::EnablePrivilege(SE_TCB_NAME, FALSE);
        return(FALSE);
    }

    // Obtain the size of the user information in the token.
    DWORD cbti = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti);

    // Call should have failed due to zero-length buffer.
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate buffer for user information in the token.
        PTOKEN_USER ptiUser =
            (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, cbti);
        if (ptiUser != NULL) {
            // Retrieve the user information from the token.
            if (GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti)) {
                SID_NAME_USE   snu;
                TCHAR          szUser[MAX_PATH];
                DWORD          chUser = MAX_PATH;
                PDWORD         pcchUser = &chUser;
                TCHAR          szDomain[MAX_PATH];
                DWORD          chDomain = MAX_PATH;
                PDWORD         pcchDomain = &chDomain;

                // Retrieve user name and domain name based on user's SID.
                if (
                    LookupAccountSid(
                        NULL,
                        ptiUser->User.Sid,
                        szUser,
                        pcchUser,
                        szDomain,
                        pcchDomain,
                        &snu
                    )
                    ) {
                    // build the owner string as \\DomainName\UserName
                    _tcscpy_s(szOwner, cchSize, TEXT("\\\\"));
                    _tcscat_s(szOwner, cchSize, szDomain);
                    _tcscat_s(szOwner, cchSize, TEXT("\\"));
                    _tcscat_s(szOwner, cchSize, szUser);
                }
            }

            // Don't forget to free memory buffer
            HeapFree(GetProcessHeap(), 0, ptiUser);
        }
    }

    // Don't forget to free process token
    CloseHandle(hToken);

    // Restore privileges
    CToolhelp::EnablePrivilege(SE_TCB_NAME, TRUE);

    return(TRUE);
}


BOOL GetProcessOwner(DWORD PID, LPTSTR szOwner, DWORD cchSize) {

    // Sanity checks
    if ((PID <= 0) || (szOwner == NULL))
        return(FALSE);

    // Check if we can get information for this process
    HANDLE hProcess =
        OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PID);
    if (hProcess == NULL)
        return(FALSE);

    BOOL bReturn = GetProcessOwner(hProcess, szOwner, cchSize);

    // Don't forget to release the process handle
    CloseHandle(hProcess);

    return(bReturn);
}

PVOID GetModulePreferredBaseAddr(DWORD dwProcessId, PVOID pvModuleRemote) {

    PVOID pvModulePreferredBaseAddr = NULL;
    IMAGE_DOS_HEADER idh;
    IMAGE_NT_HEADERS inth;

    // Read the remote module's DOS header
    Toolhelp32ReadProcessMemory(dwProcessId,
        pvModuleRemote, &idh, sizeof(idh), NULL);

    // Verify the DOS image header
    if (idh.e_magic == IMAGE_DOS_SIGNATURE) {
        // Read the remote module's NT header
        Toolhelp32ReadProcessMemory(dwProcessId,
            (PBYTE)pvModuleRemote + idh.e_lfanew, &inth, sizeof(inth), NULL);

        // Verify the NT image header
        if (inth.Signature == IMAGE_NT_SIGNATURE) {
            // This is valid NT header, get the image's preferred base address
            pvModulePreferredBaseAddr = (PVOID)inth.OptionalHeader.ImageBase;
        }
    }
    return(pvModulePreferredBaseAddr);
}


 VOID ShowProcessInfo(HWND hwnd, DWORD dwProcessID) {

    SetWindowText(hwnd, TEXT(""));   // Clear the output box

    CToolhelp th(TH32CS_SNAPALL, dwProcessID);

    // can only inspect x86 process if this is build in x86 arch.  if need to inpsect x64 we need to build in x64 arch.
    auto errId = GetLastError();
    if (errId != ERROR_SUCCESS) {
        HLOCAL hlocal = NULL;

        // Get the error code's texutal description
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, errId, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&hlocal, 0, NULL);

        // show error description
        if (hlocal != NULL) {
            MessageBoxA(hwnd, (PCSTR)LocalLock(hlocal), ("error"), MB_OK);
            LocalFree(hlocal);
        }
    }

    // Show Process details
    PROCESSENTRY32 pe = { sizeof(pe) };
    BOOL fOk = th.ProcessFirst(&pe);
    for (; fOk; fOk = th.ProcessNext(&pe)) {
        if (pe.th32ProcessID == dwProcessID) {
            TCHAR szCmdLine[1024];
                          AddText(hwnd, TEXT("Filename: %s\r\n"), pe.szExeFile);
            AddText(hwnd, TEXT("   PID=%08X, ParentPID=%08X, ")
                TEXT("PriorityClass=%d, Threads=%d, Heaps=%d\r\n"),
                pe.th32ProcessID, pe.th32ParentProcessID,
                pe.pcPriClassBase, pe.cntThreads,
                th.HowManyHeaps());
            TCHAR szOwner[MAX_PATH + 1];
            if (GetProcessOwner(dwProcessID, szOwner, MAX_PATH)) {
                AddText(hwnd, TEXT("Owner: %s\r\n"), szOwner);
            }

            break;   // No need to continue looping
        }
    }


    // Show Modules in the Process
   // Number of characters to display an address
    AddText(hwnd, TEXT("\r\nModules Information:\r\n")
        TEXT("  Usage  %-*s(%-*s)  %10s  Module\r\n"),
        s_cchAddress, TEXT("BaseAddr"),
        s_cchAddress, TEXT("ImagAddr"), TEXT("Size"));

    MODULEENTRY32 me = { sizeof(me) };
    fOk = th.ModuleFirst(&me);
    for (; fOk; fOk = th.ModuleNext(&me)) {
        if (me.ProccntUsage == 65535) {
            // Module was implicitly loaded and cannot be unloaded
            AddText(hwnd, TEXT("  Fixed"));
        }
        else {
            AddText(hwnd, TEXT("  %5d"), me.ProccntUsage);
        }

        // Try to format the size in kb.
        TCHAR szFormattedSize[64];
        if (StrFormatKBSize(me.modBaseSize, szFormattedSize,
            _countof(szFormattedSize)) == NULL)
        {
            StringCchPrintf(szFormattedSize, _countof(szFormattedSize),
                TEXT("%10u"), me.modBaseSize);
        }

        PVOID pvPreferredBaseAddr =
            GetModulePreferredBaseAddr(pe.th32ProcessID, me.modBaseAddr);
        if (me.modBaseAddr == pvPreferredBaseAddr) {
            AddText(hwnd, TEXT("  %p %*s   %10s  %s\r\n"),
                me.modBaseAddr, s_cchAddress, TEXT(""),
                szFormattedSize, me.szExePath);
        }
        else {
            AddText(hwnd, TEXT("  %p(%p)  %10s  %s\r\n"),
                me.modBaseAddr, pvPreferredBaseAddr,
                szFormattedSize, me.szExePath);
        }
    }
    

    // Show threads in the process
    AddText(hwnd, TEXT("\r\nThread Information:\r\n")
        TEXT("      TID     Priority\r\n"));
    THREADENTRY32 te = { sizeof(te) };
    fOk = th.ThreadFirst(&te);
    for (; fOk; fOk = th.ThreadNext(&te)) {
        if (te.th32OwnerProcessID == dwProcessID) {
            int nPriority = te.tpBasePri + te.tpDeltaPri;
            if ((te.tpBasePri < 16) && (nPriority > 15)) nPriority = 15;
            if ((te.tpBasePri > 15) && (nPriority > 31)) nPriority = 31;
            if ((te.tpBasePri < 16) && (nPriority < 1)) nPriority = 1;
            if ((te.tpBasePri > 15) && (nPriority < 16)) nPriority = 16;
            AddText(hwnd, TEXT("   %08X       %2d\r\n"),
                te.th32ThreadID, nPriority);
        }
    }

}



void handleComboSelect(HWND hWnd) {
    auto hWndComboBox = GetDlgItem(hWnd, ID_COMBOBOX);
    int selectedIndex = SendMessageW(hWndComboBox, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR)
    {
		DWORD pid = SendMessageW(hWndComboBox, CB_GETITEMDATA, selectedIndex, 0);
        

		HWND hwndEditBox = GetDlgItem(hWnd, IDC_RESULT);
		ShowProcessInfo(hwndEditBox, pid);
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        initWindow(hWnd);
        break;
    case WM_DISPLAYCHANGE:
    {
        InvalidateRect(hWnd, NULL, FALSE);
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            
            case ID_COMBOBOX:
                if (HIWORD(wParam) == CBN_SELCHANGE) {
                    // 
                    //HWND hWndComboBox = (HWND)lParam;
                    handleComboSelect(hWnd);
                }
                break;
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
