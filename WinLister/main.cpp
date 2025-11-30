#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "MainWindow.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Enable high DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    MainWindow mainWindow;

    if (!mainWindow.Create(hInstance)) {
        MessageBoxW(nullptr, L"Failed to create main window.",
            L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    mainWindow.Show(nCmdShow);

    return mainWindow.Run();
}
