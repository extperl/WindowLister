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
            EndDialog(hwnd, 0);
            return TRUE;
        }
        break;

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        HWND hListView = GetDlgItem(hwnd, IDC_DETAIL_LIST);
        if (hListView) {
            MoveWindow(hListView, 10, 10, width - 20, height - 60, TRUE);
        }

        HWND hBtnCopy = GetDlgItem(hwnd, IDC_BTN_COPY);
        HWND hBtnClose = GetDlgItem(hwnd, IDC_BTN_CLOSE);
        if (hBtnCopy && hBtnClose) {
            MoveWindow(hBtnCopy, width - 230, height - 40, 100, 30, TRUE);
            MoveWindow(hBtnClose, width - 120, height - 40, 100, 30, TRUE);
        }
        return TRUE;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 500;
        mmi->ptMinTrackSize.y = 400;
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

    PopulateDetails(hwnd);
}

void DetailDialog::PopulateDetails(HWND hwnd) {
    HWND hListView = GetDlgItem(hwnd, IDC_DETAIL_LIST);
    if (!hListView) return;

    ListView_SetExtendedListViewStyle(hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Add columns
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 200;
    col.pszText = const_cast<wchar_t*>(L"Property");
    ListView_InsertColumn(hListView, 0, &col);

    col.cx = 450;
    col.pszText = const_cast<wchar_t*>(L"Value");
    ListView_InsertColumn(hListView, 1, &col);

    // Basic info
    wchar_t buffer[512];

    swprintf_s(buffer, L"0x%p", m_windowInfo.hwnd);
    AddDetailItem(hListView, L"HWND", buffer);

    swprintf_s(buffer, L"0x%p", m_windowInfo.hwndParent);
    AddDetailItem(hListView, L"Parent HWND", buffer);

    swprintf_s(buffer, L"0x%p", m_windowInfo.hwndOwner);
    AddDetailItem(hListView, L"Owner HWND", buffer);

    AddDetailItem(hListView, L"Title", m_windowInfo.title.empty() ? L"(empty)" : m_windowInfo.title.c_str());
    AddDetailItem(hListView, L"Class", m_windowInfo.className.c_str());

    // Process info
    AddDetailItem(hListView, L"", L"--- Process Information ---");

    swprintf_s(buffer, L"%u", m_windowInfo.processId);
    AddDetailItem(hListView, L"Process ID (PID)", buffer);

    swprintf_s(buffer, L"%u", m_windowInfo.threadId);
    AddDetailItem(hListView, L"Thread ID", buffer);

    AddDetailItem(hListView, L"Process Name", m_windowInfo.processName.empty() ? L"(unknown)" : m_windowInfo.processName.c_str());
    AddDetailItem(hListView, L"Process Path", m_windowInfo.processPath.empty() ? L"(unknown)" : m_windowInfo.processPath.c_str());

    // Position and size
    AddDetailItem(hListView, L"", L"--- Position and Size ---");

    swprintf_s(buffer, L"Left: %d, Top: %d, Right: %d, Bottom: %d",
        m_windowInfo.rect.left, m_windowInfo.rect.top,
        m_windowInfo.rect.right, m_windowInfo.rect.bottom);
    AddDetailItem(hListView, L"Window Rectangle", buffer);

    swprintf_s(buffer, L"%d x %d",
        m_windowInfo.rect.right - m_windowInfo.rect.left,
        m_windowInfo.rect.bottom - m_windowInfo.rect.top);
    AddDetailItem(hListView, L"Window Size", buffer);

    swprintf_s(buffer, L"Width: %d, Height: %d",
        m_windowInfo.clientRect.right - m_windowInfo.clientRect.left,
        m_windowInfo.clientRect.bottom - m_windowInfo.clientRect.top);
    AddDetailItem(hListView, L"Client Area", buffer);

    if (m_windowInfo.hasDwmFrame) {
        swprintf_s(buffer, L"Left: %d, Top: %d, Right: %d, Bottom: %d",
            m_windowInfo.dwmExtendedFrame.left, m_windowInfo.dwmExtendedFrame.top,
            m_windowInfo.dwmExtendedFrame.right, m_windowInfo.dwmExtendedFrame.bottom);
        AddDetailItem(hListView, L"DWM Extended Frame", buffer);
    }

    // State
    AddDetailItem(hListView, L"", L"--- State ---");

    AddDetailItem(hListView, L"Visible", m_windowInfo.isVisible ? L"Yes" : L"No");
    AddDetailItem(hListView, L"Enabled", m_windowInfo.isEnabled ? L"Yes" : L"No");
    AddDetailItem(hListView, L"Minimized", m_windowInfo.isMinimized ? L"Yes" : L"No");
    AddDetailItem(hListView, L"Maximized", m_windowInfo.isMaximized ? L"Yes" : L"No");
    AddDetailItem(hListView, L"Topmost", m_windowInfo.isTopMost ? L"Yes" : L"No");
    AddDetailItem(hListView, L"Cloaked (hidden)", m_windowInfo.isCloaked ? L"Yes" : L"No");
    AddDetailItem(hListView, L"UWP App", m_windowInfo.isUWP ? L"Yes" : L"No");
    AddDetailItem(hListView, L"Hung", m_windowInfo.isHung ? L"Yes" : L"No");

    swprintf_s(buffer, L"%d", m_windowInfo.zOrder);
    AddDetailItem(hListView, L"Z-Order", buffer);

    // Layered window info
    if (m_windowInfo.isLayered) {
        AddDetailItem(hListView, L"", L"--- Transparency ---");
        AddDetailItem(hListView, L"Layered", L"Yes");

        swprintf_s(buffer, L"%d (%.1f%%)", m_windowInfo.alpha, (m_windowInfo.alpha / 255.0f) * 100.0f);
        AddDetailItem(hListView, L"Alpha", buffer);

        AddDetailItem(hListView, L"Transparent (mouse clicks)", m_windowInfo.isTransparent ? L"Yes" : L"No");
    }

    // Styles
    AddDetailItem(hListView, L"", L"--- Styles ---");

    swprintf_s(buffer, L"0x%08X", m_windowInfo.style);
    AddDetailItem(hListView, L"Style (hex)", buffer);

    std::wstring styleStr = m_windowInfo.GetStyleString();
    if (!styleStr.empty()) {
        AddDetailItem(hListView, L"Style (Flags)", styleStr.c_str());
    }

    swprintf_s(buffer, L"0x%08X", m_windowInfo.exStyle);
    AddDetailItem(hListView, L"ExStyle (hex)", buffer);

    std::wstring exStyleStr = m_windowInfo.GetExStyleString();
    if (!exStyleStr.empty()) {
        AddDetailItem(hListView, L"ExStyle (Flags)", exStyleStr.c_str());
    }

    // Classification
    AddDetailItem(hListView, L"", L"--- Classification ---");
    AddDetailItem(hListView, L"System Window", m_windowInfo.IsSystemWindow() ? L"Yes" : L"No");
    AddDetailItem(hListView, L"Hidden Window", m_windowInfo.IsHiddenWindow() ? L"Yes" : L"No");
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

void DetailDialog::CopyToClipboard(HWND hwnd) {
    HWND hListView = GetDlgItem(hwnd, IDC_DETAIL_LIST);
    if (!hListView) return;

    std::wstringstream ss;
    int count = ListView_GetItemCount(hListView);

    for (int i = 0; i < count; i++) {
        wchar_t prop[256] = {};
        wchar_t val[512] = {};

        ListView_GetItemText(hListView, i, 0, prop, 256);
        ListView_GetItemText(hListView, i, 1, val, 512);

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
    HWND hListView = GetDlgItem(hwnd, IDC_DETAIL_LIST);
    if (!hListView) return;

    int sel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (sel < 0) return;

    wchar_t prop[256] = {};
    wchar_t val[512] = {};
    ListView_GetItemText(hListView, sel, 0, prop, 256);
    ListView_GetItemText(hListView, sel, 1, val, 512);

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
