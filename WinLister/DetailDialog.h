#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <CommCtrl.h>
#include "WindowInfo.h"

class DetailDialog {
public:
    DetailDialog(HWND hwndParent, const WindowInfo& windowInfo);
    void Show();

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnInitDialog(HWND hwnd);
    void CreateTabControl(HWND hwnd);
    void CreateModifyControls(HWND hwnd);
    void PopulateDetails(HWND hwnd);
    void PopulateModifyTab(HWND hwnd);
    void AddDetailItem(HWND hListView, const wchar_t* property, const wchar_t* value);
    void CopyToClipboard(HWND hwnd);
    void ShowContextMenu(HWND hwnd, int x, int y);
    void CopyTextToClipboard(HWND hwnd, const std::wstring& text);

    // Tab switching
    void OnTabChanged(HWND hwnd);
    void ShowDetailsTab(HWND hwnd, bool show);
    void ShowModifyTab(HWND hwnd, bool show);

    // Window modification functions
    void SetWindowTitle(HWND hwnd);
    void SetWindowPosition(HWND hwnd);
    void SetWindowState(HWND hwnd);
    void SetWindowAlpha(HWND hwnd);
    void MinimizeWindow(HWND hwnd);
    void MaximizeWindow(HWND hwnd);
    void RestoreWindow(HWND hwnd);
    void RefreshWindowInfo(HWND hwnd);
    void OnAlphaSliderChanged(HWND hwnd);

    bool IsTargetWindowValid();

    HWND m_hwndParent;
    WindowInfo m_windowInfo;

    // Tab control
    HWND m_hTabControl = nullptr;
    int m_currentTab = 0;

    // Details tab controls
    HWND m_hListView = nullptr;
    HWND m_hBtnCopy = nullptr;

    // Modify tab controls
    HWND m_hEditTitle = nullptr;
    HWND m_hBtnSetTitle = nullptr;
    HWND m_hEditLeft = nullptr;
    HWND m_hEditTop = nullptr;
    HWND m_hEditWidth = nullptr;
    HWND m_hEditHeight = nullptr;
    HWND m_hBtnSetPos = nullptr;
    HWND m_hCheckVisible = nullptr;
    HWND m_hCheckEnabled = nullptr;
    HWND m_hCheckTopmost = nullptr;
    HWND m_hBtnSetState = nullptr;
    HWND m_hSliderAlpha = nullptr;
    HWND m_hEditAlpha = nullptr;
    HWND m_hBtnSetAlpha = nullptr;
    HWND m_hBtnMinimize = nullptr;
    HWND m_hBtnMaximize = nullptr;
    HWND m_hBtnRestore = nullptr;
    HWND m_hBtnRefresh = nullptr;
    HWND m_hBtnClose = nullptr;
};
