#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "MainWindow.h"

static const wchar_t* MUTEX_NAME = L"WinLister_SingleInstance_Mutex";

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Check for single instance
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is running - find and activate it
        HWND hExisting = FindWindowW(L"WinListerMainWindow", nullptr);
        if (hExisting) {
            if (IsIconic(hExisting)) {
                ShowWindow(hExisting, SW_RESTORE);
            }
            SetForegroundWindow(hExisting);
        }
        if (hMutex) {
            CloseHandle(hMutex);
        }
        return 0;
    }

    // Enable high DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    MainWindow mainWindow;

    if (!mainWindow.Create(hInstance)) {
        MessageBoxW(nullptr, L"Failed to create main window.",
            L"Error", MB_OK | MB_ICONERROR);
        if (hMutex) {
            CloseHandle(hMutex);
        }
        return 1;
    }

    mainWindow.Show(nCmdShow);

    int result = mainWindow.Run();

    if (hMutex) {
        CloseHandle(hMutex);
    }

    return result;
}
