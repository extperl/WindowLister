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
    void PopulateDetails(HWND hwnd);
    void AddDetailItem(HWND hListView, const wchar_t* property, const wchar_t* value);
    void CopyToClipboard(HWND hwnd);
    void ShowContextMenu(HWND hwnd, int x, int y);
    void CopyTextToClipboard(HWND hwnd, const std::wstring& text);

    HWND m_hwndParent;
    WindowInfo m_windowInfo;
};
