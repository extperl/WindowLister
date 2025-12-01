#include "DetailDialog.h"
#include "resource.h"
#include <sstream>
#include <iomanip>

DetailDialog::DetailDialog(HWND hwndParent, const WindowInfo& windowInfo)
    : m_hwndParent(hwndParent)
    , m_windowInfo(windowInfo)
{
}

void DetailDialog::Show() {
    DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_DETAIL),
        m_hwndParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );
}

INT_PTR CALLBACK DetailDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DetailDialog* pThis = nullptr;

    if (msg == WM_INITDIALOG) {
        pThis = reinterpret_cast<DetailDialog*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<DetailDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR DetailDialog::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        OnInitDialog(hwnd);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_COPY:
            CopyToClipboard(hwnd);
            return TRUE;

        case IDC_BTN_CLOSE:
        case IDCANCEL:
            KillTimer(hwnd, TIMER_REFRESH);
            EndDialog(hwnd, 0);
            return TRUE;

        case IDC_BTN_SET_TITLE:
            SetWindowTitle(hwnd);
            return TRUE;

        case IDC_BTN_SET_POS:
            SetWindowPosition(hwnd);
            return TRUE;

        case IDC_BTN_SET_STATE:
            SetWindowState(hwnd);
            return TRUE;

        case IDC_BTN_SET_ALPHA:
            SetWindowAlpha(hwnd);
            return TRUE;

        case IDC_BTN_MINIMIZE:
            MinimizeWindow(hwnd);
            return TRUE;

        case IDC_BTN_MAXIMIZE:
            MaximizeWindow(hwnd);
            return TRUE;

        case IDC_BTN_RESTORE:
            RestoreWindow(hwnd);
            return TRUE;

        case IDC_BTN_REFRESH_INFO:
            RefreshWindowInfo(hwnd);
            return TRUE;

        case IDC_DETAIL_AUTO_REFRESH:
            m_autoRefresh = (SendMessageW(m_hCheckAutoRefresh, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (m_autoRefresh) {
                SetTimer(hwnd, TIMER_REFRESH, m_refreshInterval, nullptr);
            } else {
                KillTimer(hwnd, TIMER_REFRESH);
            }
            return TRUE;

        case IDC_DETAIL_REFRESH_TIME:
            if (HIWORD(wParam) == EN_CHANGE) {
                wchar_t buffer[16] = {};
                GetWindowTextW(m_hEditRefreshTime, buffer, 16);
                int interval = _wtoi(buffer);
                if (interval >= 100) {  // Minimum 100ms
                    m_refreshInterval = interval;
                    if (m_autoRefresh) {
                        KillTimer(hwnd, TIMER_REFRESH);
                        SetTimer(hwnd, TIMER_REFRESH, m_refreshInterval, nullptr);
                    }
                }
            }
            return TRUE;
        }
        break;

    case WM_TIMER:
        if (wParam == TIMER_REFRESH) {
            if (IsTargetWindowValid()) {
                RefreshWindowInfo(hwnd);
            } else {
                KillTimer(hwnd, TIMER_REFRESH);
                SendMessageW(m_hCheckAutoRefresh, BM_SETCHECK, BST_UNCHECKED, 0);
                m_autoRefresh = false;
            }
            return TRUE;
        }
        break;

    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == m_hSliderAlpha) {
            OnAlphaSliderChanged(hwnd);
            return TRUE;
        }
        break;

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        // Resize tab control
        if (m_hTabControl) {
            MoveWindow(m_hTabControl, 10, 10, width - 20, height - 60, TRUE);
        }

        // Calculate tab content area
        RECT tabRect = { 10, 10, width - 10, height - 60 };
        if (m_hTabControl) {
            SendMessageW(m_hTabControl, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&tabRect));
        }

        int tabContentWidth = tabRect.right - tabRect.left;
        int tabContentHeight = tabRect.bottom - tabRect.top;

        // Resize ListView (Details tab)
        if (m_hListView) {
            MoveWindow(m_hListView, tabRect.left, tabRect.top, tabContentWidth, tabContentHeight, TRUE);
        }

        // Bottom toolbar - left side: Auto refresh, interval, ms, Refresh button
        int y = height - 40;
        if (m_hCheckAutoRefresh) {
            MoveWindow(m_hCheckAutoRefresh, 10, y + 3, 95, 24, TRUE);
        }
        if (m_hEditRefreshTime) {
            MoveWindow(m_hEditRefreshTime, 110, y + 2, 50, 24, TRUE);
        }
        if (m_hStaticMs) {
            MoveWindow(m_hStaticMs, 165, y + 6, 25, 20, TRUE);
        }
        if (m_hBtnRefresh) {
            MoveWindow(m_hBtnRefresh, 195, y, 80, 28, TRUE);
        }

        // Bottom toolbar - right side: Copy, Close buttons
        if (m_hBtnCopy && m_hBtnClose) {
            MoveWindow(m_hBtnCopy, width - 220, y, 100, 28, TRUE);
            MoveWindow(m_hBtnClose, width - 110, y, 100, 28, TRUE);
        }
        return TRUE;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 550;
        mmi->ptMinTrackSize.y = 500;
        return TRUE;
    }

    case WM_NOTIFY: {
        NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
        if (pnmhdr->idFrom == IDC_DETAIL_LIST && pnmhdr->code == NM_RCLICK) {
            NMITEMACTIVATE* pnmia = reinterpret_cast<NMITEMACTIVATE*>(pnmhdr);
            if (pnmia->iItem >= 0) {
                POINT pt;
                GetCursorPos(&pt);
                ShowContextMenu(hwnd, pt.x, pt.y);
            }
            return TRUE;
        }
        if (pnmhdr->idFrom == IDC_TAB_CONTROL && pnmhdr->code == TCN_SELCHANGE) {
            OnTabChanged(hwnd);
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}

void DetailDialog::OnInitDialog(HWND hwnd) {
    // Set dialog title
    std::wstring title = L"Window Details: ";
    if (!m_windowInfo.title.empty()) {
        title += m_windowInfo.title;
    } else {
        title += L"(no title)";
    }
    SetWindowTextW(hwnd, title.c_str());

    // Center dialog
    RECT rcParent, rcDialog;
    GetWindowRect(m_hwndParent, &rcParent);
    GetWindowRect(hwnd, &rcDialog);
    int x = rcParent.left + ((rcParent.right - rcParent.left) - (rcDialog.right - rcDialog.left)) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcDialog.bottom - rcDialog.top)) / 2;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    // Create tab control first
    CreateTabControl(hwnd);

    // Create modify tab controls
    CreateModifyControls(hwnd);

    // Populate details list
    PopulateDetails(hwnd);

    // Populate modify tab with current values
    PopulateModifyTab(hwnd);

    // Show details tab by default
    ShowDetailsTab(hwnd, true);
    ShowModifyTab(hwnd, false);
}

void DetailDialog::CreateTabControl(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    // Create tab control
    m_hTabControl = CreateWindowExW(
        0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        10, 10, rc.right - 20, rc.bottom - 60,
        hwnd, reinterpret_cast<HMENU>(IDC_TAB_CONTROL),
        GetModuleHandle(nullptr), nullptr
    );

    // Set font
    HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    if (hFont) {
        SendMessageW(m_hTabControl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }

    // Add tabs
    TCITEMW tie = {};
    tie.mask = TCIF_TEXT;

    tie.pszText = const_cast<wchar_t*>(L"Details");
    TabCtrl_InsertItem(m_hTabControl, 0, &tie);

    tie.pszText = const_cast<wchar_t*>(L"Modify");
    TabCtrl_InsertItem(m_hTabControl, 1, &tie);

    // Get tab content area
    RECT tabRect = { 10, 10, rc.right - 10, rc.bottom - 60 };
    SendMessageW(m_hTabControl, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&tabRect));

    // Create ListView for details tab (using existing control from resource)
    m_hListView = GetDlgItem(hwnd, IDC_DETAIL_LIST);
    if (m_hListView) {
        // Move ListView to tab content area
        MoveWindow(m_hListView, tabRect.left, tabRect.top,
            tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);
    }

    // Get existing buttons
    m_hBtnCopy = GetDlgItem(hwnd, IDC_BTN_COPY);
    m_hBtnClose = GetDlgItem(hwnd, IDC_BTN_CLOSE);

    HINSTANCE hInst = GetModuleHandle(nullptr);

    // Create bottom toolbar controls (left side): Auto refresh checkbox, interval, ms label, Refresh button
    // These are positioned in WM_SIZE handler

    m_hCheckAutoRefresh = CreateWindowExW(
        0, L"BUTTON", L"Auto refresh",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, rc.bottom - 40, 95, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_DETAIL_AUTO_REFRESH), hInst, nullptr);
    SendMessageW(m_hCheckAutoRefresh, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hEditRefreshTime = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"1000",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_RIGHT,
        110, rc.bottom - 40, 50, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_DETAIL_REFRESH_TIME), hInst, nullptr);
    SendMessageW(m_hEditRefreshTime, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hStaticMs = CreateWindowExW(
        0, L"STATIC", L"ms",
        WS_CHILD | WS_VISIBLE,
        165, rc.bottom - 36, 25, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(m_hStaticMs, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnRefresh = CreateWindowExW(
        0, L"BUTTON", L"Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        195, rc.bottom - 40, 80, 28,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_REFRESH_INFO), hInst, nullptr);
    SendMessageW(m_hBtnRefresh, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Enable auto-refresh by default
    m_autoRefresh = true;
    SendMessageW(m_hCheckAutoRefresh, BM_SETCHECK, BST_CHECKED, 0);
    SetTimer(hwnd, TIMER_REFRESH, m_refreshInterval, nullptr);
}

void DetailDialog::CreateModifyControls(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    // Get tab content area
    RECT tabRect = { 10, 10, rc.right - 10, rc.bottom - 60 };
    SendMessageW(m_hTabControl, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&tabRect));

    int contentX = tabRect.left + 10;
    int contentY = tabRect.top + 10;
    int contentWidth = tabRect.right - tabRect.left - 20;

    HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
    HINSTANCE hInst = GetModuleHandle(nullptr);

    int labelWidth = 100;
    int editWidth = contentWidth - labelWidth - 90;
    int buttonWidth = 75;
    int rowHeight = 28;
    int spacing = 8;
    int y = contentY;

    // === Title Section ===
    HWND hLabel = CreateWindowExW(0, L"STATIC", L"Title:",
        WS_CHILD | SS_RIGHT,
        contentX, y + 4, labelWidth, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hEditTitle = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | ES_AUTOHSCROLL,
        contentX + labelWidth + 5, y, editWidth, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_TITLE), hInst, nullptr);
    SendMessageW(m_hEditTitle, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnSetTitle = CreateWindowExW(0, L"BUTTON", L"Set",
        WS_CHILD | BS_PUSHBUTTON,
        contentX + labelWidth + editWidth + 10, y, buttonWidth, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_SET_TITLE), hInst, nullptr);
    SendMessageW(m_hBtnSetTitle, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight + spacing * 2;

    // === Position/Size Section ===
    hLabel = CreateWindowExW(0, L"STATIC", L"Position/Size:",
        WS_CHILD | SS_LEFT,
        contentX, y, 200, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight;

    // Left
    hLabel = CreateWindowExW(0, L"STATIC", L"Left:",
        WS_CHILD | SS_RIGHT,
        contentX, y + 4, 50, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hEditLeft = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | ES_NUMBER,
        contentX + 55, y, 70, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_LEFT), hInst, nullptr);
    SendMessageW(m_hEditLeft, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Top
    hLabel = CreateWindowExW(0, L"STATIC", L"Top:",
        WS_CHILD | SS_RIGHT,
        contentX + 135, y + 4, 50, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hEditTop = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | ES_NUMBER,
        contentX + 190, y, 70, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_TOP), hInst, nullptr);
    SendMessageW(m_hEditTop, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Width
    hLabel = CreateWindowExW(0, L"STATIC", L"Width:",
        WS_CHILD | SS_RIGHT,
        contentX + 270, y + 4, 50, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hEditWidth = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | ES_NUMBER,
        contentX + 325, y, 70, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_WIDTH), hInst, nullptr);
    SendMessageW(m_hEditWidth, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight;

    // Height
    hLabel = CreateWindowExW(0, L"STATIC", L"Height:",
        WS_CHILD | SS_RIGHT,
        contentX + 270, y + 4, 50, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hEditHeight = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | ES_NUMBER,
        contentX + 325, y, 70, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_HEIGHT), hInst, nullptr);
    SendMessageW(m_hEditHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnSetPos = CreateWindowExW(0, L"BUTTON", L"Apply",
        WS_CHILD | BS_PUSHBUTTON,
        contentX + 405, y, buttonWidth, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_SET_POS), hInst, nullptr);
    SendMessageW(m_hBtnSetPos, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight + spacing * 2;

    // === State Section ===
    hLabel = CreateWindowExW(0, L"STATIC", L"State:",
        WS_CHILD | SS_LEFT,
        contentX, y, 100, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight;

    m_hCheckVisible = CreateWindowExW(0, L"BUTTON", L"Visible",
        WS_CHILD | BS_AUTOCHECKBOX,
        contentX + 10, y, 80, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_CHECK_VISIBLE), hInst, nullptr);
    SendMessageW(m_hCheckVisible, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hCheckEnabled = CreateWindowExW(0, L"BUTTON", L"Enabled",
        WS_CHILD | BS_AUTOCHECKBOX,
        contentX + 100, y, 80, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_CHECK_ENABLED), hInst, nullptr);
    SendMessageW(m_hCheckEnabled, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hCheckTopmost = CreateWindowExW(0, L"BUTTON", L"Topmost",
        WS_CHILD | BS_AUTOCHECKBOX,
        contentX + 190, y, 90, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_CHECK_TOPMOST), hInst, nullptr);
    SendMessageW(m_hCheckTopmost, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnSetState = CreateWindowExW(0, L"BUTTON", L"Apply",
        WS_CHILD | BS_PUSHBUTTON,
        contentX + 290, y, buttonWidth, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_SET_STATE), hInst, nullptr);
    SendMessageW(m_hBtnSetState, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight + spacing;

    // Window control buttons
    m_hBtnMinimize = CreateWindowExW(0, L"BUTTON", L"Minimize",
        WS_CHILD | BS_PUSHBUTTON,
        contentX + 10, y, 85, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_MINIMIZE), hInst, nullptr);
    SendMessageW(m_hBtnMinimize, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnMaximize = CreateWindowExW(0, L"BUTTON", L"Maximize",
        WS_CHILD | BS_PUSHBUTTON,
        contentX + 105, y, 85, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_MAXIMIZE), hInst, nullptr);
    SendMessageW(m_hBtnMaximize, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnRestore = CreateWindowExW(0, L"BUTTON", L"Restore",
        WS_CHILD | BS_PUSHBUTTON,
        contentX + 200, y, 85, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_RESTORE), hInst, nullptr);
    SendMessageW(m_hBtnRestore, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight + spacing * 2;

    // === Transparency Section ===
    hLabel = CreateWindowExW(0, L"STATIC", L"Transparency:",
        WS_CHILD | SS_LEFT,
        contentX, y, 150, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    y += rowHeight;

    hLabel = CreateWindowExW(0, L"STATIC", L"Alpha:",
        WS_CHILD | SS_RIGHT,
        contentX, y + 4, 50, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hSliderAlpha = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
        WS_CHILD | TBS_HORZ | TBS_NOTICKS,
        contentX + 55, y, 250, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_SLIDER_ALPHA), hInst, nullptr);
    SendMessageW(m_hSliderAlpha, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
    SendMessageW(m_hSliderAlpha, TBM_SETPOS, TRUE, 255);

    m_hEditAlpha = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"255",
        WS_CHILD | ES_NUMBER | ES_CENTER,
        contentX + 315, y, 50, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_EDIT_ALPHA), hInst, nullptr);
    SendMessageW(m_hEditAlpha, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hBtnSetAlpha = CreateWindowExW(0, L"BUTTON", L"Apply",
        WS_CHILD | BS_PUSHBUTTON,
        contentX + 375, y, buttonWidth, 24,
        hwnd, reinterpret_cast<HMENU>(IDC_BTN_SET_ALPHA), hInst, nullptr);
    SendMessageW(m_hBtnSetAlpha, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
}

void DetailDialog::PopulateDetails(HWND hwnd) {
    if (!m_hListView) return;

    ListView_DeleteAllItems(m_hListView);

    ListView_SetExtendedListViewStyle(m_hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Add columns if not already added
    if (ListView_GetItemCount(m_hListView) == 0) {
        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.cx = 200;
        col.pszText = const_cast<wchar_t*>(L"Property");
        ListView_InsertColumn(m_hListView, 0, &col);

        col.cx = 450;
        col.pszText = const_cast<wchar_t*>(L"Value");
        ListView_InsertColumn(m_hListView, 1, &col);
    }

    // Basic info
    wchar_t buffer[512];

    swprintf_s(buffer, L"0x%p", m_windowInfo.hwnd);
    AddDetailItem(m_hListView, L"HWND", buffer);

    swprintf_s(buffer, L"0x%p", m_windowInfo.hwndParent);
    AddDetailItem(m_hListView, L"Parent HWND", buffer);

    swprintf_s(buffer, L"0x%p", m_windowInfo.hwndOwner);
    AddDetailItem(m_hListView, L"Owner HWND", buffer);

    AddDetailItem(m_hListView, L"Title", m_windowInfo.title.empty() ? L"(empty)" : m_windowInfo.title.c_str());
    AddDetailItem(m_hListView, L"Class", m_windowInfo.className.c_str());

    // Process info
    AddDetailItem(m_hListView, L"", L"--- Process Information ---");

    swprintf_s(buffer, L"%u", m_windowInfo.processId);
    AddDetailItem(m_hListView, L"Process ID (PID)", buffer);

    swprintf_s(buffer, L"%u", m_windowInfo.threadId);
    AddDetailItem(m_hListView, L"Thread ID", buffer);

    AddDetailItem(m_hListView, L"Process Name", m_windowInfo.processName.empty() ? L"(unknown)" : m_windowInfo.processName.c_str());
    AddDetailItem(m_hListView, L"Process Path", m_windowInfo.processPath.empty() ? L"(unknown)" : m_windowInfo.processPath.c_str());

    // Position and size
    AddDetailItem(m_hListView, L"", L"--- Position and Size ---");

    swprintf_s(buffer, L"Left: %d, Top: %d, Right: %d, Bottom: %d",
        m_windowInfo.rect.left, m_windowInfo.rect.top,
        m_windowInfo.rect.right, m_windowInfo.rect.bottom);
    AddDetailItem(m_hListView, L"Window Rectangle", buffer);

    swprintf_s(buffer, L"%d x %d",
        m_windowInfo.rect.right - m_windowInfo.rect.left,
        m_windowInfo.rect.bottom - m_windowInfo.rect.top);
    AddDetailItem(m_hListView, L"Window Size", buffer);

    swprintf_s(buffer, L"Width: %d, Height: %d",
        m_windowInfo.clientRect.right - m_windowInfo.clientRect.left,
        m_windowInfo.clientRect.bottom - m_windowInfo.clientRect.top);
    AddDetailItem(m_hListView, L"Client Area", buffer);

    if (m_windowInfo.hasDwmFrame) {
        swprintf_s(buffer, L"Left: %d, Top: %d, Right: %d, Bottom: %d",
            m_windowInfo.dwmExtendedFrame.left, m_windowInfo.dwmExtendedFrame.top,
            m_windowInfo.dwmExtendedFrame.right, m_windowInfo.dwmExtendedFrame.bottom);
        AddDetailItem(m_hListView, L"DWM Extended Frame", buffer);
    }

    // State
    AddDetailItem(m_hListView, L"", L"--- State ---");

    AddDetailItem(m_hListView, L"Visible", m_windowInfo.isVisible ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"Enabled", m_windowInfo.isEnabled ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"Minimized", m_windowInfo.isMinimized ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"Maximized", m_windowInfo.isMaximized ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"Topmost", m_windowInfo.isTopMost ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"Cloaked (hidden)", m_windowInfo.isCloaked ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"UWP App", m_windowInfo.isUWP ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"Hung", m_windowInfo.isHung ? L"Yes" : L"No");

    swprintf_s(buffer, L"%d", m_windowInfo.zOrder);
    AddDetailItem(m_hListView, L"Z-Order", buffer);

    // Layered window info
    if (m_windowInfo.isLayered) {
        AddDetailItem(m_hListView, L"", L"--- Transparency ---");
        AddDetailItem(m_hListView, L"Layered", L"Yes");

        swprintf_s(buffer, L"%d (%.1f%%)", m_windowInfo.alpha, (m_windowInfo.alpha / 255.0f) * 100.0f);
        AddDetailItem(m_hListView, L"Alpha", buffer);

        AddDetailItem(m_hListView, L"Transparent (mouse clicks)", m_windowInfo.isTransparent ? L"Yes" : L"No");
    }

    // Styles
    AddDetailItem(m_hListView, L"", L"--- Styles ---");

    swprintf_s(buffer, L"0x%08X", m_windowInfo.style);
    AddDetailItem(m_hListView, L"Style (hex)", buffer);

    std::wstring styleStr = m_windowInfo.GetStyleString();
    if (!styleStr.empty()) {
        AddDetailItem(m_hListView, L"Style (Flags)", styleStr.c_str());
    }

    swprintf_s(buffer, L"0x%08X", m_windowInfo.exStyle);
    AddDetailItem(m_hListView, L"ExStyle (hex)", buffer);

    std::wstring exStyleStr = m_windowInfo.GetExStyleString();
    if (!exStyleStr.empty()) {
        AddDetailItem(m_hListView, L"ExStyle (Flags)", exStyleStr.c_str());
    }

    // Classification
    AddDetailItem(m_hListView, L"", L"--- Classification ---");
    AddDetailItem(m_hListView, L"System Window", m_windowInfo.IsSystemWindow() ? L"Yes" : L"No");
    AddDetailItem(m_hListView, L"Hidden Window", m_windowInfo.IsHiddenWindow() ? L"Yes" : L"No");
}

void DetailDialog::PopulateModifyTab(HWND hwnd) {
    // Set title
    SetWindowTextW(m_hEditTitle, m_windowInfo.title.c_str());

    // Set position/size
    wchar_t buffer[32];
    swprintf_s(buffer, L"%d", m_windowInfo.rect.left);
    SetWindowTextW(m_hEditLeft, buffer);

    swprintf_s(buffer, L"%d", m_windowInfo.rect.top);
    SetWindowTextW(m_hEditTop, buffer);

    swprintf_s(buffer, L"%d", m_windowInfo.rect.right - m_windowInfo.rect.left);
    SetWindowTextW(m_hEditWidth, buffer);

    swprintf_s(buffer, L"%d", m_windowInfo.rect.bottom - m_windowInfo.rect.top);
    SetWindowTextW(m_hEditHeight, buffer);

    // Set state checkboxes
    SendMessageW(m_hCheckVisible, BM_SETCHECK, m_windowInfo.isVisible ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_hCheckEnabled, BM_SETCHECK, m_windowInfo.isEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_hCheckTopmost, BM_SETCHECK, m_windowInfo.isTopMost ? BST_CHECKED : BST_UNCHECKED, 0);

    // Set alpha
    BYTE alpha = m_windowInfo.isLayered ? m_windowInfo.alpha : 255;
    SendMessageW(m_hSliderAlpha, TBM_SETPOS, TRUE, alpha);
    swprintf_s(buffer, L"%d", alpha);
    SetWindowTextW(m_hEditAlpha, buffer);
}

void DetailDialog::AddDetailItem(HWND hListView, const wchar_t* property, const wchar_t* value) {
    int index = ListView_GetItemCount(hListView);

    LVITEMW item = {};
    item.mask = LVIF_TEXT;
    item.iItem = index;
    item.iSubItem = 0;
    item.pszText = const_cast<wchar_t*>(property);
    ListView_InsertItem(hListView, &item);

    ListView_SetItemText(hListView, index, 1, const_cast<wchar_t*>(value));
}

void DetailDialog::OnTabChanged(HWND hwnd) {
    int sel = TabCtrl_GetCurSel(m_hTabControl);
    m_currentTab = sel;

    if (sel == 0) {
        ShowDetailsTab(hwnd, true);
        ShowModifyTab(hwnd, false);
    } else {
        ShowDetailsTab(hwnd, false);
        ShowModifyTab(hwnd, true);
    }
}

void DetailDialog::ShowDetailsTab(HWND hwnd, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    if (m_hListView) ShowWindow(m_hListView, cmd);
    if (m_hBtnCopy) ShowWindow(m_hBtnCopy, cmd);
}

void DetailDialog::ShowModifyTab(HWND hwnd, bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    if (m_hEditTitle) ShowWindow(m_hEditTitle, cmd);
    if (m_hBtnSetTitle) ShowWindow(m_hBtnSetTitle, cmd);
    if (m_hEditLeft) ShowWindow(m_hEditLeft, cmd);
    if (m_hEditTop) ShowWindow(m_hEditTop, cmd);
    if (m_hEditWidth) ShowWindow(m_hEditWidth, cmd);
    if (m_hEditHeight) ShowWindow(m_hEditHeight, cmd);
    if (m_hBtnSetPos) ShowWindow(m_hBtnSetPos, cmd);
    if (m_hCheckVisible) ShowWindow(m_hCheckVisible, cmd);
    if (m_hCheckEnabled) ShowWindow(m_hCheckEnabled, cmd);
    if (m_hCheckTopmost) ShowWindow(m_hCheckTopmost, cmd);
    if (m_hBtnSetState) ShowWindow(m_hBtnSetState, cmd);
    if (m_hSliderAlpha) ShowWindow(m_hSliderAlpha, cmd);
    if (m_hEditAlpha) ShowWindow(m_hEditAlpha, cmd);
    if (m_hBtnSetAlpha) ShowWindow(m_hBtnSetAlpha, cmd);
    if (m_hBtnMinimize) ShowWindow(m_hBtnMinimize, cmd);
    if (m_hBtnMaximize) ShowWindow(m_hBtnMaximize, cmd);
    if (m_hBtnRestore) ShowWindow(m_hBtnRestore, cmd);

    // Show all labels for modify tab
    HWND hChild = GetWindow(hwnd, GW_CHILD);
    while (hChild) {
        wchar_t className[64];
        GetClassNameW(hChild, className, 64);
        if (wcscmp(className, L"Static") == 0) {
            // Check if this is a modify tab label (not the copy button label)
            HWND hParent = GetParent(hChild);
            if (hParent == hwnd) {
                // Check position to determine if it's a modify tab control
                RECT rc;
                GetWindowRect(hChild, &rc);
                MapWindowPoints(HWND_DESKTOP, hwnd, reinterpret_cast<LPPOINT>(&rc), 2);

                // Labels in the modify area (below the tab header)
                if (rc.top > 40) {
                    ShowWindow(hChild, cmd);
                }
            }
        }
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
}

bool DetailDialog::IsTargetWindowValid() {
    return IsWindow(m_windowInfo.hwnd) != FALSE;
}

void DetailDialog::SetWindowTitle(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t buffer[512];
    GetWindowTextW(m_hEditTitle, buffer, 512);

    if (SetWindowTextW(m_windowInfo.hwnd, buffer)) {
        m_windowInfo.title = buffer;
        MessageBoxW(hwnd, L"Window title updated successfully.", L"Success", MB_OK | MB_ICONINFORMATION);
    } else {
        DWORD error = GetLastError();
        swprintf_s(buffer, L"Failed to set window title. Error code: %u", error);
        MessageBoxW(hwnd, buffer, L"Error", MB_OK | MB_ICONERROR);
    }
}

void DetailDialog::SetWindowPosition(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t buffer[32];

    GetWindowTextW(m_hEditLeft, buffer, 32);
    int left = _wtoi(buffer);

    GetWindowTextW(m_hEditTop, buffer, 32);
    int top = _wtoi(buffer);

    GetWindowTextW(m_hEditWidth, buffer, 32);
    int width = _wtoi(buffer);

    GetWindowTextW(m_hEditHeight, buffer, 32);
    int height = _wtoi(buffer);

    if (width <= 0 || height <= 0) {
        MessageBoxW(hwnd, L"Width and height must be positive values.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (SetWindowPos(m_windowInfo.hwnd, nullptr, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE)) {
        // Update stored info
        m_windowInfo.rect.left = left;
        m_windowInfo.rect.top = top;
        m_windowInfo.rect.right = left + width;
        m_windowInfo.rect.bottom = top + height;
        MessageBoxW(hwnd, L"Window position/size updated successfully.", L"Success", MB_OK | MB_ICONINFORMATION);
    } else {
        DWORD error = GetLastError();
        swprintf_s(buffer, L"Failed to set window position. Error code: %u", error);
        MessageBoxW(hwnd, buffer, L"Error", MB_OK | MB_ICONERROR);
    }
}

void DetailDialog::SetWindowState(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    bool visible = SendMessageW(m_hCheckVisible, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool enabled = SendMessageW(m_hCheckEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool topmost = SendMessageW(m_hCheckTopmost, BM_GETCHECK, 0, 0) == BST_CHECKED;

    // Set visibility
    ShowWindow(m_windowInfo.hwnd, visible ? SW_SHOW : SW_HIDE);
    m_windowInfo.isVisible = visible;

    // Set enabled state
    EnableWindow(m_windowInfo.hwnd, enabled);
    m_windowInfo.isEnabled = enabled;

    // Set topmost
    HWND insertAfter = topmost ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(m_windowInfo.hwnd, insertAfter, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    m_windowInfo.isTopMost = topmost;

    MessageBoxW(hwnd, L"Window state updated successfully.", L"Success", MB_OK | MB_ICONINFORMATION);
}

void DetailDialog::SetWindowAlpha(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t buffer[32];
    GetWindowTextW(m_hEditAlpha, buffer, 32);
    int alpha = _wtoi(buffer);

    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;

    // Get current extended style
    LONG_PTR exStyle = GetWindowLongPtrW(m_windowInfo.hwnd, GWL_EXSTYLE);

    if (alpha == 255) {
        // Remove layered style if fully opaque
        SetWindowLongPtrW(m_windowInfo.hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        m_windowInfo.isLayered = false;
    } else {
        // Add layered style and set alpha
        if (!(exStyle & WS_EX_LAYERED)) {
            SetWindowLongPtrW(m_windowInfo.hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        }
        SetLayeredWindowAttributes(m_windowInfo.hwnd, 0, static_cast<BYTE>(alpha), LWA_ALPHA);
        m_windowInfo.isLayered = true;
        m_windowInfo.alpha = static_cast<BYTE>(alpha);
    }

    MessageBoxW(hwnd, L"Window transparency updated successfully.", L"Success", MB_OK | MB_ICONINFORMATION);
}

void DetailDialog::MinimizeWindow(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    ShowWindow(m_windowInfo.hwnd, SW_MINIMIZE);
    m_windowInfo.isMinimized = true;
    m_windowInfo.isMaximized = false;
}

void DetailDialog::MaximizeWindow(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    ShowWindow(m_windowInfo.hwnd, SW_MAXIMIZE);
    m_windowInfo.isMaximized = true;
    m_windowInfo.isMinimized = false;
}

void DetailDialog::RestoreWindow(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    ShowWindow(m_windowInfo.hwnd, SW_RESTORE);
    m_windowInfo.isMinimized = false;
    m_windowInfo.isMaximized = false;
}

void DetailDialog::RefreshWindowInfo(HWND hwnd) {
    if (!IsTargetWindowValid()) {
        MessageBoxW(hwnd, L"The target window no longer exists.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Save scroll position and selection
    int topIndex = ListView_GetTopIndex(m_hListView);
    int selectedItem = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);

    // Re-gather window information
    m_windowInfo = WindowEnumerator::GetWindowDetails(m_windowInfo.hwnd);

    // Update dialog title
    std::wstring title = L"Window Details: ";
    if (!m_windowInfo.title.empty()) {
        title += m_windowInfo.title;
    } else {
        title += L"(no title)";
    }
    SetWindowTextW(hwnd, title.c_str());

    // Update both tabs
    PopulateDetails(hwnd);
    PopulateModifyTab(hwnd);

    // Restore scroll position and selection
    if (topIndex > 0) {
        ListView_EnsureVisible(m_hListView, ListView_GetItemCount(m_hListView) - 1, FALSE);
        ListView_EnsureVisible(m_hListView, topIndex, FALSE);
    }
    if (selectedItem >= 0 && selectedItem < ListView_GetItemCount(m_hListView)) {
        ListView_SetItemState(m_hListView, selectedItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void DetailDialog::OnAlphaSliderChanged(HWND hwnd) {
    int pos = static_cast<int>(SendMessageW(m_hSliderAlpha, TBM_GETPOS, 0, 0));
    wchar_t buffer[32];
    swprintf_s(buffer, L"%d", pos);
    SetWindowTextW(m_hEditAlpha, buffer);
}

void DetailDialog::CopyToClipboard(HWND hwnd) {
    if (!m_hListView) return;

    std::wstringstream ss;
    int count = ListView_GetItemCount(m_hListView);

    for (int i = 0; i < count; i++) {
        wchar_t prop[256] = {};
        wchar_t val[512] = {};

        ListView_GetItemText(m_hListView, i, 0, prop, 256);
        ListView_GetItemText(m_hListView, i, 1, val, 512);

        if (prop[0] != L'\0' || val[0] != L'\0') {
            ss << prop << L"\t" << val << L"\r\n";
        }
    }

    std::wstring text = ss.str();

    if (OpenClipboard(hwnd)) {
        EmptyClipboard();

        size_t size = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hMem) {
            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
            if (pMem) {
                wcscpy_s(pMem, text.length() + 1, text.c_str());
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }

        CloseClipboard();

        MessageBoxW(hwnd, L"Details have been copied to clipboard.",
            L"Copied", MB_OK | MB_ICONINFORMATION);
    }
}

void DetailDialog::ShowContextMenu(HWND hwnd, int x, int y) {
    if (!m_hListView) return;

    int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel < 0) return;

    wchar_t prop[256] = {};
    wchar_t val[512] = {};
    ListView_GetItemText(m_hListView, sel, 0, prop, 256);
    ListView_GetItemText(m_hListView, sel, 1, val, 512);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_DETAIL_COPY_PROP, L"Copy Property");
    AppendMenuW(hMenu, MF_STRING, IDM_DETAIL_COPY_VALUE, L"Copy Value");
    AppendMenuW(hMenu, MF_STRING, IDM_DETAIL_COPY_BOTH, L"Copy Both");

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);

    switch (cmd) {
    case IDM_DETAIL_COPY_PROP:
        CopyTextToClipboard(hwnd, prop);
        break;
    case IDM_DETAIL_COPY_VALUE:
        CopyTextToClipboard(hwnd, val);
        break;
    case IDM_DETAIL_COPY_BOTH: {
        std::wstring both = prop;
        both += L"\t";
        both += val;
        CopyTextToClipboard(hwnd, both);
        break;
    }
    }
}

void DetailDialog::CopyTextToClipboard(HWND hwnd, const std::wstring& text) {
    if (OpenClipboard(hwnd)) {
        EmptyClipboard();

        size_t size = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hMem) {
            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
            if (pMem) {
                wcscpy_s(pMem, text.length() + 1, text.c_str());
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }

        CloseClipboard();
    }
}
