#include "results.h"
#include "monitor.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace
{
int Compare(const std::wstring &a, const std::wstring &b)
{
    return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) - CSTR_EQUAL;
}
std::wstring AppName(const std::wstring &path)
{
    return std::filesystem::path(path).filename().wstring();
}
std::string Cell(const std::wstring &text)
{
    std::string out = "\"";
    for (char c : Utf8(text))
    {
        if (c == '"')
            out += '"';
        out += c;
    }
    return out + '"';
}
} // namespace
bool KeyLess::operator()(const ResultKey &a, const ResultKey &b) const
{
    if (auto c = Compare(a.app, b.app))
        return c < 0;
    if (auto c = Compare(a.path, b.path))
        return c < 0;
    return Compare(a.operation, b.operation) < 0;
}
void ResultSet::Add(Row row)
{
    row.successes = row.result == L"Success" ? 1 : 0;
    row.failures = row.result.starts_with(L"Failed:") ? 1 : 0;
    row.unconfirmed = !row.successes && !row.failures ? 1 : 0;
    row.lastTime = row.time;
    ResultKey key{row.app, row.path, row.operation};
    auto [it, inserted] = rows_.try_emplace(std::move(key), row);
    if (inserted)
        return;
    auto &existing = it->second;
    ++existing.count;
    existing.successes += row.successes;
    existing.failures += row.failures;
    existing.unconfirmed += row.unconfirmed;
    if (row.time < existing.time)
        existing.time = row.time;
    if (row.lastTime > existing.lastTime)
        existing.lastTime = row.lastTime;
    if (existing.result != row.result)
        existing.result = L"Mixed results";
    if (existing.pid != row.pid)
        existing.pid = 0;
    if (row.kind != ItemKind::Unknown)
        existing.kind = row.kind;
}
std::vector<Row> ResultSet::Sorted() const
{
    std::vector<Row> out;
    out.reserve(rows_.size());
    // Carry known type to every operation on the same application/path, even after deletion.
    std::map<ResultKey, ItemKind, KeyLess> kinds;
    for (auto &[key, row] : rows_)
        if (row.kind != ItemKind::Unknown)
            kinds[{key.app, key.path, L""}] = row.kind;
    for (auto &[key, row] : rows_)
    {
        out.push_back(row);
        if (auto kind = kinds.find({key.app, key.path, L""}); kind != kinds.end())
            out.back().kind = kind->second;
    }
    std::sort(out.begin(), out.end(), [](const Row &a, const Row &b) {
        if (auto c = Compare(AppName(a.app), AppName(b.app)))
            return c < 0;
        if (auto c = Compare(a.app, b.app))
            return c < 0;
        if (a.kind != b.kind)
            return a.kind < b.kind;
        if (auto c = Compare(a.path, b.path))
            return c < 0;
        return Compare(a.operation, b.operation) < 0;
    });
    return out;
}
const wchar_t *KindName(ItemKind kind)
{
    return kind == ItemKind::Directory ? L"Folder" : kind == ItemKind::File ? L"File" : L"Unknown";
}
bool SaveCsv(const std::wstring &path, const std::vector<Row> &rows)
{
    std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!out)
        return false;
    out << "\xEF\xBB\xBF"
        << "Application,Application path,Type,Path,Operation,Count,First seen,Last "
           "seen,Result,Successes,Failures,Unconfirmed\r\n";
    for (auto &row : rows)
        out << Cell(AppName(row.app)) << ',' << Cell(row.app) << ',' << Cell(KindName(row.kind)) << ','
            << Cell(row.path) << ',' << Cell(row.operation) << ',' << row.count << ',' << Cell(row.time)
            << ',' << Cell(row.lastTime) << ',' << Cell(row.result) << ',' << row.successes << ','
            << row.failures << ',' << row.unconfirmed << "\r\n";
    out.flush();
    return out.good();
}
