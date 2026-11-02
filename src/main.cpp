#include "monitor.h"
#include "resource.h"
#include <algorithm>
#include <commctrl.h>
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include <shlobj.h>

namespace
{
constexpr int Add = 101, Remove = 102, Start = 103, Stop = 104, Save = 105, Children = 106, Targets = 107;
HWND mainWindow, targetsList, eventList, statusBar, startButton, stopButton, addButton, removeButton,
    childrenBox;
HFONT font;
Monitor monitor;
bool running = false;
std::vector<std::wstring> targets;
std::wstring logDir;
std::vector<Row> visible;
std::wstring Exe()
{
    wchar_t p[32768];
    GetModuleFileNameW(nullptr, p, 32768);
    return p;
}
void SaveTargets()
{
    std::ofstream out(std::filesystem::path(logDir) / L"targets.txt", std::ios::binary);
    for (auto &p : targets)
        out << Utf8(p) << '\n';
}
void LoadTargets()
{
    std::ifstream in(std::filesystem::path(logDir) / L"targets.txt", std::ios::binary);
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty())
        {
            auto p = Wide(line);
            targets.push_back(p);
            SendMessageW(targetsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(p.c_str()));
        }
    }
}
HWND Control(const wchar_t *cls, const wchar_t *label, DWORD style, int id)
{
    HWND h = CreateWindowExW(0, cls, label, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, mainWindow,
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
    MoveWindow(targetsList, 12, 12, w - 168, 94, TRUE);
    MoveWindow(addButton, w - 144, 12, 132, 28, TRUE);
    MoveWindow(removeButton, w - 144, 48, 132, 28, TRUE);
    MoveWindow(childrenBox, 12, 114, 270, 24, TRUE);
    MoveWindow(startButton, 300, 112, 110, 28, TRUE);
    MoveWindow(stopButton, 420, 112, 110, 28, TRUE);
    MoveWindow(GetDlgItem(mainWindow, Save), w - 144, 112, 132, 28, TRUE);
    SendMessageW(statusBar, WM_SIZE, 0, 0);
    RECT s;
    GetWindowRect(statusBar, &s);
    MoveWindow(eventList, 12, 150, w - 24, std::max(50, h - 162 - static_cast<int>(s.bottom - s.top)), TRUE);
}
void UpdateStatus()
{
    monitor.RefreshStats();
    auto text = std::wstring(running ? L"Monitoring" : L"Stopped") + L" | Events: " +
                std::to_wstring(monitor.recorded.load()) + L" | Lost ETW: " +
                std::to_wstring(monitor.lost.load()) + L" | Dropped: " +
                std::to_wstring(monitor.dropped.load()) + L" | Unresolved paths: " +
                std::to_wstring(monitor.unresolved.load()) + L" | Unmatched: " +
                std::to_wstring(monitor.unmatched.load());

    if (monitor.traceError)
        text += L" | ETW ERROR " + std::to_wstring(monitor.traceError.load());
    SendMessageW(statusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
}
void UpdateRows()
{
    static unsigned long long lastCount = ~0ULL;
    auto count = monitor.recorded.load();
    if (count != lastCount)
    {
        auto rows = monitor.Snapshot();
        visible.clear();
        std::wstring previous;
        for (auto &row : rows)
        {
            if (_wcsicmp(previous.c_str(), row.app.c_str()) != 0)
            {
                Row header;
                header.app = row.app;
                visible.push_back(std::move(header));
                previous = row.app;
            }
            visible.push_back(std::move(row));
        }
        lastCount = count;
        ListView_SetItemCountEx(eventList, static_cast<int>(visible.size()), LVSICF_NOSCROLL);
        InvalidateRect(eventList, nullptr, FALSE);
    }
    UpdateStatus();
}
void StopMonitoring()
{
    if (running)
    {
        monitor.Stop();
        running = false;
        EnableWindow(startButton, TRUE);
        EnableWindow(stopButton, FALSE);
        EnableWindow(addButton, TRUE);
        EnableWindow(removeButton, TRUE);
        EnableWindow(childrenBox, TRUE);
        UpdateRows();
    }
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
            if (_wcsicmp(t.c_str(), path) == 0)
                return;
        targets.emplace_back(path);
        SendMessageW(targetsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(path));
        SaveTargets();
    }
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
        SendMessageW(childrenBox, BM_SETCHECK, BST_CHECKED, 0);
        startButton = Control(L"BUTTON", L"Start", BS_PUSHBUTTON | WS_TABSTOP, Start);
        stopButton = Control(L"BUTTON", L"Stop", BS_PUSHBUTTON | WS_TABSTOP, Stop);
        EnableWindow(stopButton, FALSE);
        Control(L"BUTTON", L"Save result...", BS_PUSHBUTTON | WS_TABSTOP, Save);
        eventList = Control(WC_LISTVIEWW, L"",
                            WS_BORDER | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | WS_TABSTOP, 108);
        ListView_SetExtendedListViewStyle(eventList,
                                          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        const wchar_t *columns[] = {L"Application", L"Type",       L"Path",      L"Operation",
                                    L"Count",       L"First seen", L"Last seen", L"Result"};
        int widths[] = {220, 75, 430, 170, 70, 175, 175, 150};
        for (int i = 0; i < 8; ++i)
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
        SetTimer(hwnd, 1, 500, nullptr);
        Layout();
        return 0;
    }
    case WM_SIZE:
        Layout();
        return 0;
    case WM_GETMINMAXINFO: {
        auto m = reinterpret_cast<MINMAXINFO *>(lp);
        m->ptMinTrackSize = {760, 400};
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
            if (d->item.iItem >= 0 && static_cast<size_t>(d->item.iItem) < visible.size() &&
                (d->item.mask & LVIF_TEXT))
            {
                auto &row = visible[d->item.iItem];
                std::wstring s;
                if (row.operation.empty())
                {
                    if (d->item.iSubItem == 0)
                        s = row.app;
                }
                else
                    switch (d->item.iSubItem)
                    {
                    case 0:
                        s = std::filesystem::path(row.app).filename().wstring();
                        break;
                    case 1:
                        s = KindName(row.kind);
                        break;
                    case 2:
                        s = row.path;
                        break;
                    case 3:
                        s = row.operation;
                        break;
                    case 4:
                        s = std::to_wstring(row.count);
                        break;
                    case 5:
                        s = row.time;
                        break;
                    case 6:
                        s = row.lastTime;
                        break;
                    case 7:
                        s = row.result;
                        break;
                    }
                wcsncpy_s(d->item.pszText, d->item.cchTextMax, s.c_str(), _TRUNCATE);
            }
            return 0;
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case Add:
            if (!running)
                AddTarget();
            return 0;
        case Remove:
            if (!running)
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
        case Save: {
            UpdateRows();
            auto snapshot = monitor.Snapshot();
            if (snapshot.empty())
            {
                MessageBoxW(hwnd, L"There are no results to save.", L"VoidSentinel",
                            MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            wchar_t path[32768] = L"VoidSentinel-results.csv";
            OPENFILENAMEW ofn{sizeof(ofn)};
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"CSV files (*.csv)\0*.csv\0\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = 32768;
            ofn.lpstrDefExt = L"csv";
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetSaveFileNameW(&ofn) && !SaveCsv(path, snapshot))
                MessageBoxW(hwnd, L"The CSV could not be saved.", L"VoidSentinel", MB_OK | MB_ICONERROR);
            return 0;
        }
        case Start: {
            if (running)
                return 0;
            if (targets.empty())
            {
                MessageBoxW(hwnd, L"Add at least one application first.", L"VoidSentinel",
                            MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            std::wstring error;
            if (!monitor.Start(targets, SendMessageW(childrenBox, BM_GETCHECK, 0, 0) == BST_CHECKED, error))
            {
                MessageBoxW(hwnd, error.c_str(), L"Monitoring could not start", MB_OK | MB_ICONERROR);
                return 0;
            }
            visible.clear();
            ListView_SetItemCount(eventList, 0);
            running = true;
            EnableWindow(startButton, FALSE);
            EnableWindow(stopButton, TRUE);
            EnableWindow(addButton, FALSE);
            EnableWindow(removeButton, FALSE);
            EnableWindow(childrenBox, FALSE);
            UpdateStatus();
            return 0;
        }
        case Stop:
            StopMonitoring();
            return 0;
        }
        break;
    case WM_CLOSE:
        StopMonitoring();
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        DeleteObject(font);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
int Probe(const std::wstring &directory)
{
    std::filesystem::create_directories(directory);
    auto a = std::filesystem::path(directory) / L"árvíztűrő-test.txt",
         b = std::filesystem::path(directory) / L"renamed.txt";
    HANDLE h = CreateFileW(a.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return 2;
    DWORD written;
    WriteFile(h, "VoidSentinel probe\n", 19, &written, nullptr);
    SetEndOfFile(h);
    CloseHandle(h);
    if (!MoveFileExW(a.c_str(), b.c_str(), MOVEFILE_REPLACE_EXISTING))
        return 3;
    if (!DeleteFileW(b.c_str()))
        return 4;
    RemoveDirectoryW(directory.c_str());
    return 0;
}
DWORD RunProbe(const std::wstring &executable, const std::wstring &mode, const std::wstring &directory)
{
    auto command = L"\"" + executable + L"\" " + mode + L" \"" + directory + L"\"";
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                        &si, &pi))
        return 100;
    auto waited = WaitForSingleObject(pi.hProcess, 30000);
    DWORD result = 101;
    if (waited == WAIT_OBJECT_0)
        GetExitCodeProcess(pi.hProcess, &result);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return result;
}
int ProbeParent(const std::wstring &directory)
{
    auto root = Probe(directory + L"\\root");
    if (root)
        return root;
    auto helper = directory + L"\\child-helper.exe";
    if (!CopyFileW(Exe().c_str(), helper.c_str(), FALSE))
        return 5;
    return static_cast<int>(RunProbe(helper, L"--probe", directory + L"\\child"));
}
int ProbeStress(const std::wstring &directory)
{
    std::filesystem::create_directories(directory);
    auto path = std::filesystem::path(directory) / L"stress.bin";
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return 2;
    char buffer[64]{};
    bool ok = true;
    for (int i = 0; i < 10000; ++i)
    {
        DWORD written = 0;
        if (!WriteFile(h, buffer, sizeof(buffer), &written, nullptr) || written != sizeof(buffer))
        {
            ok = false;
            break;
        }
    }
    FlushFileBuffers(h);
    CloseHandle(h);
    return ok ? 0 : 3;
}
int CaptureTest(const std::wstring &directory)
{
    std::filesystem::create_directories(directory);
    Monitor test;
    std::wstring error;
    if (!test.Start({Exe()}, true, error))
    {
        std::ofstream(std::filesystem::path(directory) / L"error.txt") << Utf8(error);
        return 10;
    }
    // The monitoring process is always excluded; the probe is a separate instance.
    auto helper = directory + L"\\unrelated-helper.exe";
    if (!CopyFileW(Exe().c_str(), helper.c_str(), FALSE))
    {
        test.Stop();
        return 11;
    }
    Sleep(1000);
    auto exitCode = RunProbe(Exe(), L"--probe-parent", directory + L"\\probe");
    auto unrelated = RunProbe(helper, L"--probe", directory + L"\\unrelated");
    Sleep(2000);
    test.Stop();
    SaveCsv(directory + L"\\capture.csv", test.Snapshot());
    std::ofstream report(std::filesystem::path(directory) / L"report.txt");
    report << "probe_exit=" << exitCode << "\nrecorded=" << test.recorded << "\nlost=" << test.lost
           << "\ndropped=" << test.dropped << "\nunresolved=" << test.unresolved
           << "\nunmatched=" << test.unmatched << "\n";
    bool firstOk = exitCode == 0 && unrelated == 0 && test.recorded > 0;
    if (!test.Start({Exe()}, false, error))
        return 13;
    Sleep(1000);
    auto second = RunProbe(Exe(), L"--probe-parent", directory + L"\\second");
    Sleep(2000);
    test.Stop();
    SaveCsv(directory + L"\\no-children.csv", test.Snapshot());
    report << "second_probe_exit=" << second << "\nsecond_recorded=" << test.recorded
           << "\nsecond_lost=" << test.lost << "\nsecond_dropped=" << test.dropped << "\n";
    bool secondOk = second == 0 && test.recorded > 0;
    if (!test.Start({Exe(), helper}, false, error))
        return 14;
    Sleep(1000);
    auto third = RunProbe(Exe(), L"--probe-stress", directory + L"\\stress");
    auto extra = RunProbe(helper, L"--probe", directory + L"\\explicit-target");
    Sleep(2000);
    test.Stop();
    SaveCsv(directory + L"\\stress.csv", test.Snapshot());
    report << "stress_exit=" << third << "\nextra_target_exit=" << extra
           << "\nstress_recorded=" << test.recorded << "\nstress_lost=" << test.lost
           << "\nstress_dropped=" << test.dropped << "\n";
    return firstOk && secondOk && third == 0 && extra == 0 && test.recorded >= 10000 ? 0 : 12;
}
} // namespace
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int show)
{
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 3)
    {
        std::wstring arg = argv[1], dir = argv[2];
        LocalFree(argv);
        if (arg == L"--probe")
            return Probe(dir);
        if (arg == L"--probe-parent")
            return ProbeParent(dir);
        if (arg == L"--probe-stress")
            return ProbeStress(dir);
        if (arg == L"--capture-test")
            return CaptureTest(dir);
    }
    else
        LocalFree(argv);
    PWSTR local = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local)))
        return 1;
    logDir = std::wstring(local) + L"\\VoidSentinel";
    CoTaskMemFree(local);
    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);
    if (ec)
    {
        MessageBoxW(nullptr, L"Cannot create log folder.", L"VoidSentinel", MB_OK | MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&controls);
    WNDCLASSEXW cls{sizeof(WNDCLASSEXW)};
    cls.lpfnWndProc = WindowProc;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_VOID_SENTINEL), IMAGE_ICON,
                                            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
    cls.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_VOID_SENTINEL), IMAGE_ICON,
                                              GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    cls.lpszClassName = L"VoidSentinelWindow";
    RegisterClassExW(&cls);
    auto window = CreateWindowExW(0, cls.lpszClassName, L"VoidSentinel", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 1200, 650, nullptr, nullptr, instance, nullptr);
    if (!window)
        return 1;
    ShowWindow(window, show);
    UpdateWindow(window);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageW(window, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}
