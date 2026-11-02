#include "monitor.h"
#include "resource.h"
#include "session.h"
#include "rules_ui.h"
#include <future>
#include <chrono>
#include <algorithm>
#include <commctrl.h>
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include <shlobj.h>

namespace
{
#include "ui.inc"
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
    auto shared = std::filesystem::path(directory).parent_path() / L"shared.txt";
    HANDLE sharedFile = CreateFileW(shared.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (sharedFile != INVALID_HANDLE_VALUE)
    {
        WriteFile(sharedFile, "child", 5, &written, nullptr);
        CloseHandle(sharedFile);
    }
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
    {
        std::ofstream shared(std::filesystem::path(directory) / L"shared.txt");
        shared << "parent";
    }
    auto temp = directory + L"\\atomic-save.tmp", final = directory + L"\\saved-document.txt";
    {
        std::ofstream file(std::filesystem::path(temp), std::ios::binary);
        file << "saved";
    }
    if (!MoveFileExW(temp.c_str(), final.c_str(), MOVEFILE_REPLACE_EXISTING))
        return 6;
    auto transient = directory + L"\\ephemeral.tmp";
    {
        std::ofstream file{std::filesystem::path(transient)};
        file << "temporary";
    }
    if (!DeleteFileW(transient.c_str()))
        return 7;
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
    {
        ViewOptions view;
        view.rules = DefaultRules();
        SaveCsv(directory + L"\\meaningful.csv", BuildView(test.RawSnapshot(), view).rows);
        view.technical = true;
        view.errors = true;
        view.unconfirmed = true;
        view.background = true;
        view.transient = true;
        view.metadata = true;
        SaveCsv(directory + L"\\full.csv", BuildView(test.RawSnapshot(), view).rows);
    }
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
    {
        SessionWorker worker;
        ViewOptions view;
        view.rules = DefaultRules();
        worker.Configure(view);
        auto action = worker.Start({Exe()}, true);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (worker.Frame()->completedAction < action && std::chrono::steady_clock::now() < deadline)
            Sleep(10);
        if (!worker.Frame()->running)
            return 15;
        auto probe = RunProbe(Exe(), L"--probe-parent", directory + L"\\worker-probe");
        action = worker.Stop();
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (worker.Frame()->completedAction < action && std::chrono::steady_clock::now() < deadline)
            Sleep(10);
        auto frame = worker.Frame();
        if (frame->completedAction < action || frame->busy || frame->running || probe != 0 ||
            frame->content->data.empty() || frame->lost || frame->dropped)
            return 16;
        if (!SaveCsv(directory + L"\\worker-meaningful.csv", frame->content->data))
            return 17;
        report << "worker_events=" << frame->events << "\nworker_rows=" << frame->content->data.size()
               << "\nworker_lost=" << frame->lost << "\nworker_dropped=" << frame->dropped << "\n";
    }
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
#ifdef VOIDSENTINEL_UI_TEST
    // UI tests use isolated settings and normal user privileges. ETW capture is tested separately.
    logDir = (std::filesystem::path(Exe()).parent_path() / L"ui-test-state").wstring();
#endif
    CoTaskMemFree(local);
    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);
    if (ec)
    {
        MessageBoxW(nullptr, L"Cannot create the settings folder.", L"VoidSentinel", MB_OK | MB_ICONERROR);
        return 1;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&controls);
    WNDCLASSEXW cls{sizeof(WNDCLASSEXW)};
    cls.lpfnWndProc = WindowProc;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hIcon =
        static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_VOID_SENTINEL), IMAGE_ICON,
                                      GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
    cls.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_VOID_SENTINEL), IMAGE_ICON,
                                                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                                LR_SHARED));
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
