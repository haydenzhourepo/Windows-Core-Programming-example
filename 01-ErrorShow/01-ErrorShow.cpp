// 01-ErrorShow.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "01-ErrorShow.h"
#include <string>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

//
static const int STATIC_ERROR_TEXT = 1;
static const int ID_ET_ERROR = 2;
static const int ID_LOOKUP_BTN = 3;
static const int ID_RESULT_TEXT = 4;

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

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY01ERRORSHOW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY01ERRORSHOW));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY01ERRORSHOW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY01ERRORSHOW);
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
      CW_USEDEFAULT, 0, 900, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void testAnWindowsError(HWND hWnd) {
    // in watch window add "@err" can look the error code
    // in watch window add "@err,hr" can look the error description
    // 100 is not a valid child window ID
    auto childHwnd = GetDlgItem(hWnd, 100);
    auto errId = GetLastError();
    
}

void HandleErrorCodeLookup(HWND hWnd) {
    //testAnWindowsError(hWnd);
    // find child window's window handler
    auto hwndEdit = GetDlgItem(hWnd, ID_ET_ERROR);
    auto hwndResultText = GetDlgItem(hWnd, ID_RESULT_TEXT);
    
    // check if child windows exists
    if (hwndEdit == NULL || hwndResultText == NULL) {
        MessageBoxA(hWnd, "system error please contact Hayden", "Error", MB_OK);
        return;
    }

    // varialbles
    char editText[256];
    HLOCAL hlocal = NULL;
    
    // Get the edit window text
    GetWindowTextA(hwndEdit, editText, sizeof(editText)); 

    // convert edit text to DWORD
    char* endptr;
    auto errID = strtoul(editText, &endptr, 10);
    if (*endptr != NULL)
    {
        // convert failed
        SetWindowTextA(hwndResultText, "Please input number.");
        return;
    }


    // Get the error code's texutal description
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, errID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&hlocal, 0, NULL);

    // show error description
    if (hlocal != NULL) {
        SetWindowTextA(hwndResultText, (PCSTR)LocalLock(hlocal));
        LocalFree(hlocal);
    }
    else {
        SetWindowTextA(hwndResultText, "Error number not found.");

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
    {
        // create static controle show message
        CreateWindowA("static", "Error: ", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE , 100, 30, 80, 30, hWnd, (HMENU)STATIC_ERROR_TEXT, hInst, NULL);

        // create edit box
        CreateWindowA("edit", NULL, WS_CHILD | WS_VISIBLE |
            WS_BORDER | ES_LEFT , 330, 30, 180, 30, hWnd, (HMENU)ID_ET_ERROR, hInst, NULL);


        // create button
        CreateWindowA("button", "Look up", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 620, 30, 80, 30, hWnd, (HMENU)ID_LOOKUP_BTN, hInst, NULL);
        
        // create result text
        CreateWindowA("static", "", WS_CHILD | WS_VISIBLE | SS_LEFT , 100, 80, 600, 300, hWnd, (HMENU)ID_RESULT_TEXT, hInst, NULL);


        return 0;
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case ID_LOOKUP_BTN:
                HandleErrorCodeLookup(hWnd);
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
            //RECT rect;
            //GetClientRect(hWnd, &rect);
            //TextOut(hdc, 10, 10, TEXT("hello goold"), 11);
            //DrawText(hdc, TEXT("Hello, Hayden"), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);


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
