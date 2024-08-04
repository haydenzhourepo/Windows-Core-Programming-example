// 05-JobLab.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "05-JobLab.h"
#include "Job.h"
#include <process.h>  
#include <StrSafe.h>

#pragma comment (lib, "psapi.lib")  

typedef unsigned(__stdcall* PTHREAD_START) (void*);
#define chBEGINTHREADEX(psa, cbStackSize, pfnStartAddr, \
   pvParam, dwCreateFlags, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                          \
         (void *)        (psa),                         \
         (unsigned)      (cbStackSize),                 \
         (PTHREAD_START) (pfnStartAddr),                \
         (void *)        (pvParam),                     \
         (unsigned)      (dwCreateFlags),               \
         (unsigned *)    (pdwThreadId)))

#define MAX_LOADSTRING 100


CJob   g_job;           // Job object
HWND   g_hwnd;          // Handle to dialog box (accessible by all threads)
HANDLE g_hIOCP;         // Completion port that receives Job notifications
HANDLE g_hThreadIOCP;   // Completion port thread

// Completion keys for the completion port
#define COMPKEY_TERMINATE  ((UINT_PTR) 0)
#define COMPKEY_STATUS     ((UINT_PTR) 1)
#define COMPKEY_JOBOBJECT  ((UINT_PTR) 2)


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



//
static const int IDC_PRIORITYCLASS = 1;
static const int IDC_PRIORITYCLASSLABEL  = 2;
static const int IDC_SCHEDULINGCLASS = 3;
static const int IDC_SCHEDULINGCLASSLABEL  = 4;
static const int IDC_STATUS  = 5;
static const int IDC_PERPROCESSUSERTIMELIMIT = 6;
static const int IDC_PERJOBUSERTIMELIMIT = 7;
static const int IDC_APPLYLIMITS = 8;




void GetProcessName(DWORD PID, PTSTR szProcessName, size_t cchSize)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE, PID);
    if (hProcess == NULL) {
        _tcscpy_s(szProcessName, cchSize, TEXT("???"));
        return;
    }

    //if (GetModuleFileNameEx(hProcess, (HMODULE)0, szProcessName, cchSize) 
    //    == 0) {
    //   // GetModuleFileNameEx could fail when the address space
    //   // is not completely initialized. This occurs when the job
    //   // notification happens.
    //   // Hopefully, GetProcessImageFileNameW still works even though
    //   // the obtained path is more complication to decipher
    //   //    \Device\HarddiskVolume1\Windows\System32\notepad.exe
    //   if (!GetProcessImageFileName(hProcess, szProcessName, cchSize)) {
    //      _tcscpy_s(szProcessName, cchSize, TEXT("???"));
    //   }
    //}
    // but it is easier to call this function instead that works fine
    // in all situations.
    DWORD dwSize = (DWORD)cchSize;
    QueryFullProcessImageName(hProcess, 0, szProcessName, &dwSize);

    // Don't forget to close the process handle
    CloseHandle(hProcess);
}


DWORD WINAPI JobNotify(PVOID) {
    TCHAR sz[2000];
    BOOL fDone = FALSE;

    while (!fDone) {
        DWORD dwBytesXferred;
        ULONG_PTR CompKey;
        LPOVERLAPPED po;
        GetQueuedCompletionStatus(g_hIOCP,
            &dwBytesXferred, &CompKey, &po, INFINITE);

        // The app is shutting down, exit this thread
        fDone = (CompKey == COMPKEY_TERMINATE);

        HWND hwndLB = FindWindow(NULL, TEXT("Job Lab"));
        hwndLB = GetDlgItem(hwndLB, IDC_STATUS);

        if (CompKey == COMPKEY_JOBOBJECT) {
            _tcscpy_s(sz, _countof(sz), TEXT("--> Notification: "));
            PTSTR psz = sz + _tcslen(sz);
            switch (dwBytesXferred) {
            case JOB_OBJECT_MSG_END_OF_JOB_TIME:
                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Job time limit reached"));
                break;

            case JOB_OBJECT_MSG_END_OF_PROCESS_TIME: {
                TCHAR szProcessName[MAX_PATH];
                GetProcessName(PtrToUlong(po), szProcessName, MAX_PATH);

                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Job process %s (Id=%d) time limit reached"),
                    szProcessName, po);
            }
                                                   break;

            case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Too many active processes in job"));
                break;

            case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Job contains no active processes"));
                break;

            case JOB_OBJECT_MSG_NEW_PROCESS: {
                TCHAR szProcessName[MAX_PATH];
                GetProcessName(PtrToUlong(po), szProcessName, MAX_PATH);

                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("New process %s (Id=%d) in Job"), szProcessName, po);
            }
                                           break;

            case JOB_OBJECT_MSG_EXIT_PROCESS: {
                TCHAR szProcessName[MAX_PATH];
                GetProcessName(PtrToUlong(po), szProcessName, MAX_PATH);

                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Process %s (Id=%d) terminated"), szProcessName, po);
            }
                                            break;

            case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS: {
                TCHAR szProcessName[MAX_PATH];
                GetProcessName(PtrToUlong(po), szProcessName, MAX_PATH);

                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Process %s (Id=%d) terminated abnormally"),
                    szProcessName, po);
            }
                                                     break;

            case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT: {
                TCHAR szProcessName[MAX_PATH];
                GetProcessName(PtrToUlong(po), szProcessName, MAX_PATH);

                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Process (%s Id=%d) exceeded memory limit"),
                    szProcessName, po);
            }
                                                    break;

            case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT: {
                TCHAR szProcessName[MAX_PATH];
                GetProcessName(PtrToUlong(po), szProcessName, MAX_PATH);

                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Process %s (Id=%d) exceeded job memory limit"),
                    szProcessName, po);
            }
                                                break;

            default:
                StringCchPrintf(psz, _countof(sz) - _tcslen(sz),
                    TEXT("Unknown notification: %d"), dwBytesXferred);
                break;
            }
            ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));
            CompKey = 1;   // Force a status update when a notification arrives
        }


        if (CompKey == COMPKEY_STATUS) {

            static int s_nStatusCount = 0;
            StringCchPrintf(sz, _countof(sz),
                TEXT("--> Status Update (%u)"), s_nStatusCount++);
            ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));

            // Show the basic accounting information
            JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION jobai;
            g_job.QueryBasicAccountingInfo(&jobai);

            StringCchPrintf(sz, _countof(sz),
                TEXT("Total Time: User=%I64u, Kernel=%I64u        ")
                TEXT("Period Time: User=%I64u, Kernel=%I64u"),
                jobai.BasicInfo.TotalUserTime.QuadPart,
                jobai.BasicInfo.TotalKernelTime.QuadPart,
                jobai.BasicInfo.ThisPeriodTotalUserTime.QuadPart,
                jobai.BasicInfo.ThisPeriodTotalKernelTime.QuadPart);
            ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));

            StringCchPrintf(sz, _countof(sz),
                TEXT("Page Faults=%u, Total Processes=%u, ")
                TEXT("Active Processes=%u, Terminated Processes=%u"),
                jobai.BasicInfo.TotalPageFaultCount,
                jobai.BasicInfo.TotalProcesses,
                jobai.BasicInfo.ActiveProcesses,
                jobai.BasicInfo.TotalTerminatedProcesses);
            ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));

            // Show the I/O accounting information
            StringCchPrintf(sz, _countof(sz),
                TEXT("Reads=%I64u (%I64u bytes), ")
                TEXT("Write=%I64u (%I64u bytes), Other=%I64u (%I64u bytes)"),
                jobai.IoInfo.ReadOperationCount, jobai.IoInfo.ReadTransferCount,
                jobai.IoInfo.WriteOperationCount, jobai.IoInfo.WriteTransferCount,
                jobai.IoInfo.OtherOperationCount, jobai.IoInfo.OtherTransferCount);
            ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));

            // Show the peak per-process and job memory usage
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
            g_job.QueryExtendedLimitInfo(&joeli);
            StringCchPrintf(sz, _countof(sz),
                TEXT("Peak memory used: Process=%I64u, Job=%I64u"),
                (__int64)joeli.PeakProcessMemoryUsed,
                (__int64)joeli.PeakJobMemoryUsed);
            ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));

            // Show the set of Process IDs 
            DWORD dwNumProcesses = 50;
            DWORD dwProcessIdList[50];
            g_job.QueryBasicProcessIdList(dwNumProcesses,
                dwProcessIdList, &dwNumProcesses);
            StringCchPrintf(sz, _countof(sz), TEXT("PIDs: %s"),
                (dwNumProcesses == 0) ? TEXT("(none)") : TEXT(""));
            ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));
            TCHAR szProcessName[MAX_PATH];
            for (DWORD x = 0; x < dwNumProcesses; x++) {
                GetProcessName(dwProcessIdList[x],
                    szProcessName, _countof(szProcessName));
                StringCchPrintf(sz, _countof(sz), TEXT("   %d - %s"),
                    dwProcessIdList[x], szProcessName);
                ListBox_SetCurSel(hwndLB, ListBox_AddString(hwndLB, sz));
            }
        }
    }
    return(0);
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Check if we are not already associated with a job.
    // If this is the case, there is no way to switch to another job.
    BOOL bInJob = FALSE;
    IsProcessInJob(GetCurrentProcess(), NULL, &bInJob);
    if (bInJob) {
        MessageBox(NULL, TEXT("Process already in a job"),
            TEXT(""), MB_OK);
        return -1;
    }

    // Create the completion port that receives job notifications
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    // Create a thread that waits on the completion port
    g_hThreadIOCP = chBEGINTHREADEX(NULL, 0, JobNotify, NULL, 0, NULL);

    // Create the job object
    g_job.Create(NULL, TEXT("JobLab"));
    g_job.SetEndOfJobInfo(JOB_OBJECT_POST_AT_END_OF_JOB);
    g_job.AssociateCompletionPort(g_hIOCP, COMPKEY_JOBOBJECT);


    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY05JOBLAB, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY05JOBLAB));

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

    // Post a special key that tells the completion port thread to terminate
    PostQueuedCompletionStatus(g_hIOCP, 0, COMPKEY_TERMINATE, NULL);

    // Wait for the completion port thread to terminate
    WaitForSingleObject(g_hThreadIOCP, INFINITE);

    // Clean up everything properly
    CloseHandle(g_hIOCP);
    CloseHandle(g_hThreadIOCP);


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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY05JOBLAB));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY05JOBLAB);
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

int xPos = 100;
int yPos = 30;
int totalWidth = 600;

void initWindow(HWND hwnd) {

    g_hwnd = hwnd;
    
    CreateWindowW(WC_STATIC, TEXT("Priotiry class"),  WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, xPos, yPos, 200, 20, hwnd, (HMENU)IDC_PRIORITYCLASSLABEL, hInst, NULL);
    HWND hwndPriorityClass = CreateWindowW(WC_COMBOBOX, TEXT(""), CBS_DROPDOWN | CBS_HASSTRINGS | CBS_AUTOHSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL, xPos + 220, yPos, 100, 250, hwnd, (HMENU)IDC_PRIORITYCLASS, hInst, NULL);

    ComboBox_AddString(hwndPriorityClass, TEXT("No limit"));
    ComboBox_AddString(hwndPriorityClass, TEXT("Idle"));
    ComboBox_AddString(hwndPriorityClass, TEXT("Below normal"));
    ComboBox_AddString(hwndPriorityClass, TEXT("Normal"));
    ComboBox_AddString(hwndPriorityClass, TEXT("Above normal"));
    ComboBox_AddString(hwndPriorityClass, TEXT("High"));
    ComboBox_AddString(hwndPriorityClass, TEXT("Realtime"));
    ComboBox_SetCurSel(hwndPriorityClass, 0); // Default to "No Limit"

   /* HWND hwndPriorityClass = CreateWindow(WC_COMBOBOX, TEXT(""), CBS_DROPDOWN | CBS_HASSTRINGS | CBS_AUTOHSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL, 100, 30, 600, 250, hwnd, (HMENU)IDC_PRIORITYCLASS, hInst, NULL);*/
    

    CreateWindowW(WC_STATIC, TEXT("Scheduling class"), WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, xPos + 220 + 120, yPos, 200, 20, hwnd, (HMENU)IDC_SCHEDULINGCLASSLABEL, hInst, NULL);
    HWND hwndSchedulingClass = CreateWindowW(WC_COMBOBOX, TEXT(""), CBS_DROPDOWN | CBS_HASSTRINGS | CBS_AUTOHSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL, xPos + 220 + 120 +220, yPos, 100, 250, hwnd, (HMENU)IDC_SCHEDULINGCLASS, hInst, NULL);


    ComboBox_AddString(hwndSchedulingClass, TEXT("No limit"));
    for (int n = 0; n <= 9; n++) {
        TCHAR szSchedulingClass[2];
        StringCchPrintf(szSchedulingClass, _countof(szSchedulingClass),
            TEXT("%u"), n);
        ComboBox_AddString(hwndSchedulingClass, szSchedulingClass);
    }
    ComboBox_SetCurSel(hwndSchedulingClass, 0); // Default to "No Limit"
    SetTimer(hwnd, 1, 10000, NULL);



    CreateWindowW(WC_STATIC, TEXT("Per-process user time limit(ms)"), WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, xPos, yPos +40, 200, 20, hwnd, (HMENU)110, hInst, NULL);
    CreateWindowW(WC_EDIT, TEXT(""),  WS_CHILD | WS_OVERLAPPED | WS_VISIBLE , xPos + 220, yPos + 40, 100, 250, hwnd, (HMENU)IDC_PERPROCESSUSERTIMELIMIT, hInst, NULL);
    
    CreateWindowW(WC_STATIC, TEXT("Per-job user time limit(ms)"), WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, xPos + 220 + 120, yPos +40, 200, 20, hwnd, (HMENU)111, hInst, NULL);
    CreateWindowW(WC_EDIT, TEXT(""),  WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, xPos + 220 + 120 + 220, yPos + 40, 100, 250, hwnd, (HMENU)IDC_PERJOBUSERTIMELIMIT, hInst, NULL);
    
    CreateWindow(WC_BUTTON, TEXT("Apply limits"), WS_CHILD | WS_VISIBLE | WS_VSCROLL, 280, yPos +80, 100, 30, hwnd, (HMENU)IDC_APPLYLIMITS, hInst, NULL);

    CreateWindow(WC_LISTBOX, TEXT(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 100, 380, 600, 200, hwnd, (HMENU)IDC_STATUS, hInst, NULL);
}

void handlerTimer() {
    PostQueuedCompletionStatus(g_hIOCP, 0, COMPKEY_STATUS, NULL);
}



void ApplyLimits(HWND hwnd) {
    const int nNanosecondsPerSecond = 1000000000;
    const int nMillisecondsPerSecond = 1000;
    const int nNanosecondsPerMillisecond =
        nNanosecondsPerSecond / nMillisecondsPerSecond;
    BOOL f;
    __int64 q;
    SIZE_T s;
    DWORD d;

    // Set Basic and Extended Limits
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli = { 0 };
    joeli.BasicLimitInformation.LimitFlags = 0;

    q = GetDlgItemInt(hwnd, IDC_PERPROCESSUSERTIMELIMIT, &f, FALSE);
    if (f) {
        joeli.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_TIME;
        joeli.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart =
            q * nNanosecondsPerMillisecond / 100;
    }

    q = GetDlgItemInt(hwnd, IDC_PERJOBUSERTIMELIMIT, &f, FALSE);
    if (f) {
        joeli.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_TIME;
        joeli.BasicLimitInformation.PerJobUserTimeLimit.QuadPart =
            q * nNanosecondsPerMillisecond / 100;
    }

    joeli.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
    switch (ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PRIORITYCLASS))) {
    case 0:
        joeli.BasicLimitInformation.LimitFlags &=
            ~JOB_OBJECT_LIMIT_PRIORITY_CLASS;
        break;

    case 1:
        joeli.BasicLimitInformation.PriorityClass =
            IDLE_PRIORITY_CLASS;
        break;

    case 2:
        joeli.BasicLimitInformation.PriorityClass =
            BELOW_NORMAL_PRIORITY_CLASS;
        break;

    case 3:
        joeli.BasicLimitInformation.PriorityClass =
            NORMAL_PRIORITY_CLASS;
        break;

    case 4:
        joeli.BasicLimitInformation.PriorityClass =
            ABOVE_NORMAL_PRIORITY_CLASS;
        break;

    case 5:
        joeli.BasicLimitInformation.PriorityClass =
            HIGH_PRIORITY_CLASS;
        break;

    case 6:
        joeli.BasicLimitInformation.PriorityClass =
            REALTIME_PRIORITY_CLASS;
        break;
    }

    int nSchedulingClass =
        ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SCHEDULINGCLASS));
    if (nSchedulingClass > 0) {
        joeli.BasicLimitInformation.LimitFlags |=
            JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
        joeli.BasicLimitInformation.SchedulingClass = nSchedulingClass - 1;
    }

  
    f = g_job.SetExtendedLimitInfo(&joeli, FALSE);
       

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
    case WM_TIMER:
        handlerTimer();
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
            case IDC_APPLYLIMITS:
                ApplyLimits(hWnd);
                PostQueuedCompletionStatus(g_hIOCP, 0, COMPKEY_STATUS, NULL);
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
