#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

enum class ItemKind
{
    Directory,
    File,
    Unknown
};
struct Row
{
    std::wstring time, app, operation, path, result;
    DWORD pid = 0;
    ItemKind kind = ItemKind::Unknown;
    unsigned long long count = 1, successes = 0, failures = 0, unconfirmed = 0;
    std::wstring lastTime;
};
struct ResultKey
{
    std::wstring app, path, operation;
};
struct KeyLess
{
    bool operator()(const ResultKey &a, const ResultKey &b) const;
};
class ResultSet
{
  public:
    void Add(Row row);
    void Clear()
    {
        rows_.clear();
    }
    std::vector<Row> Sorted() const;

  private:
    std::map<ResultKey, Row, KeyLess> rows_;
};
const wchar_t *KindName(ItemKind kind);
bool SaveCsv(const std::wstring &path, const std::vector<Row> &rows);
