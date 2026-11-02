#include "rules_ui.h"
#include "monitor.h"
#include "resource.h"
#include <algorithm>
#include <commdlg.h>
#include <shlobj.h>
#include <future>
#include <chrono>

namespace
{
constexpr int List = 1001, Action = 1002, Match = 1003, Pattern = 1004, App = 1005, Description = 1006,
              Enabled = 1007;
constexpr int Add = 1010, Update = 1011, Remove = 1012, Import = 1013, Export = 1014, Defaults = 1015,
              Browse = 1016;
struct IoResult
{
    bool ok = false;
    int action = 0;
    std::vector<Rule> rules;
    std::wstring error;
};
struct Editor
{
    std::wstring storage;
    std::vector<Rule> rules;
    std::future<IoResult> io;
};
std::wstring Text(HWND h, int id)
{
    auto control = GetDlgItem(h, id);
    int n = GetWindowTextLengthW(control);
    std::wstring s(n + 1, 0);
    GetWindowTextW(control, s.data(), n + 1);
    s.resize(n);
    return s;
}
void Refresh(HWND h, Editor &e)
{
    SendDlgItemMessageW(h, List, LB_RESETCONTENT, 0, 0);
    for (auto &r : e.rules)
    {
        auto label = std::wstring(r.enabled ? L"[on] " : L"[off] ") +
                     (r.action == RuleAction::Include   ? L"Include "
                      : r.action == RuleAction::Exclude ? L"Exclude "
                                                        : L"Background ") +
                     r.pattern + (r.application.empty() ? L"" : L"  [" + r.application + L"]");
        SendDlgItemMessageW(h, List, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
}
void Fill(HWND h, const Rule &r)
{
    SendDlgItemMessageW(h, Action, CB_SETCURSEL, static_cast<int>(r.action), 0);
    SendDlgItemMessageW(h, Match, CB_SETCURSEL, static_cast<int>(r.match), 0);
    SetDlgItemTextW(h, Pattern, r.pattern.c_str());
    SetDlgItemTextW(h, App, r.application.c_str());
    SetDlgItemTextW(h, Description, r.description.c_str());
    CheckDlgButton(h, Enabled, r.enabled ? BST_CHECKED : BST_UNCHECKED);
}
Rule Read(HWND h)
{
    return {IsDlgButtonChecked(h, Enabled) == BST_CHECKED,
            static_cast<RuleAction>(SendDlgItemMessageW(h, Action, CB_GETCURSEL, 0, 0)),
            static_cast<RuleMatch>(SendDlgItemMessageW(h, Match, CB_GETCURSEL, 0, 0)),
            Text(h, Pattern),
            Text(h, App),
            Text(h, Description)};
}
bool Choose(HWND h, bool save, std::wstring &path)
{
    wchar_t file[32768] = L"VoidSentinel-rules.tsv";
    OPENFILENAMEW ofn{sizeof(ofn)};
    ofn.hwndOwner = h;
    ofn.lpstrFilter = L"VoidSentinel rules (*.tsv)\0*.tsv\0All files\0*.*\0\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = 32768;
    ofn.lpstrDefExt = L"tsv";
    ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn))
    {
        path = file;
        return true;
    }
    return false;
}
void Busy(HWND h, bool busy)
{
    for (int id : {List, Action, Match, Pattern, App, Description, Enabled, Add, Update, Remove, Import,
                   Export, Defaults, Browse, IDOK, IDCANCEL})
        EnableWindow(GetDlgItem(h, id), !busy);
    SetDlgItemTextW(
        h, 1020,
        busy
            ? L"Working..."
            : L"Changes apply after Save rules. Import replaces this list; Cancel keeps your current rules.");
}
INT_PTR CALLBACK Proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    auto e = reinterpret_cast<Editor *>(GetWindowLongPtrW(h, DWLP_USER));
    if (msg == WM_INITDIALOG)
    {
        e = reinterpret_cast<Editor *>(lp);
        SetWindowLongPtrW(h, DWLP_USER, lp);
        for (auto s : {L"Include", L"Exclude", L"Background"})
            SendDlgItemMessageW(h, Action, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
        for (auto s : {L"Folder and descendants", L"Exact path", L"Wildcard (* and ?)"})
            SendDlgItemMessageW(h, Match, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
        Refresh(h, *e);
        Fill(h, e->rules.empty() ? Rule{} : e->rules.back());
        SetTimer(h, 1, 100, nullptr);
        MONITORINFO monitor{sizeof(monitor)};
        if (GetMonitorInfoW(MonitorFromWindow(GetParent(h), MONITOR_DEFAULTTONEAREST), &monitor))
        {
            RECT rect;
            GetWindowRect(h, &rect);
            auto &area = monitor.rcWork;
            SetWindowPos(h, nullptr,
                         area.left + std::max(0L, (area.right - area.left - (rect.right - rect.left)) / 2),
                         area.top + std::max(0L, (area.bottom - area.top - (rect.bottom - rect.top)) / 2), 0,
                         0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return TRUE;
    }
    if (!e)
        return FALSE;
    if (msg == WM_TIMER && e->io.valid() &&
        e->io.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        IoResult result;
        try
        {
            result = e->io.get();
        }
        catch (const std::exception &x)
        {
            result.error = Wide(x.what());
        }
        Busy(h, false);
        if (!result.ok)
            MessageBoxW(h, result.error.c_str(), L"Rules", MB_OK | MB_ICONERROR);
        else if (result.action == Import)
        {
            e->rules = std::move(result.rules);
            Refresh(h, *e);
        }
        else if (result.action == IDOK)
        {
            EndDialog(h, IDOK);
            return TRUE;
        }
        else
            SetDlgItemTextW(h, 1020, L"Rules exported successfully.");
        return TRUE;
    }
    if (msg == WM_CLOSE)
    {
        if (!e->io.valid())
            EndDialog(h, IDCANCEL);
        return TRUE;
    }
    if (msg == WM_DESTROY)
    {
        KillTimer(h, 1);
        return TRUE;
    }
    if (msg != WM_COMMAND)
        return FALSE;
    if (e->io.valid())
        return TRUE;
    int id = LOWORD(wp);
    if (id == IDCANCEL)
    {
        EndDialog(h, IDCANCEL);
        return TRUE;
    }
    if (id == List && HIWORD(wp) == LBN_SELCHANGE)
    {
        auto i = SendDlgItemMessageW(h, List, LB_GETCURSEL, 0, 0);
        if (i != LB_ERR)
            Fill(h, e->rules[i]);
        return TRUE;
    }
    if (id == Add || id == Update)
    {
        auto r = Read(h);
        std::wstring error;
        if (!ValidateRules({r}, error))
        {
            MessageBoxW(h, error.c_str(), L"Invalid rule", MB_OK | MB_ICONERROR);
            return TRUE;
        }
        auto i = SendDlgItemMessageW(h, List, LB_GETCURSEL, 0, 0);
        if (id == Update && i == LB_ERR)
            return TRUE;
        if (id == Add)
            e->rules.push_back(r);
        else
            e->rules[i] = r;
        Refresh(h, *e);
        return TRUE;
    }
    if (id == Remove)
    {
        auto i = SendDlgItemMessageW(h, List, LB_GETCURSEL, 0, 0);
        if (i != LB_ERR)
        {
            e->rules.erase(e->rules.begin() + i);
            Refresh(h, *e);
        }
        return TRUE;
    }
    if (id == Defaults)
    {
        e->rules = DefaultRules();
        Refresh(h, *e);
        return TRUE;
    }
    if (id == Browse)
    {
        BROWSEINFOW info{};
        info.hwndOwner = h;
        info.lpszTitle = L"Choose a folder for this rule";
        info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        auto pidl = SHBrowseForFolderW(&info);
        if (pidl)
        {
            wchar_t path[MAX_PATH];
            if (SHGetPathFromIDListW(pidl, path))
            {
                std::wstring value = path;
                for (auto variable : {L"%LOCALAPPDATA%", L"%APPDATA%", L"%PROGRAMDATA%", L"%USERPROFILE%"})
                {
                    wchar_t base[32768];
                    if (ExpandEnvironmentStringsW(variable, base, 32768))
                    {
                        auto v = NormalizePath(value), b = NormalizePath(base);
                        if (v == b || v.starts_with(b + L"\\"))
                        {
                            value = std::wstring(variable) + value.substr(wcslen(base));
                            break;
                        }
                    }
                }
                SetDlgItemTextW(h, Pattern, value.c_str());
                SendDlgItemMessageW(h, Match, CB_SETCURSEL, 0, 0);
            }
            CoTaskMemFree(pidl);
        }
        return TRUE;
    }
    if (id == IDOK || id == Import || id == Export)
    {
        std::wstring path = e->storage;
        if (id != IDOK && !Choose(h, id == Export, path))
            return TRUE;
        auto rules = e->rules;
        Busy(h, true);
        e->io = std::async(std::launch::async, [path, rules, id]() mutable {
            IoResult result;
            result.action = id;
            result.rules = std::move(rules);
            result.ok = id == Import ? LoadRules(path, result.rules, result.error)
                                     : SaveRules(path, result.rules, result.error);
            return result;
        });
        return TRUE;
    }
    return FALSE;
}
} // namespace
bool EditRules(HWND owner, const std::wstring &storage, std::vector<Rule> &rules, const Rule *suggested)
{
    Editor editor{storage, rules, {}};
    if (suggested)
        editor.rules.push_back(*suggested);
    if (DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_RULES), owner, Proc,
                        reinterpret_cast<LPARAM>(&editor)) != IDOK)
        return false;
    rules = std::move(editor.rules);
    return true;
}
