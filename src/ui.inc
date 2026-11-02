// Included inside main.cpp's anonymous namespace. All controls are standard Win32 controls.
constexpr int Add = 101, Remove = 102, Start = 103, Stop = 104, Save = 105, Children = 106, Targets = 107;
constexpr int Profile = 120, Technical = 121, Errors = 122, Unconfirmed = 123, Background = 124,
              Transient = 125, Metadata = 126, Rules = 127;
HWND mainWindow, targetsList, eventList, statusBar, startButton, stopButton, addButton, removeButton,
    childrenBox;
HFONT font;
SessionWorker session;
unsigned long long pendingAction = 0;
bool running = false, busy = false, closing = false, requestPending = false, rulesReady = false;
std::vector<std::wstring> targets;
std::wstring logDir;
ViewOptions options;
std::shared_ptr<const ViewFrame> shown = std::make_shared<ViewFrame>();
struct StartupSettings
{
    std::vector<std::wstring> targets;
    std::vector<Rule> rules;
    std::wstring error;
};
std::future<StartupSettings> startup;
std::wstring Exe()
{
    wchar_t p[32768];
    GetModuleFileNameW(nullptr, p, 32768);
    return p;
}
void SaveTargets()
{
    session.PersistTargets(logDir + L"\\targets.txt", targets);
}
void LoadTargets()
{
    auto dir = logDir;
    startup = std::async(std::launch::async, [dir] {
        StartupSettings result;
        result.rules = DefaultRules();
        std::ifstream in(std::filesystem::path(dir) / L"targets.txt", std::ios::binary);
        std::string line;
        while (std::getline(in, line))
            if (!line.empty())
                result.targets.push_back(Wide(line));
        auto path = dir + L"\\rules.tsv";
        if (std::filesystem::exists(path))
            LoadRules(path, result.rules, result.error);
        else
            SaveRules(path, result.rules, result.error);
        return result;
    });
}
HWND Control(const wchar_t *cls, const wchar_t *label, DWORD style, int id)
{
    auto h = CreateWindowExW(0, cls, label, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, mainWindow,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr),
                             nullptr);
    SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return h;
}
void Layout()
{
    RECT r;
    GetClientRect(mainWindow, &r);
    int w = r.right, h = r.bottom;
    MoveWindow(targetsList, 12, 12, w - 168, 80, TRUE);
    MoveWindow(addButton, w - 144, 12, 132, 28, TRUE);
    MoveWindow(removeButton, w - 144, 48, 132, 28, TRUE);
    MoveWindow(childrenBox, 12, 103, 240, 24, TRUE);
    MoveWindow(startButton, 270, 100, 100, 28, TRUE);
    MoveWindow(stopButton, 380, 100, 100, 28, TRUE);
    MoveWindow(GetDlgItem(mainWindow, Save), w - 144, 100, 132, 28, TRUE);
    MoveWindow(GetDlgItem(mainWindow, Profile), 12, 138, 210, 160, TRUE);
    MoveWindow(GetDlgItem(mainWindow, Rules), 232, 138, 115, 26, TRUE);
    int x = 12;
    for (auto [id, width] : {std::pair{Technical, 135}, std::pair{Errors, 100}, std::pair{Unconfirmed, 130},
                             std::pair{Background, 130}, std::pair{Transient, 115}, std::pair{Metadata, 115}})
    {
        MoveWindow(GetDlgItem(mainWindow, id), x, 176, width, 24, TRUE);
        x += width;
    }
    SendMessageW(statusBar, WM_SIZE, 0, 0);
    RECT s;
    GetWindowRect(statusBar, &s);
    MoveWindow(eventList, 12, 210, w - 24, std::max(40, h - 222 - static_cast<int>(s.bottom - s.top)), TRUE);
}
void ApplyOptions()
{
    options.technical = IsDlgButtonChecked(mainWindow, Technical) == BST_CHECKED;
    options.errors = IsDlgButtonChecked(mainWindow, Errors) == BST_CHECKED;
    options.unconfirmed = IsDlgButtonChecked(mainWindow, Unconfirmed) == BST_CHECKED;
    options.background = IsDlgButtonChecked(mainWindow, Background) == BST_CHECKED;
    options.transient = IsDlgButtonChecked(mainWindow, Transient) == BST_CHECKED;
    options.metadata = IsDlgButtonChecked(mainWindow, Metadata) == BST_CHECKED;
    session.Configure(options);
}
void Edit(const Rule *suggested = nullptr)
{
    if (!rulesReady)
        return;
    if (EditRules(mainWindow, logDir + L"\\rules.tsv", options.rules, suggested))
        ApplyOptions();
}
void UpdateRows()
{
    if (startup.valid() && startup.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        StartupSettings loaded;
        try
        {
            loaded = startup.get();
        }
        catch (const std::exception &e)
        {
            loaded.rules = DefaultRules();
            loaded.error = Wide(e.what());
        }
        targets = std::move(loaded.targets);
        options.rules = std::move(loaded.rules);
        rulesReady = true;
        for (auto &t : targets)
            SendMessageW(targetsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(t.c_str()));
        ApplyOptions();
        if (!loaded.error.empty())
            MessageBoxW(
                mainWindow,
                (L"Rules could not be loaded/saved. Defaults are active; the existing file was kept.\n" +
                 loaded.error)
                    .c_str(),
                L"VoidSentinel settings", MB_OK | MB_ICONWARNING);
    }
    auto next = session.Frame();
    if (next != shown)
    {
        bool changed = next->content != shown->content;
        shown = std::move(next);
        running = shown->running;
        busy = shown->busy;
        if (!busy && shown->completedAction >= pendingAction)
            requestPending = false;
        if (changed)
        {
            ListView_SetItemCountEx(eventList, static_cast<int>(shown->content->display.size()),
                                    LVSICF_NOSCROLL);
            InvalidateRect(eventList, nullptr, FALSE);
        }
    }
    EnableWindow(startButton, rulesReady && !running && !busy && !requestPending && !closing);
    EnableWindow(stopButton, running && !busy && !requestPending && !closing);
    for (auto h : {addButton, removeButton, childrenBox})
        EnableWindow(h, !running && !busy && !requestPending && !closing);
    EnableWindow(GetDlgItem(mainWindow, Rules), rulesReady && !closing);
    EnableWindow(GetDlgItem(mainWindow, Save), !shown->content->data.empty() && !closing);
    const auto &s = shown->content->stats;
    std::wstring text = closing ? L"Closing..." : busy ? L"Working..." : running ? L"Monitoring" : L"Stopped";
    text += L" | Rows: " + std::to_wstring(shown->content->data.size()) + L" | Events: " +
            std::to_wstring(shown->events) + L" | Hidden: rules " + std::to_wstring(s.excluded) +
            L", background " + std::to_wstring(s.background) + L", transient " +
            std::to_wstring(s.transient) + L", errors " + std::to_wstring(s.errors) + L", unconfirmed " +
            std::to_wstring(s.unconfirmed) + L", metadata " + std::to_wstring(s.metadata);
    text += L" | Lost: " + std::to_wstring(shown->lost) + L" | Dropped: " + std::to_wstring(shown->dropped);
    if (shown->traceError)
        text += L" | ETW ERROR " + std::to_wstring(shown->traceError);
    if (!shown->message.empty())
        text += L" | " + shown->message;
    SendMessageW(statusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
    if (closing && !running && !busy && !requestPending)
        DestroyWindow(mainWindow);
}
void AddTarget()
{
    wchar_t path[32768]{};
    OPENFILENAMEW ofn{sizeof(ofn)};
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFilter = L"Applications (*.exe)\0*.exe\0\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = 32768;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn))
    {
        for (auto &t : targets)
            if (!_wcsicmp(t.c_str(), path))
                return;
        targets.emplace_back(path);
        SendMessageW(targetsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(path));
        SaveTargets();
    }
}
void SaveResult()
{
    auto frame = shown;
    if (frame->content->data.empty())
        return;
    wchar_t path[32768] = L"VoidSentinel-results.csv";
    OPENFILENAMEW ofn{sizeof(ofn)};
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFilter = L"CSV files (*.csv)\0*.csv\0\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = 32768;
    ofn.lpstrDefExt = L"csv";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetSaveFileNameW(&ofn))
        session.Export(path, std::move(frame));
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE: {
        mainWindow = hwnd;
        NONCLIENTMETRICSW metrics{sizeof(metrics)};
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
        font = CreateFontIndirectW(&metrics.lfMessageFont);
        targetsList = Control(
            L"LISTBOX", L"",
            WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_TABSTOP, Targets);
        addButton = Control(L"BUTTON", L"+ Browse...", BS_PUSHBUTTON | WS_TABSTOP, Add);
        removeButton = Control(L"BUTTON", L"Remove", BS_PUSHBUTTON | WS_TABSTOP, Remove);
        childrenBox = Control(L"BUTTON", L"Include child processes", BS_AUTOCHECKBOX | WS_TABSTOP, Children);
        CheckDlgButton(hwnd, Children, BST_CHECKED);
        startButton = Control(L"BUTTON", L"Start", BS_PUSHBUTTON | WS_TABSTOP, Start);
        stopButton = Control(L"BUTTON", L"Stop", BS_PUSHBUTTON | WS_TABSTOP, Stop);
        Control(L"BUTTON", L"Save result...", BS_PUSHBUTTON | WS_TABSTOP, Save);
        auto profile = Control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, Profile);
        SendMessageW(profile, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Meaningful changes"));
        SendMessageW(profile, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Full application footprint"));
        SendMessageW(profile, CB_SETCURSEL, 0, 0);
        Control(L"BUTTON", L"Filter rules...", BS_PUSHBUTTON | WS_TABSTOP, Rules);
        for (auto [id, label] : {std::pair{Technical, L"Technical rows"}, std::pair{Errors, L"Errors"},
                                 std::pair{Unconfirmed, L"Unconfirmed"}, std::pair{Background, L"Background"},
                                 std::pair{Transient, L"Transient"}, std::pair{Metadata, L"Metadata"}})
            Control(L"BUTTON", label, BS_AUTOCHECKBOX | WS_TABSTOP, id);
        eventList = Control(WC_LISTVIEWW, L"",
                            WS_BORDER | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | WS_TABSTOP, 108);
        ListView_SetExtendedListViewStyle(eventList,
                                          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        const wchar_t *columns[] = {
            L"Application", L"Type",      L"Path",   L"Change / operation", L"Count",
            L"First seen",  L"Last seen", L"Result", L"Process chain",      L"Rename destination"};
        int widths[] = {190, 75, 350, 235, 65, 175, 175, 130, 350, 350};
        for (int i = 0; i < 10; ++i)
        {
            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = const_cast<wchar_t *>(columns[i]);
            col.cx = widths[i];
            ListView_InsertColumn(eventList, i, &col);
        }
        statusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
                                    nullptr, GetModuleHandleW(nullptr), nullptr);
        LoadTargets();
        SetTimer(hwnd, 1, 200, nullptr);
        Layout();
        UpdateRows();
        return 0;
    }
    case WM_SIZE:
        Layout();
        return 0;
    case WM_GETMINMAXINFO: {
        auto m = reinterpret_cast<MINMAXINFO *>(lp);
        m->ptMinTrackSize = {900, 500};
        return 0;
    }
    case WM_TIMER:
        UpdateRows();
        return 0;
    case WM_NOTIFY: {
        auto n = reinterpret_cast<NMHDR *>(lp);
        if (n->hwndFrom == eventList && n->code == LVN_GETDISPINFOW)
        {
            auto d = reinterpret_cast<NMLVDISPINFOW *>(lp);
            const auto &rows = shown->content->display;
            if (d->item.iItem >= 0 && static_cast<size_t>(d->item.iItem) < rows.size() &&
                (d->item.mask & LVIF_TEXT))
            {
                const auto &r = rows[d->item.iItem];
                std::wstring s;
                if (r.operation.empty())
                {
                    if (!d->item.iSubItem)
                        s = r.app;
                }
                else
                    switch (d->item.iSubItem)
                    {
                    case 0:
                        s = std::filesystem::path(r.app).filename().wstring();
                        break;
                    case 1:
                        s = KindName(r.kind);
                        break;
                    case 2:
                        s = r.path;
                        break;
                    case 3:
                        s = r.operation;
                        break;
                    case 4:
                        s = std::to_wstring(r.count);
                        break;
                    case 5:
                        s = r.time;
                        break;
                    case 6:
                        s = r.lastTime;
                        break;
                    case 7:
                        s = r.result;
                        break;
                    case 8:
                        s = r.lineage;
                        break;
                    case 9:
                        s = r.destination;
                        break;
                    }
                wcsncpy_s(d->item.pszText, d->item.cchTextMax, s.c_str(), _TRUNCATE);
            }
            return 0;
        }
        if (n->hwndFrom == eventList && n->code == NM_DBLCLK)
        {
            int index = ListView_GetNextItem(eventList, -1, LVNI_SELECTED);
            if (index >= 0 && static_cast<size_t>(index) < shown->content->display.size())
            {
                auto &r = shown->content->display[index];
                auto text = r.path + L"\n" + r.operation + L"\n\nProcess chain:\n" + r.lineage +
                            L"\n\nDestination:\n" + r.destination + L"\n\nSuccesses: " +
                            std::to_wstring(r.successes) + L"\nFailures: " + std::to_wstring(r.failures) +
                            L"\nUnconfirmed: " + std::to_wstring(r.unconfirmed);
                MessageBoxW(hwnd, text.c_str(), L"Result details", MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        }
        break;
    }
    case WM_CONTEXTMENU:
        if (reinterpret_cast<HWND>(wp) == eventList)
        {
            int i = ListView_GetNextItem(eventList, -1, LVNI_SELECTED);
            if (lp != -1)
            {
                POINT point{static_cast<short>(LOWORD(lp)), static_cast<short>(HIWORD(lp))};
                ScreenToClient(eventList, &point);
                LVHITTESTINFO hit{};
                hit.pt = point;
                i = ListView_SubItemHitTest(eventList, &hit);
                if (i >= 0)
                {
                    ListView_SetItemState(eventList, -1, 0, LVIS_SELECTED);
                    ListView_SetItemState(eventList, i, LVIS_SELECTED | LVIS_FOCUSED,
                                          LVIS_SELECTED | LVIS_FOCUSED);
                }
            }
            if (i < 0 || static_cast<size_t>(i) >= shown->content->display.size())
                return 0;
            auto row = shown->content->display[i];
            if (row.path.empty() || row.path[0] == L'[')
                return 0;
            auto menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, 1, L"Exclude this folder...");
            AppendMenuW(menu, MF_STRING, 2, L"Always include this folder...");
            POINT p;
            GetCursorPos(&p);
            int action = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, p.x, p.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            if (action)
            {
                Rule rule;
                rule.action = action == 1 ? RuleAction::Exclude : RuleAction::Include;
                rule.pattern = row.kind == ItemKind::Directory
                                   ? row.path
                                   : std::filesystem::path(row.path).parent_path().wstring();
                rule.pattern = PortablePath(rule.pattern);
                Edit(&rule);
            }
            return 0;
        }
        break;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= Technical && id <= Metadata)
        {
            ApplyOptions();
            return 0;
        }
        switch (id)
        {
        case Profile:
            if (HIWORD(wp) == CBN_SELCHANGE)
            {
                bool full = SendDlgItemMessageW(hwnd, Profile, CB_GETCURSEL, 0, 0) == 1;
                for (int control : {Technical, Errors, Unconfirmed, Background, Transient, Metadata})
                    CheckDlgButton(hwnd, control, full ? BST_CHECKED : BST_UNCHECKED);
                ApplyOptions();
            }
            return 0;
        case Rules:
            Edit();
            return 0;
        case Add:
            if (!running && !busy)
                AddTarget();
            return 0;
        case Remove:
            if (!running && !busy)
            {
                auto i = SendMessageW(targetsList, LB_GETCURSEL, 0, 0);
                if (i != LB_ERR)
                {
                    targets.erase(targets.begin() + i);
                    SendMessageW(targetsList, LB_DELETESTRING, i, 0);
                    SaveTargets();
                }
            }
            return 0;
        case Save:
            SaveResult();
            return 0;
        case Start:
            if (!running && !busy && !requestPending && rulesReady)
            {
                if (targets.empty())
                    MessageBoxW(hwnd, L"Add at least one application first.", L"VoidSentinel",
                                MB_OK | MB_ICONINFORMATION);
                else
                {
                    pendingAction = session.Start(targets, IsDlgButtonChecked(hwnd, Children) == BST_CHECKED);
                    requestPending = true;
                    EnableWindow(startButton, FALSE);
                }
            }
            return 0;
        case Stop:
            if (running && !requestPending)
            {
                pendingAction = session.Stop();
                requestPending = true;
                EnableWindow(stopButton, FALSE);
            }
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        if (!closing)
        {
            closing = true;
            requestPending = true;
            pendingAction = session.Stop();
        }
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        DeleteObject(font);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
