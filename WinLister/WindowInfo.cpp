#include "WindowInfo.h"
#include <Psapi.h>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")

bool WindowInfo::IsSystemWindow() const {
    // System windows typically have these characteristics
    static const std::vector<std::wstring> systemClasses = {
        L"Shell_TrayWnd",
        L"Shell_SecondaryTrayWnd",
        L"Progman",
        L"WorkerW",
        L"DV2ControlHost",
        L"MsgrIMEWindowClass",
        L"SysShadow",
        L"Button",
        L"Windows.UI.Core.CoreWindow",
        L"ApplicationFrameWindow",
        L"Windows.UI.Composition.DesktopWindowContentBridge"
    };

    for (const auto& sysClass : systemClasses) {
        if (className == sysClass) {
            return true;
        }
    }

    // Windows with no title and specific styles
    if (title.empty() && (exStyle & WS_EX_TOOLWINDOW)) {
        return true;
    }

    // Cloaked UWP windows
    if (isCloaked && isUWP) {
        return true;
    }

    return false;
}

bool WindowInfo::IsHiddenWindow() const {
    return !isVisible || isCloaked;
}

std::wstring WindowInfo::GetStyleString() const {
    std::wstring result;

    if (style & WS_POPUP) result += L"WS_POPUP | ";
    if (style & WS_CHILD) result += L"WS_CHILD | ";
    if (style & WS_MINIMIZE) result += L"WS_MINIMIZE | ";
    if (style & WS_VISIBLE) result += L"WS_VISIBLE | ";
    if (style & WS_DISABLED) result += L"WS_DISABLED | ";
    if (style & WS_CLIPSIBLINGS) result += L"WS_CLIPSIBLINGS | ";
    if (style & WS_CLIPCHILDREN) result += L"WS_CLIPCHILDREN | ";
    if (style & WS_MAXIMIZE) result += L"WS_MAXIMIZE | ";
    if (style & WS_CAPTION) result += L"WS_CAPTION | ";
    if (style & WS_BORDER) result += L"WS_BORDER | ";
    if (style & WS_DLGFRAME) result += L"WS_DLGFRAME | ";
    if (style & WS_VSCROLL) result += L"WS_VSCROLL | ";
    if (style & WS_HSCROLL) result += L"WS_HSCROLL | ";
    if (style & WS_SYSMENU) result += L"WS_SYSMENU | ";
    if (style & WS_THICKFRAME) result += L"WS_THICKFRAME | ";
    if (style & WS_MINIMIZEBOX) result += L"WS_MINIMIZEBOX | ";
    if (style & WS_MAXIMIZEBOX) result += L"WS_MAXIMIZEBOX | ";

    if (!result.empty()) {
        result = result.substr(0, result.length() - 3);
    }
    return result;
}

std::wstring WindowInfo::GetExStyleString() const {
    std::wstring result;

    if (exStyle & WS_EX_DLGMODALFRAME) result += L"WS_EX_DLGMODALFRAME | ";
    if (exStyle & WS_EX_NOPARENTNOTIFY) result += L"WS_EX_NOPARENTNOTIFY | ";
    if (exStyle & WS_EX_TOPMOST) result += L"WS_EX_TOPMOST | ";
    if (exStyle & WS_EX_ACCEPTFILES) result += L"WS_EX_ACCEPTFILES | ";
    if (exStyle & WS_EX_TRANSPARENT) result += L"WS_EX_TRANSPARENT | ";
    if (exStyle & WS_EX_MDICHILD) result += L"WS_EX_MDICHILD | ";
    if (exStyle & WS_EX_TOOLWINDOW) result += L"WS_EX_TOOLWINDOW | ";
    if (exStyle & WS_EX_WINDOWEDGE) result += L"WS_EX_WINDOWEDGE | ";
    if (exStyle & WS_EX_CLIENTEDGE) result += L"WS_EX_CLIENTEDGE | ";
    if (exStyle & WS_EX_CONTEXTHELP) result += L"WS_EX_CONTEXTHELP | ";
    if (exStyle & WS_EX_RIGHT) result += L"WS_EX_RIGHT | ";
    if (exStyle & WS_EX_RTLREADING) result += L"WS_EX_RTLREADING | ";
    if (exStyle & WS_EX_LEFTSCROLLBAR) result += L"WS_EX_LEFTSCROLLBAR | ";
    if (exStyle & WS_EX_CONTROLPARENT) result += L"WS_EX_CONTROLPARENT | ";
    if (exStyle & WS_EX_STATICEDGE) result += L"WS_EX_STATICEDGE | ";
    if (exStyle & WS_EX_APPWINDOW) result += L"WS_EX_APPWINDOW | ";
    if (exStyle & WS_EX_LAYERED) result += L"WS_EX_LAYERED | ";
    if (exStyle & WS_EX_NOINHERITLAYOUT) result += L"WS_EX_NOINHERITLAYOUT | ";
    if (exStyle & WS_EX_NOREDIRECTIONBITMAP) result += L"WS_EX_NOREDIRECTIONBITMAP | ";
    if (exStyle & WS_EX_LAYOUTRTL) result += L"WS_EX_LAYOUTRTL | ";
    if (exStyle & WS_EX_COMPOSITED) result += L"WS_EX_COMPOSITED | ";
    if (exStyle & WS_EX_NOACTIVATE) result += L"WS_EX_NOACTIVATE | ";

    if (!result.empty()) {
        result = result.substr(0, result.length() - 3);
    }
    return result;
}

std::vector<WindowInfo> WindowEnumerator::EnumerateAllWindows() {
    std::vector<WindowInfo> windows;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

    // Set z-order
    int zOrder = 0;
    for (auto& win : windows) {
        win.zOrder = zOrder++;
    }

    return windows;
}

BOOL CALLBACK WindowEnumerator::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* windows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
    windows->push_back(GetWindowDetails(hwnd));
    return TRUE;
}

WindowInfo WindowEnumerator::GetWindowDetails(HWND hwnd) {
    WindowInfo info = {};
    info.hwnd = hwnd;
    info.hwndParent = GetParent(hwnd);
    info.hwndOwner = GetWindow(hwnd, GW_OWNER);

    // Window title
    int titleLen = GetWindowTextLengthW(hwnd);
    if (titleLen > 0) {
        info.title.resize(titleLen + 1);
        GetWindowTextW(hwnd, &info.title[0], titleLen + 1);
        info.title.resize(titleLen);
    }

    // Class name
    wchar_t className[256] = {};
    GetClassNameW(hwnd, className, 256);
    info.className = className;

    // Process info
    info.threadId = GetWindowThreadProcessId(hwnd, &info.processId);
    info.processName = GetProcessName(info.processId);
    info.processPath = GetProcessPath(info.processId);

    // Rectangles
    GetWindowRect(hwnd, &info.rect);
    GetClientRect(hwnd, &info.clientRect);

    // Styles
    info.style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    info.exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));

    // State flags
    info.isVisible = IsWindowVisible(hwnd) != FALSE;
    info.isEnabled = IsWindowEnabled(hwnd) != FALSE;
    info.isMinimized = IsIconic(hwnd) != FALSE;
    info.isMaximized = IsZoomed(hwnd) != FALSE;
    info.isTopMost = (info.exStyle & WS_EX_TOPMOST) != 0;
    info.isLayered = (info.exStyle & WS_EX_LAYERED) != 0;
    info.isTransparent = (info.exStyle & WS_EX_TRANSPARENT) != 0;
    info.isCloaked = IsWindowCloaked(hwnd);
    info.isUWP = IsUWPWindow(hwnd);
    info.isHung = IsHungAppWindow(hwnd) != FALSE;

    // Alpha/transparency
    info.alpha = 255;
    if (info.isLayered) {
        BYTE alpha = 255;
        DWORD flags = 0;
        if (GetLayeredWindowAttributes(hwnd, nullptr, &alpha, &flags)) {
            if (flags & LWA_ALPHA) {
                info.alpha = alpha;
            }
        }
    }

    // Icon
    info.hIcon = GetWindowIcon(hwnd);

    // DWM extended frame
    info.hasDwmFrame = false;
    BOOL dwmEnabled = FALSE;
    if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled)) && dwmEnabled) {
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS,
            &info.dwmExtendedFrame, sizeof(RECT)))) {
            info.hasDwmFrame = true;
        }
    }

    return info;
}

std::wstring WindowEnumerator::GetProcessName(DWORD processId) {
    std::wstring name;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess) {
        wchar_t buffer[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size)) {
            std::wstring fullPath = buffer;
            size_t pos = fullPath.find_last_of(L"\\/");
            if (pos != std::wstring::npos) {
                name = fullPath.substr(pos + 1);
            } else {
                name = fullPath;
            }
        }
        CloseHandle(hProcess);
    }
    return name;
}

std::wstring WindowEnumerator::GetProcessPath(DWORD processId) {
    std::wstring path;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess) {
        wchar_t buffer[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size)) {
            path = buffer;
        }
        CloseHandle(hProcess);
    }
    return path;
}

HICON WindowEnumerator::GetWindowIcon(HWND hwnd) {
    HICON hIcon = nullptr;

    // Try to get the window's icon
    hIcon = reinterpret_cast<HICON>(SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, nullptr));

    if (!hIcon) {
        hIcon = reinterpret_cast<HICON>(SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0,
            SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, nullptr));
    }

    if (!hIcon) {
        hIcon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICONSM));
    }

    if (!hIcon) {
        hIcon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICON));
    }

    return hIcon;
}

bool WindowEnumerator::IsWindowCloaked(HWND hwnd) {
    BOOL cloaked = FALSE;
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    return SUCCEEDED(hr) && cloaked;
}

bool WindowEnumerator::IsUWPWindow(HWND hwnd) {
    wchar_t className[256] = {};
    GetClassNameW(hwnd, className, 256);
    return wcscmp(className, L"ApplicationFrameWindow") == 0 ||
           wcscmp(className, L"Windows.UI.Core.CoreWindow") == 0;
}
