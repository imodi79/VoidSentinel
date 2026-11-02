#include "filter.h"
#include "monitor.h"
#include "session.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#define CHECK(x)                                                                                             \
    do                                                                                                       \
    {                                                                                                        \
        if (!(x))                                                                                            \
            throw std::runtime_error(#x);                                                                    \
    } while (false)

int main()
{
    try
    {
        Rule exclude{true, RuleAction::Exclude, RuleMatch::Folder, L"C:\\Cache", L"", L"Cache"};
        CHECK(RuleMatches(exclude, L"c:/CACHE/x.txt", L"C:\\A.exe"));
        CHECK(!RuleMatches(exclude, L"C:\\CacheOther\\x", L"C:\\A.exe"));
        CHECK(RuleMatches(exclude, L"\\\\?\\C:\\Cache\\sub\\..\\x", L"C:\\A.exe"));
        CHECK(RuleMatches(exclude, L"C:\\Cache", L"C:\\A.exe"));
        auto disabled = exclude;
        disabled.enabled = false;
        CHECK(!RuleMatches(disabled, L"C:\\Cache\\x", L"A.exe"));
        Rule glob{true,
                  RuleAction::Background,
                  RuleMatch::Wildcard,
                  L"%LOCALAPPDATA%\\Packages\\Example_*\\Cache\\*",
                  L"helper?.exe",
                  L"Portable"};
        wchar_t base[32768];
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%", base, 32768);
        CHECK(
            RuleMatches(glob, std::wstring(base) + L"\\Packages\\Example_123\\Cache\\x", L"C:\\helper1.exe"));
        CHECK(!RuleMatches(glob, std::wstring(base) + L"\\Packages\\Example_123\\Settings\\x",
                           L"C:\\helper1.exe"));
        CHECK(
            !RuleMatches(glob, std::wstring(base) + L"\\Packages\\Example_123\\Cache\\x", L"C:\\other.exe"));
        ResultSet data;
        unsigned long long seq = 0;
        auto add = [&](const wchar_t *path, const wchar_t *op, const wchar_t *result = L"Success",
                       const wchar_t *app = L"C:\\helper.exe", const wchar_t *dest = L"") {
            Row row{L"2026-11-02 12:00:00.000", app, op, path, result, 42};
            row.rootApp = L"C:\\Parent.exe";
            row.lineage = row.rootApp + L" -> " + app;
            row.kind = ItemKind::File;
            row.destination = dest;
            row.sequence = ++seq;
            data.Add(row);
        };
        add(L"D:\\Documents\\ac", L"Create", L"Success", L"C:\\Parent.exe");
        for (int i = 0; i < 10000; ++i)
            add(L"D:\\Documents\\ac", L"Write");
        add(L"D:\\Documents\\ac", L"Write", L"Failed: 0xC0000022");
        add(L"D:\\Documents\\ac", L"End of file");
        add(L"D:\\Documents\\ac", L"Allocation size");
        add(L"D:\\Documents\\ac", L"Attributes / timestamps");
        add(L"C:\\Cache\\state", L"Write");
        add(L"D:\\Documents\\temp.tmp", L"Create");
        add(L"D:\\Documents\\temp.tmp", L"Write");
        add(L"D:\\Documents\\temp.tmp", L"Rename request", L"Success", L"C:\\helper.exe",
            L"D:\\Documents\\final.txt");
        add(L"D:\\Documents\\gone.tmp", L"Create");
        add(L"D:\\Documents\\gone.tmp", L"Delete request");
        add(L"D:\\Documents\\denied", L"Create", L"Failed: 0xC0000022");
        add(L"D:\\Documents\\unknown", L"Write", L"Completion unavailable");
        ViewOptions options;
        options.rules = {exclude};
        auto view = BuildView(data.Copy(), options);
        CHECK(view.rows.size() == 2);
        CHECK(view.excluded == 1);
        CHECK(view.errors == 2);
        CHECK(view.unconfirmed == 1);
        CHECK(view.transient == 2);
        CHECK(view.metadata == 1);
        CHECK(view.rows[0].path == L"D:\\Documents\\ac");
        CHECK(view.rows[0].count == 10003);
        CHECK(view.rows[0].result == L"Success");
        CHECK(view.rows[0].app == L"C:\\Parent.exe");
        CHECK(view.rows[0].lineage.find(L"helper.exe") != std::wstring::npos);
        CHECK(view.rows[1].path == L"D:\\Documents\\final.txt");
        CHECK(view.rows[1].operation.find(L"Saved via rename") != std::wstring::npos);
        CHECK(!view.rows[1].originalPath.empty());
        auto bg = exclude;
        bg.action = RuleAction::Background;
        options.rules = {bg};
        CHECK(BuildView(data.Copy(), options).background == 1);
        options.background = true;
        CHECK(BuildView(data.Copy(), options).rows.size() == 3);
        auto include = exclude;
        include.action = RuleAction::Include;
        options.rules = {exclude, include};
        options.background = false;
        CHECK(BuildView(data.Copy(), options).rows.size() == 3);
        options.errors = true;
        options.unconfirmed = true;
        options.transient = true;
        options.metadata = true;
        options.technical = true;
        view = BuildView(data.Copy(), options);
        CHECK(view.errors == 0 && view.unconfirmed == 0 && view.transient == 0 && view.metadata == 0);
        CHECK(view.rows.size() == data.Copy().size());
        // Recreating a path after deleting it must not classify the new object as transient.
        add(L"D:\\Documents\\gone.tmp", L"Create");
        options = {};
        CHECK(BuildView(data.Copy(), options).transient == 0);
        // A delete with no observed creation must remain visible.
        add(L"D:\\Documents\\existing.txt", L"Delete request");
        CHECK(BuildView(data.Copy(), options).transient == 0);
        auto folder = std::filesystem::current_path() / L"filter-test-fixtures";
        std::filesystem::create_directories(folder);
        auto path = (folder / L"rules.tsv").wstring();
        std::wstring error;
        std::vector<Rule> loaded;
        std::vector<Rule> original = {exclude, include, glob, disabled};
        CHECK(SaveRules(path, original, error));
        CHECK(LoadRules(path, loaded, error));
        CHECK(loaded.size() == 4);
        auto locked = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  OPEN_EXISTING, 0, nullptr);
        CHECK(locked != INVALID_HANDLE_VALUE);
        CHECK(!SaveRules(path, {include}, error));
        CloseHandle(locked);
        CHECK(LoadRules(path, loaded, error));
        CHECK(loaded.size() == 4);
        auto lockedCsv = (folder / L"locked.csv").wstring();
        {
            std::ofstream originalFile{std::filesystem::path(lockedCsv)};
            originalFile << "original";
        }
        locked = CreateFileW(lockedCsv.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                             OPEN_EXISTING, 0, nullptr);
        CHECK(locked != INVALID_HANDLE_VALUE);
        CHECK(!SaveCsv(lockedCsv, data.Sorted()));
        CloseHandle(locked);
        {
            std::ifstream originalFile{std::filesystem::path(lockedCsv)};
            std::string text;
            std::getline(originalFile, text);
            CHECK(text == "original");
        }
        {
            std::ofstream bad(folder / L"invalid-utf8.tsv", std::ios::binary);
            bad << "VoidSentinel-Rules\t1\n" << '\xFF';
        }
        CHECK(!LoadRules((folder / L"invalid-utf8.tsv").wstring(), loaded, error));
        CHECK(loaded.size() == 4);
        CHECK(loaded[2].pattern == glob.pattern);
        CHECK(!loaded[3].enabled);
        {
            std::ofstream bad(folder / L"invalid.tsv");
            bad << "VoidSentinel-Rules\t2\n";
        }
        CHECK(!LoadRules((folder / L"invalid.tsv").wstring(), loaded, error));
        CHECK(loaded.size() == 4);
        {
            std::ofstream bad(folder / L"invalid.tsv");
            bad << "VoidSentinel-Rules\t1\n1\texclude\tfolder\tC:\\Cache\t\textra\tINVALID\n";
        }
        CHECK(!LoadRules((folder / L"invalid.tsv").wstring(), loaded, error));
        CHECK(loaded.size() == 4);
        auto invalid = exclude;
        invalid.pattern = L"relative";
        CHECK(!SaveRules(path, {invalid}, error));
        CHECK(LoadRules(path, loaded, error));
        CHECK(loaded.size() == 4);
        for (auto &rule : DefaultRules())
            CHECK(rule.pattern != L"%LOCALAPPDATA%" && rule.pattern != L"%APPDATA%" &&
                  rule.pattern != L"%PROGRAMDATA%" && rule.pattern != L"%WINDIR%");
        CHECK(PortablePath(std::wstring(base) + L"\\Example").starts_with(L"%LOCALAPPDATA%"));
        // Excluding a temporary source must not hide a successful save to another folder.
        ResultSet renameData;
        Row from{L"time", L"C:\\App.exe", L"Create", L"C:\\Temp\\work.tmp", L"Success", 1};
        from.sequence = 1;
        renameData.Add(from);
        from.operation = L"Rename request";
        from.destination = L"D:\\Documents\\saved.txt";
        from.sequence = 2;
        renameData.Add(from);
        ViewOptions renameOptions;
        renameOptions.rules = {{true, RuleAction::Exclude, RuleMatch::Folder, L"C:\\Temp", L"", L""}};
        CHECK(BuildView(renameData.Copy(), renameOptions).rows.size() == 1);
        CHECK(BuildView(renameData.Copy(), renameOptions).rows[0].path == L"D:\\Documents\\saved.txt");
        // Exercise indexed rename/filter processing on a substantial set of distinct paths.
        std::vector<Row> large;
        large.reserve(20000);
        for (int i = 0; i < 10000; ++i)
        {
            auto row = from;
            row.path = L"C:\\Temp\\" + std::to_wstring(i) + L".tmp";
            row.destination = L"D:\\Documents\\" + std::to_wstring(i) + L".txt";
            row.operation = L"Rename request";
            row.successes = 1;
            row.count = 1;
            row.sequence = 2 * i + 2;
            row.lastSequence = row.sequence;
            large.push_back(row);
            row.operation = L"Write";
            row.destination.clear();
            row.sequence--;
            row.lastSequence = row.sequence;
            large.push_back(row);
        }
        auto started = std::chrono::steady_clock::now();
        auto largeView = BuildView(large, renameOptions);
        CHECK(largeView.rows.size() == 10000);
        std::cout << "20000 records / 10000 renames: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                           started)
                         .count()
                  << " ms\n";
        // Configuration and snapshot reads must remain usable concurrently with background work.
        {
            SessionWorker worker;
            for (int i = 0; i < 50; ++i)
            {
                worker.Configure(options);
                auto frame = worker.Frame();
                CHECK(frame != nullptr);
            }
            auto action = worker.Stop();
            auto end = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (worker.Frame()->completedAction < action && std::chrono::steady_clock::now() < end)
                Sleep(10);
            CHECK(worker.Frame()->completedAction >= action);
            auto content = std::make_shared<ViewContent>();
            content->data = std::move(largeView.rows);
            auto frame = std::make_shared<ViewFrame>();
            frame->content = content;
            worker.Export((folder / L"async-results.csv").wstring(), frame);
            auto readsStart = std::chrono::steady_clock::now();
            for (int i = 0; i < 1000; ++i)
                CHECK(worker.Frame() != nullptr);
            CHECK(std::chrono::steady_clock::now() - readsStart < std::chrono::seconds(1));
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (worker.Frame()->message != L"Results saved." &&
                   std::chrono::steady_clock::now() < deadline)
                Sleep(10);
            CHECK(worker.Frame()->message == L"Results saved.");
        }
        std::cout
            << "PASS: semantic summaries, 10000 writes, mixed outcomes, rename chains, transient lifecycle, "
               "rules, boundaries, portable persistence, invalid import isolation, background worker.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
