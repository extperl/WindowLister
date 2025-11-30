#include "MainWindow.h"
#include "DetailDialog.h"
#include "resource.h"
#include <windowsx.h>
#include <sstream>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

const wchar_t* MainWindow::CLASS_NAME = L"WinListerMainWindow";

MainWindow::MainWindow()
    : m_hwnd(nullptr)
    , m_hListView(nullptr)
    , m_hCheckHideHidden(nullptr)
    , m_hCheckHideSystem(nullptr)
    , m_hBtnRefresh(nullptr)
    , m_hStaticCount(nullptr)
    , m_hEditSearch(nullptr)
    , m_hInstance(nullptr)
    , m_hImageList(nullptr)
    , m_hideHidden(true)
    , m_hideSystem(true)
    , m_sortColumn(-1)
    , m_sortState(0)
{
}

MainWindow::~MainWindow() {
    if (m_hImageList) {
        ImageList_Destroy(m_hImageList);
    }
}

bool MainWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    m_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"WinLister - Window Information Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 700,
        nullptr, nullptr, hInstance, this
    );

    return m_hwnd != nullptr;
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

int MainWindow::Run() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<MainWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        OnCreate();
        return 0;

    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND:
        OnCommand(LOWORD(wParam), HIWORD(wParam));
        return 0;

    case WM_NOTIFY:
        OnNotify(reinterpret_cast<NMHDR*>(lParam));
        return 0;

    case WM_DESTROY:
        OnDestroy();
        return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 800;
        mmi->ptMinTrackSize.y = 400;
        return 0;
    }
    }

    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

void MainWindow::OnCreate() {
    CreateControls();
    RefreshWindowList();
}

void MainWindow::CreateControls() {
    HFONT hFont = CreateFontW(
        -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    // Search box
    CreateWindowExW(0, L"STATIC", L"Search:",
        WS_CHILD | WS_VISIBLE,
        10, 12, 50, 20,
        m_hwnd, nullptr, m_hInstance, nullptr);

    m_hEditSearch = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        65, 10, 200, 24,
        m_hwnd, reinterpret_cast<HMENU>(IDC_EDIT_SEARCH), m_hInstance, nullptr
    );

    // Checkboxes
    m_hCheckHideHidden = CreateWindowExW(
        0, L"BUTTON", L"Hide hidden windows",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        280, 12, 150, 20,
        m_hwnd, reinterpret_cast<HMENU>(IDC_CHECK_HIDE_HIDDEN), m_hInstance, nullptr
    );
    Button_SetCheck(m_hCheckHideHidden, BST_CHECKED);

    m_hCheckHideSystem = CreateWindowExW(
        0, L"BUTTON", L"Hide system windows",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        440, 12, 150, 20,
        m_hwnd, reinterpret_cast<HMENU>(IDC_CHECK_HIDE_SYSTEM), m_hInstance, nullptr
    );
    Button_SetCheck(m_hCheckHideSystem, BST_CHECKED);

    // Refresh button
    m_hBtnRefresh = CreateWindowExW(
        0, L"BUTTON", L"Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        600, 8, 90, 28,
        m_hwnd, reinterpret_cast<HMENU>(IDC_BTN_REFRESH), m_hInstance, nullptr
    );

    // Status count
    m_hStaticCount = CreateWindowExW(
        0, L"STATIC", L"0 windows",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        700, 12, 150, 20,
        m_hwnd, reinterpret_cast<HMENU>(IDC_STATIC_COUNT), m_hInstance, nullptr
    );

    // Apply font
    SendMessage(m_hEditSearch, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessage(m_hCheckHideHidden, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessage(m_hCheckHideSystem, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessage(m_hBtnRefresh, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    SendMessage(m_hStaticCount, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    CreateListView();
}

void MainWindow::CreateListView() {
    m_hListView = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        10, 45, 780, 400,
        m_hwnd,
        reinterpret_cast<HMENU>(IDC_LISTVIEW),
        m_hInstance,
        nullptr
    );

    ListView_SetExtendedListViewStyle(m_hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);

    // Create image list
    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 100, 100);
    ListView_SetImageList(m_hListView, m_hImageList, LVSIL_SMALL);

    // Add columns
    struct ColumnInfo {
        const wchar_t* name;
        int width;
    };

    ColumnInfo columns[] = {
        { L"HWND", 100 },
        { L"Title", 250 },
        { L"Class", 180 },
        { L"Process", 150 },
        { L"PID", 60 },
        { L"Visible", 70 },
        { L"Position", 150 },
        { L"Size", 100 }
    };

    for (int i = 0; i < _countof(columns); i++) {
        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        col.fmt = LVCFMT_LEFT;
        col.cx = columns[i].width;
        col.pszText = const_cast<wchar_t*>(columns[i].name);
        ListView_InsertColumn(m_hListView, i, &col);
    }
}

void MainWindow::OnSize(int width, int height) {
    if (m_hListView) {
        MoveWindow(m_hListView, 10, 45, width - 20, height - 55, TRUE);
    }
    if (m_hStaticCount) {
        MoveWindow(m_hStaticCount, width - 160, 12, 150, 20, TRUE);
    }
}

void MainWindow::OnCommand(WORD id, WORD notifyCode) {
    switch (id) {
    case IDC_BTN_REFRESH:
        RefreshWindowList();
        break;

    case IDC_CHECK_HIDE_HIDDEN:
        m_hideHidden = Button_GetCheck(m_hCheckHideHidden) == BST_CHECKED;
        ApplyFilter();
        break;

    case IDC_CHECK_HIDE_SYSTEM:
        m_hideSystem = Button_GetCheck(m_hCheckHideSystem) == BST_CHECKED;
        ApplyFilter();
        break;

    case IDC_EDIT_SEARCH:
        if (notifyCode == EN_CHANGE) {
            wchar_t buffer[256] = {};
            GetWindowTextW(m_hEditSearch, buffer, 256);
            m_searchText = buffer;
            ApplyFilter();
        }
        break;
    }
}

void MainWindow::OnNotify(NMHDR* pnmhdr) {
    if (pnmhdr->idFrom == IDC_LISTVIEW) {
        switch (pnmhdr->code) {
        case NM_DBLCLK: {
            NMITEMACTIVATE* pnmia = reinterpret_cast<NMITEMACTIVATE*>(pnmhdr);
            if (pnmia->iItem >= 0) {
                ShowWindowDetails(pnmia->iItem);
            }
            break;
        }
        case NM_RCLICK: {
            NMITEMACTIVATE* pnmia = reinterpret_cast<NMITEMACTIVATE*>(pnmhdr);
            if (pnmia->iItem >= 0) {
                POINT pt;
                GetCursorPos(&pt);
                ShowContextMenu(pt.x, pt.y);
            }
            break;
        }
        case LVN_KEYDOWN: {
            NMLVKEYDOWN* pnkd = reinterpret_cast<NMLVKEYDOWN*>(pnmhdr);
            if (pnkd->wVKey == VK_RETURN) {
                int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                if (sel >= 0) {
                    ShowWindowDetails(sel);
                }
            }
            break;
        }
        case LVN_COLUMNCLICK: {
            NMLISTVIEW* pnmlv = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
            OnColumnClick(pnmlv->iSubItem);
            break;
        }
        }
    }
}

void MainWindow::OnDestroy() {
    PostQuitMessage(0);
}

void MainWindow::RefreshWindowList() {
    m_allWindows = WindowEnumerator::EnumerateAllWindows();
    ApplyFilter();
}

void MainWindow::ApplyFilter() {
    m_filteredWindows.clear();

    std::wstring searchLower = m_searchText;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::towlower);

    for (const auto& win : m_allWindows) {
        // Filter hidden windows
        if (m_hideHidden && win.IsHiddenWindow()) {
            continue;
        }

        // Filter system windows
        if (m_hideSystem && win.IsSystemWindow()) {
            continue;
        }

        // Search filter
        if (!searchLower.empty()) {
            std::wstring titleLower = win.title;
            std::wstring classLower = win.className;
            std::wstring processLower = win.processName;

            std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::towlower);
            std::transform(classLower.begin(), classLower.end(), classLower.begin(), ::towlower);
            std::transform(processLower.begin(), processLower.end(), processLower.begin(), ::towlower);

            bool match = titleLower.find(searchLower) != std::wstring::npos ||
                         classLower.find(searchLower) != std::wstring::npos ||
                         processLower.find(searchLower) != std::wstring::npos;

            if (!match) {
                continue;
            }
        }

        m_filteredWindows.push_back(win);
    }

    PopulateListView();
    UpdateStatusCount();
}

void MainWindow::PopulateListView() {
    SetWindowRedraw(m_hListView, FALSE);
    ListView_DeleteAllItems(m_hListView);
    ImageList_RemoveAll(m_hImageList);

    for (size_t i = 0; i < m_filteredWindows.size(); i++) {
        const auto& win = m_filteredWindows[i];

        // Add icon
        int imageIndex = -1;
        if (win.hIcon) {
            imageIndex = ImageList_AddIcon(m_hImageList, win.hIcon);
        }

        LVITEMW item = {};
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = static_cast<int>(i);
        item.iSubItem = 0;
        item.lParam = reinterpret_cast<LPARAM>(win.hwnd);
        item.iImage = imageIndex;

        // HWND
        wchar_t hwndStr[32];
        swprintf_s(hwndStr, L"%llX", reinterpret_cast<unsigned long long>(win.hwnd));
        item.pszText = hwndStr;
        ListView_InsertItem(m_hListView, &item);

        // Title
        ListView_SetItemText(m_hListView, static_cast<int>(i), 1,
            const_cast<wchar_t*>(win.title.empty() ? L"(no title)" : win.title.c_str()));

        // Class
        ListView_SetItemText(m_hListView, static_cast<int>(i), 2,
            const_cast<wchar_t*>(win.className.c_str()));

        // Process
        ListView_SetItemText(m_hListView, static_cast<int>(i), 3,
            const_cast<wchar_t*>(win.processName.empty() ? L"(unknown)" : win.processName.c_str()));

        // PID
        wchar_t pidStr[16];
        swprintf_s(pidStr, L"%u", win.processId);
        ListView_SetItemText(m_hListView, static_cast<int>(i), 4, pidStr);

        // Visible
        ListView_SetItemText(m_hListView, static_cast<int>(i), 5,
            const_cast<wchar_t*>(win.isVisible ? L"Yes" : L"No"));

        // Position
        wchar_t posStr[64];
        swprintf_s(posStr, L"%d, %d", win.rect.left, win.rect.top);
        ListView_SetItemText(m_hListView, static_cast<int>(i), 6, posStr);

        // Size
        wchar_t sizeStr[64];
        swprintf_s(sizeStr, L"%d x %d",
            win.rect.right - win.rect.left,
            win.rect.bottom - win.rect.top);
        ListView_SetItemText(m_hListView, static_cast<int>(i), 7, sizeStr);
    }

    SetWindowRedraw(m_hListView, TRUE);
    InvalidateRect(m_hListView, nullptr, TRUE);
}

void MainWindow::UpdateStatusCount() {
    wchar_t buffer[128];
    swprintf_s(buffer, L"%zu of %zu windows",
        m_filteredWindows.size(), m_allWindows.size());
    SetWindowTextW(m_hStaticCount, buffer);
}

void MainWindow::ShowWindowDetails(int index) {
    if (index < 0 || index >= static_cast<int>(m_filteredWindows.size())) {
        return;
    }

    DetailDialog dialog(m_hwnd, m_filteredWindows[index]);
    dialog.Show();
}

void MainWindow::ShowContextMenu(int x, int y) {
    int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= static_cast<int>(m_filteredWindows.size())) {
        return;
    }

    const auto& win = m_filteredWindows[sel];

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_COPY_HWND, L"Copy HWND");
    AppendMenuW(hMenu, MF_STRING, IDM_COPY_TITLE, L"Copy Title");
    AppendMenuW(hMenu, MF_STRING, IDM_COPY_CLASS, L"Copy Class");
    AppendMenuW(hMenu, MF_STRING, IDM_COPY_PROCESS, L"Copy Process Name");
    AppendMenuW(hMenu, MF_STRING, IDM_COPY_PID, L"Copy PID");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_COPY_ALL, L"Copy All");

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, m_hwnd, nullptr);
    DestroyMenu(hMenu);

    wchar_t buffer[1024];
    switch (cmd) {
    case IDM_COPY_HWND:
        swprintf_s(buffer, L"%llX", reinterpret_cast<unsigned long long>(win.hwnd));
        CopyToClipboard(buffer);
        break;
    case IDM_COPY_TITLE:
        CopyToClipboard(win.title);
        break;
    case IDM_COPY_CLASS:
        CopyToClipboard(win.className);
        break;
    case IDM_COPY_PROCESS:
        CopyToClipboard(win.processName);
        break;
    case IDM_COPY_PID:
        swprintf_s(buffer, L"%u", win.processId);
        CopyToClipboard(buffer);
        break;
    case IDM_COPY_ALL:
        swprintf_s(buffer, L"HWND: %llX\r\nTitle: %s\r\nClass: %s\r\nProcess: %s\r\nPID: %u",
            reinterpret_cast<unsigned long long>(win.hwnd),
            win.title.c_str(),
            win.className.c_str(),
            win.processName.c_str(),
            win.processId);
        CopyToClipboard(buffer);
        break;
    }
}

void MainWindow::CopyToClipboard(const std::wstring& text) {
    if (OpenClipboard(m_hwnd)) {
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

void MainWindow::OnColumnClick(int column) {
    if (m_sortColumn == column) {
        // Same column clicked - cycle through states
        m_sortState = (m_sortState + 1) % 3;
        if (m_sortState == 0) {
            m_sortColumn = -1;
        }
    } else {
        // New column clicked
        m_sortColumn = column;
        m_sortState = 1; // Start with ascending
    }

    // Update header to show sort arrow
    HWND hHeader = ListView_GetHeader(m_hListView);
    for (int i = 0; i < 8; i++) {
        HDITEMW hdi = {};
        hdi.mask = HDI_FORMAT;
        Header_GetItem(hHeader, i, &hdi);
        hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (i == m_sortColumn && m_sortState != 0) {
            hdi.fmt |= (m_sortState == 1) ? HDF_SORTUP : HDF_SORTDOWN;
        }
        Header_SetItem(hHeader, i, &hdi);
    }

    SortWindows();
    PopulateListView();
}

void MainWindow::SortWindows() {
    if (m_sortColumn < 0 || m_sortState == 0) {
        // No sorting - restore original order from ApplyFilter
        ApplyFilter();
        return;
    }

    bool ascending = (m_sortState == 1);

    std::sort(m_filteredWindows.begin(), m_filteredWindows.end(),
        [this, ascending](const WindowInfo& a, const WindowInfo& b) {
            int cmp = 0;

            switch (m_sortColumn) {
            case 0: // HWND
                cmp = (reinterpret_cast<uintptr_t>(a.hwnd) < reinterpret_cast<uintptr_t>(b.hwnd)) ? -1 :
                      (reinterpret_cast<uintptr_t>(a.hwnd) > reinterpret_cast<uintptr_t>(b.hwnd)) ? 1 : 0;
                break;
            case 1: // Title
                cmp = _wcsicmp(a.title.c_str(), b.title.c_str());
                break;
            case 2: // Class
                cmp = _wcsicmp(a.className.c_str(), b.className.c_str());
                break;
            case 3: // Process
                cmp = _wcsicmp(a.processName.c_str(), b.processName.c_str());
                break;
            case 4: // PID
                cmp = (a.processId < b.processId) ? -1 : (a.processId > b.processId) ? 1 : 0;
                break;
            case 5: // Visible
                cmp = (a.isVisible == b.isVisible) ? 0 : (a.isVisible ? -1 : 1);
                break;
            case 6: // Position (sort by left, then top)
                cmp = (a.rect.left < b.rect.left) ? -1 : (a.rect.left > b.rect.left) ? 1 :
                      (a.rect.top < b.rect.top) ? -1 : (a.rect.top > b.rect.top) ? 1 : 0;
                break;
            case 7: { // Size (sort by total pixels)
                int sizeA = (a.rect.right - a.rect.left) * (a.rect.bottom - a.rect.top);
                int sizeB = (b.rect.right - b.rect.left) * (b.rect.bottom - b.rect.top);
                cmp = (sizeA < sizeB) ? -1 : (sizeA > sizeB) ? 1 : 0;
                break;
            }
            }

            return ascending ? (cmp < 0) : (cmp > 0);
        });
}
