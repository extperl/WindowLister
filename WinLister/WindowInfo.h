#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>
#include <vector>
#include <dwmapi.h>

struct WindowInfo {
    HWND hwnd;
    HWND hwndParent;
    HWND hwndOwner;
    std::wstring title;
    std::wstring className;
    DWORD processId;
    DWORD threadId;
    std::wstring processName;
    std::wstring processPath;
    RECT rect;
    RECT clientRect;
    DWORD style;
    DWORD exStyle;
    bool isVisible;
    bool isEnabled;
    bool isMinimized;
    bool isMaximized;
    bool isTopMost;
    bool isLayered;
    bool isTransparent;
    bool isCloaked;
    bool isUWP;
    bool isHung;
    BYTE alpha;
    int zOrder;
    HICON hIcon;

    // DWM Info
    bool hasDwmFrame;
    RECT dwmExtendedFrame;

    bool IsSystemWindow() const;
    bool IsHiddenWindow() const;
    std::wstring GetStyleString() const;
    std::wstring GetExStyleString() const;
};

class WindowEnumerator {
public:
    static std::vector<WindowInfo> EnumerateAllWindows();
    static WindowInfo GetWindowDetails(HWND hwnd);

private:
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    static std::wstring GetProcessName(DWORD processId);
    static std::wstring GetProcessPath(DWORD processId);
    static HICON GetWindowIcon(HWND hwnd);
    static bool IsWindowCloaked(HWND hwnd);
    static bool IsUWPWindow(HWND hwnd);
};
