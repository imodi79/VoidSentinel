#include "results.h"
#include <iostream>

int main()
{
    ResultSet results;
    auto add = [&](const wchar_t *app, const wchar_t *path, const wchar_t *op, ItemKind kind,
                   const wchar_t *status = L"Success") {
        Row row{L"2026-11-02 12:00:00.000", app, op, path, status, 42};
        row.kind = kind;
        results.Add(row);
    };
    for (int i = 0; i < 10; ++i)
        add(L"C:\\Apps\\A.exe", i % 2 ? L"c:\\data\\X.txt" : L"C:\\Data\\x.txt", L"Write", ItemKind::File);
    add(L"C:\\Apps\\A.exe", L"C:\\Data\\x.txt", L"Delete request", ItemKind::File);
    add(L"C:\\Apps\\A.exe", L"C:\\Data\\Z-folder", L"Create", ItemKind::Directory);
    add(L"C:\\Apps\\A.exe", L"C:\\Data\\A-folder", L"Create", ItemKind::Directory);
    add(L"C:\\Apps\\B.exe", L"C:\\Data\\A.txt", L"Write", ItemKind::File);
    add(L"C:\\Apps\\B.exe", L"C:\\Data\\Z-folder", L"Delete request", ItemKind::Unknown);
    add(L"C:\\Apps\\B.exe", L"C:\\Data\\Z-folder", L"Create", ItemKind::Directory);
    add(L"C:\\Apps\\B.exe", L"C:\\Data\\A.txt", L"Write", ItemKind::File, L"Failed: 0xC0000022");
    auto rows = results.Sorted();
    if (rows.size() != 7 || rows[0].kind != ItemKind::Directory || rows[0].path != L"C:\\Data\\A-folder" ||
        rows[1].path != L"C:\\Data\\Z-folder")
        return 1;
    if (rows[3].operation != L"Write" || rows[3].count != 10 || rows[3].successes != 10)
        return 2;
    if (rows[4].kind != ItemKind::Directory || rows[5].kind != ItemKind::Directory ||
        rows[6].kind != ItemKind::File)
        return 3;
    if (rows[6].count != 2 || rows[6].successes != 1 || rows[6].failures != 1 ||
        rows[6].result != L"Mixed results")
        return 4;
    Row escaped{L"time", L"C:\\A.exe", L"Write", L"C:\\árvíz,\"teszt\"\n.txt", L"Success", 1};
    ResultSet csv;
    csv.Add(escaped);
    if (!SaveCsv(L"results-test.csv", csv.Sorted()))
        return 5;
    results.Clear();
    if (!results.Sorted().empty())
        return 6;
    std::cout << "PASS: deduplication, operation separation, case-insensitive paths, app grouping, "
                 "folder-first ordering, mixed outcomes, type propagation, reset, CSV.\n";
}
