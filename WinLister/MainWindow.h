#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <CommCtrl.h>
#include <vector>
#include <string>
#include "WindowInfo.h"

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance);
    void Show(int nCmdShow);
    int Run();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnCreate();
    void OnSize(int width, int height);
    void OnCommand(WORD id, WORD notifyCode);
    void OnNotify(NMHDR* pnmhdr);
    void OnDestroy();

    void CreateControls();
    void CreateListView();
    void RefreshWindowList();
    void PopulateListView();
    void UpdateStatusCount();
    void ShowWindowDetails(int index);
    void ApplyFilter();
    void ShowContextMenu(int x, int y);
    void CopyToClipboard(const std::wstring& text);
    void OnColumnClick(int column);
    void SortWindows();
    void OnTimer();
    void UpdateAutoRefresh();
    void ApplyDarkMode();
    void UpdateDarkMode();
    bool IsDarkModeEnabled();
    void SetDarkModeForWindow(HWND hwnd);
    void OnSettingChange(WPARAM wParam, LPARAM lParam);

    HWND m_hwnd;
    HWND m_hListView;
    HWND m_hCheckHideHidden;
    HWND m_hCheckHideSystem;
    HWND m_hBtnRefresh;
    HWND m_hStaticCount;
    HWND m_hEditSearch;
    HWND m_hCheckAutoRefresh;
    HWND m_hEditRefreshTime;
    HWND m_hStaticMs;
    HINSTANCE m_hInstance;
    HIMAGELIST m_hImageList;

    std::vector<WindowInfo> m_allWindows;
    std::vector<WindowInfo> m_filteredWindows;
    std::wstring m_searchText;

    bool m_hideHidden;
    bool m_hideSystem;

    int m_sortColumn;      // -1 = no sort, 0-7 = column index
    int m_sortState;       // 0 = none, 1 = ascending, 2 = descending

    bool m_autoRefresh;
    UINT m_refreshInterval;

    bool m_darkMode;
    HBRUSH m_hDarkBrush;

    static const UINT_PTR TIMER_REFRESH = 1;
    static const wchar_t* CLASS_NAME;
};
