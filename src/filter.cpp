#include "filter.h"
#include "monitor.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace
{
std::wstring Lower(std::wstring s)
{
    if (!s.empty())
        CharLowerBuffW(s.data(), static_cast<DWORD>(s.size()));
    return s;
}
std::wstring Name(const std::wstring &s)
{
    return std::filesystem::path(s).filename().wstring();
}
std::wstring Expand(const std::wstring &s)
{
    DWORD n = ExpandEnvironmentStringsW(s.c_str(), nullptr, 0);
    if (!n || n > 32768)
        return s;
    std::wstring out(n, 0);
    ExpandEnvironmentStringsW(s.c_str(), out.data(), n);
    out.pop_back();
    return out;
}
bool Wildcard(const std::wstring &pattern, const std::wstring &value)
{
    size_t p = 0, v = 0, star = std::wstring::npos, retry = 0;
    while (v < value.size())
    {
        if (p < pattern.size() && (pattern[p] == L'?' || pattern[p] == value[v]))
        {
            ++p;
            ++v;
        }
        else if (p < pattern.size() && pattern[p] == L'*')
        {
            star = p++;
            retry = v;
        }
        else if (star != std::wstring::npos)
        {
            p = star + 1;
            v = ++retry;
        }
        else
            return false;
    }
    while (p < pattern.size() && pattern[p] == L'*')
        ++p;
    return p == pattern.size();
}
struct Decision
{
    bool include = false, exclude = false, background = false;
};
struct PreparedRule
{
    RuleAction action;
    RuleMatch match;
    std::wstring path, app;
    bool appName;
};
std::vector<PreparedRule> CompileRules(const std::vector<Rule> &rules)
{
    std::vector<PreparedRule> out;
    for (auto &r : rules)
        if (r.enabled)
        {
            auto app = Lower(Expand(r.application));
            out.push_back({r.action, r.match, NormalizePath(Expand(r.pattern)), app,
                           app.find_first_of(L"\\/") == std::wstring::npos});
        }
    return out;
}
Decision Decide(const std::vector<PreparedRule> &rules, const Row &row)
{
    std::vector<std::wstring> paths{NormalizePath(row.path)};
    if (!row.destination.empty())
        paths.push_back(NormalizePath(row.destination));
    if (!row.originalPath.empty())
        paths.push_back(NormalizePath(row.originalPath));
    auto app = Lower(row.app), root = Lower(row.rootApp), appName = Lower(Name(row.app)),
         rootName = Lower(Name(row.rootApp));
    Decision out;
    for (auto &r : rules)
    {
        if (!r.app.empty() && !Wildcard(r.app, r.appName ? appName : app) &&
            !Wildcard(r.app, r.appName ? rootName : root))
            continue;
        bool match = false;
        for (auto &path : paths)
        {
            // Exclusions follow the visible final path; a temporary source must not hide a saved document.
            if (r.action != RuleAction::Include && &path != &paths.front())
                continue;
            if (r.match == RuleMatch::Wildcard
                    ? Wildcard(r.path, path)
                    : path == r.path || (r.match == RuleMatch::Folder && !r.path.empty() &&
                                         path.size() > r.path.size() && path.starts_with(r.path) &&
                                         (r.path.back() == L'\\' || path[r.path.size()] == L'\\')))
            {
                match = true;
                break;
            }
        }
        if (!match)
            continue;
        if (r.action == RuleAction::Include)
            return {true, false, false};
        if (r.action == RuleAction::Exclude)
            out.exclude = true;
        if (r.action == RuleAction::Background)
            out.background = true;
    }
    return out;
}
bool IsWrite(const std::wstring &op)
{
    return op == L"Write" || op == L"End of file" || op == L"Allocation size" || op == L"Overwrite";
}
bool IsMeta(const std::wstring &op)
{
    return op == L"Attributes / timestamps" || op.starts_with(L"Set information");
}
void Join(std::wstring &dst, const std::wstring &value)
{
    if (value.empty() || dst == value)
        return;
    if (!dst.empty())
        dst += L"; ";
    dst += value;
}
} // namespace
std::wstring NormalizePath(std::wstring p)
{
    std::replace(p.begin(), p.end(), L'/', L'\\');
    if (p.starts_with(L"\\\\?\\UNC\\"))
        p = L"\\\\" + p.substr(8);
    else if (p.starts_with(L"\\\\?\\") || p.starts_with(L"\\??\\"))
        p = p.substr(4);
    p = std::filesystem::path(p).lexically_normal().wstring();
    while (p.size() > 3 && p.back() == L'\\')
        p.pop_back();
    return Lower(p);
}
std::wstring PortablePath(const std::wstring &path)
{
    auto value = NormalizePath(path);
    for (auto variable : {L"%LOCALAPPDATA%", L"%APPDATA%", L"%PROGRAMDATA%", L"%USERPROFILE%"})
    {
        auto base = Expand(variable), normal = NormalizePath(base);
        if (value == normal || value.starts_with(normal + L"\\"))
            return std::wstring(variable) + path.substr(base.size());
    }
    return path;
}
bool RuleMatches(const Rule &r, const std::wstring &path, const std::wstring &app)
{
    if (!r.enabled)
        return false;
    if (!r.application.empty())
    {
        auto pattern = Lower(Expand(r.application));
        auto candidate = Lower(pattern.find_first_of(L"\\/") == std::wstring::npos ? Name(app) : app);
        if (!Wildcard(pattern, candidate))
            return false;
    }
    auto p = NormalizePath(Expand(r.pattern)), v = NormalizePath(path);
    if (r.match == RuleMatch::Wildcard)
        return Wildcard(p, v);
    if (v == p)
        return true;
    return !p.empty() && r.match == RuleMatch::Folder && v.size() > p.size() && v.starts_with(p) &&
           (p.back() == L'\\' || v[p.size()] == L'\\');
}
std::vector<Rule> DefaultRules()
{
    std::vector<Rule> out;
    auto add = [&](const wchar_t *p, const wchar_t *text) {
        out.push_back({true, RuleAction::Background, RuleMatch::Folder, p, L"", text});
    };
    add(L"%LOCALAPPDATA%\\NVIDIA\\DXCache", L"NVIDIA shader cache");
    add(L"%LOCALAPPDATA%\\NVIDIA\\GLCache", L"NVIDIA shader cache");
    out.push_back({true, RuleAction::Background, RuleMatch::Exact,
                   L"%PROGRAMDATA%\\NVIDIA Corporation\\Drs\\nvAppTimestamps", L"",
                   L"NVIDIA application timestamps"});
    add(L"%LOCALAPPDATA%\\Microsoft\\Windows\\Caches", L"Windows caches");
    add(L"%APPDATA%\\Microsoft\\Spelling", L"Spelling dictionaries");
    out.push_back({true, RuleAction::Background, RuleMatch::Wildcard,
                   L"%LOCALAPPDATA%\\Packages\\Microsoft.WindowsNotepad_*\\LocalState\\TabState\\*",
                   L"Notepad.exe", L"Notepad session recovery state"});
    out.push_back({true, RuleAction::Background, RuleMatch::Wildcard,
                   L"%LOCALAPPDATA%\\Packages\\Microsoft.WindowsNotepad_*\\LocalState\\TabState",
                   L"Notepad.exe", L"Notepad session recovery folder"});
    // Deliberately no blanket AppData/ProgramData/Windows or Packages exclusion.
    return out;
}
bool ValidateRules(const std::vector<Rule> &rules, std::wstring &error)
{
    if (rules.size() > 10000)
    {
        error = L"At most 10000 rules are supported.";
        return false;
    }
    for (const auto &r : rules)
    {
        if (r.pattern.empty() || r.pattern.size() > 4096 || r.application.size() > 4096 ||
            r.description.size() > 4096 || r.pattern.find_first_of(L"\t\r\n") != std::wstring::npos ||
            r.application.find_first_of(L"\t\r\n") != std::wstring::npos ||
            r.description.find_first_of(L"\t\r\n") != std::wstring::npos)
        {
            error =
                L"Each rule needs a path (max 4096 characters); tabs and newlines are not allowed in fields.";
            return false;
        }
        auto pattern = Expand(r.pattern);
        if (pattern.empty() || pattern.find(L'%') != std::wstring::npos)
        {
            error = L"A rule contains an unknown environment variable.";
            return false;
        }
        if (r.match != RuleMatch::Wildcard && pattern.find_first_of(L"*?") != std::wstring::npos)
        {
            error = L"Use Wildcard matching for * or ? patterns.";
            return false;
        }
        if (!std::filesystem::path(pattern).is_absolute())
        {
            error =
                L"Rule paths must be absolute, or begin with an environment variable such as %LOCALAPPDATA%.";
            return false;
        }
    }
    return true;
}
bool LoadRules(const std::wstring &path, std::vector<Rule> &rules, std::wstring &error)
{
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in)
    {
        error = L"Cannot open rules file.";
        return false;
    }
    in.seekg(0, std::ios::end);
    if (in.tellg() > 4 * 1024 * 1024)
    {
        error = L"Rules file is too large.";
        return false;
    }
    in.seekg(0);
    std::string content((std::istreambuf_iterator<char>(in)), {});
    if (content.starts_with("\xEF\xBB\xBF"))
        content.erase(0, 3);
    if (content.find('\0') != std::string::npos ||
        (!content.empty() && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, content.data(),
                                                  static_cast<int>(content.size()), nullptr, 0)))
    {
        error = L"Rules file must be valid UTF-8.";
        return false;
    }
    std::istringstream lines(content);
    std::string line;
    std::vector<Rule> parsed;
    if (!std::getline(lines, line))
    {
        error = L"Empty rules file.";
        return false;
    }
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    if (line != "VoidSentinel-Rules\t1")
    {
        error = L"Unsupported rules format or version.";
        return false;
    }
    while (std::getline(lines, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        if (parsed.size() >= 10000)
        {
            error = L"At most 10000 rules are supported.";
            return false;
        }
        std::vector<std::string> fields;
        size_t start = 0;
        for (;;)
        {
            auto end = line.find('\t', start);
            fields.push_back(line.substr(start, end - start));
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        if (fields.size() != 6 || (fields[0] != "1" && fields[0] != "0") ||
            (fields[1] != "include" && fields[1] != "exclude" && fields[1] != "background") ||
            (fields[2] != "folder" && fields[2] != "exact" && fields[2] != "wildcard"))
        {
            error = L"Invalid rule record.";
            return false;
        }
        parsed.push_back({fields[0] == "1",
                          fields[1] == "include"   ? RuleAction::Include
                          : fields[1] == "exclude" ? RuleAction::Exclude
                                                   : RuleAction::Background,
                          fields[2] == "folder"  ? RuleMatch::Folder
                          : fields[2] == "exact" ? RuleMatch::Exact
                                                 : RuleMatch::Wildcard,
                          Wide(fields[3]), Wide(fields[4]), Wide(fields[5])});
    }
    if (!ValidateRules(parsed, error))
        return false;
    rules = std::move(parsed);
    return true;
}
bool SaveRules(const std::wstring &path, const std::vector<Rule> &rules, std::wstring &error)
{
    if (!ValidateRules(rules, error))
        return false;
    auto tmp = path + L".tmp-" + std::to_wstring(GetCurrentProcessId());
    std::ofstream out(std::filesystem::path(tmp), std::ios::binary | std::ios::trunc);
    if (!out)
    {
        error = L"Cannot create rules file.";
        return false;
    }
    out << "VoidSentinel-Rules\t1\r\n";
    for (auto &r : rules)
        out << (r.enabled ? "1" : "0") << '\t'
            << (r.action == RuleAction::Include   ? "include"
                : r.action == RuleAction::Exclude ? "exclude"
                                                  : "background")
            << '\t'
            << (r.match == RuleMatch::Folder  ? "folder"
                : r.match == RuleMatch::Exact ? "exact"
                                              : "wildcard")
            << '\t' << Utf8(r.pattern) << '\t' << Utf8(r.application) << '\t' << Utf8(r.description)
            << "\r\n";
    out.flush();
    bool ok = out.good();
    out.close();
    if (!ok || !MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileW(tmp.c_str());
        error = L"Cannot save rules; previous file was kept.";
        return false;
    }
    return true;
}

ViewResult BuildView(const std::vector<Row> &events, const ViewOptions &options)
{
    ViewResult out;
    auto prepared = events;
    auto compiledRules = CompileRules(options.rules);
    std::map<ResultKey, std::vector<Row>, KeyLess> renames;
    for (const auto &r : events)
        if (r.operation == L"Rename request" && r.successes && !r.destination.empty())
            renames[{r.rootApp.empty() ? r.app : r.rootApp, NormalizePath(r.path), L""}].push_back(r);
    for (auto &[key, edges] : renames)
        std::sort(edges.begin(), edges.end(), [](auto &a, auto &b) { return a.sequence < b.sequence; });
    // Follow observed successful edges only. Overlapping aggregated lifetimes remain unmerged.
    if (!options.technical)
        for (auto &r : prepared)
        {
            auto root = r.rootApp.empty() ? r.app : r.rootApp;
            auto cursor = r.sequence;
            for (int depth = 0; depth < 64; ++depth)
            {
                auto it = renames.find({root, NormalizePath(r.path), L""});
                if (it == renames.end())
                    break;
                auto edge = std::lower_bound(
                    it->second.begin(), it->second.end(), cursor,
                    [](const Row &edge, unsigned long long seq) { return edge.sequence < seq; });
                if (edge == it->second.end() || r.lastSequence > edge->sequence)
                    break;
                if (r.originalPath.empty())
                    r.originalPath = r.path;
                r.path = edge->destination;
                r.destination = edge->destination;
                cursor = edge->sequence + 1;
                if (r.operation == L"Create")
                    r.operation = L"Saved via rename";
            }
        }
    std::map<ResultKey, ItemKind, KeyLess> knownKinds;
    for (const auto &r : prepared)
        if (r.kind != ItemKind::Unknown)
            knownKinds[{r.rootApp.empty() ? r.app : r.rootApp, r.path, L""}] = r.kind;
    for (auto &r : prepared)
        if (auto kind = knownKinds.find({r.rootApp.empty() ? r.app : r.rootApp, r.path, L""});
            kind != knownKinds.end())
            r.kind = kind->second;
    // An immutable technical snapshot is retained by the monitor; filters never discard captured data.
    std::map<ResultKey, Row, KeyLess> summaries;
    struct Life
    {
        bool created = false, deleted = false, renamed = false;
        unsigned long long first = ~0ULL, lastCreate = 0, lastDelete = 0;
    };
    std::map<ResultKey, Life, KeyLess> lives;
    for (const auto &r : prepared)
        if (r.successes)
        {
            auto &l = lives[{r.rootApp.empty() ? r.app : r.rootApp, r.path, L""}];
            if (r.operation == L"Create")
            {
                l.created = true;
                l.first = std::min(l.first, r.sequence);
                l.lastCreate = std::max(l.lastCreate, r.lastSequence);
            }
            if (r.operation == L"Delete request")
            {
                l.deleted = true;
                l.lastDelete = std::max(l.lastDelete, r.lastSequence);
            }
            if (r.operation == L"Rename request")
                l.renamed = true;
        }
    for (auto r : prepared)
    {
        auto root = r.rootApp.empty() ? r.app : r.rootApp;
        auto decision = Decide(compiledRules, r);
        if (decision.exclude)
        {
            out.excluded += r.count;
            continue;
        }
        if (decision.background && !options.background)
        {
            out.background += r.count;
            continue;
        }
        if (r.path.starts_with(L"[unresolved") && !options.unconfirmed)
        {
            out.unconfirmed += r.count;
            continue;
        }
        if (!options.errors)
            out.errors += r.failures;
        if (!options.unconfirmed)
            out.unconfirmed += r.unconfirmed;
        auto included =
            r.successes + (options.errors ? r.failures : 0) + (options.unconfirmed ? r.unconfirmed : 0);
        if (!included)
            continue;
        r.count = included;
        if (!options.errors)
            r.failures = 0;
        if (!options.unconfirmed)
            r.unconfirmed = 0;
        r.result = r.failures || r.unconfirmed ? (r.successes  ? L"Mixed results"
                                                  : r.failures ? L"Failed requests"
                                                               : L"Unconfirmed requests")
                                               : L"Success";
        auto life = lives.find({root, r.path, L""});
        bool transient = life != lives.end() && life->second.created && life->second.deleted &&
                         !life->second.renamed && life->second.lastDelete > life->second.lastCreate;
        if (transient && !options.transient && !decision.include)
        {
            out.transient += r.count;
            continue;
        }
        if (IsMeta(r.operation) && !options.metadata && !decision.include)
        {
            out.metadata += r.count;
            continue;
        }
        if (options.technical)
        {
            out.rows.push_back(std::move(r));
            continue;
        }
        auto op = r.operation;
        r.operation = op == L"Create"           ? L"Created"
                      : IsWrite(op)             ? L"Written / resized"
                      : op == L"Delete request" ? L"Deletion requested"
                      : op == L"Rename request" ? L"Renamed"
                      : IsMeta(op)              ? L"Metadata changed"
                                                : op;
        if (transient)
            r.operation = L"Transient candidate: " + r.operation;
        r.app = root;
        if (r.lineage.empty())
            r.lineage = root;
        ResultKey key{root, r.path, L""};
        auto [it, inserted] = summaries.try_emplace(key, r);
        if (!inserted)
        {
            auto &x = it->second;
            x.count += r.count;
            x.successes += r.successes;
            x.failures += r.failures;
            x.unconfirmed += r.unconfirmed;
            if (x.operation.find(r.operation) == std::wstring::npos)
                Join(x.operation, r.operation);
            if (x.lineage.find(r.lineage) == std::wstring::npos)
                Join(x.lineage, r.lineage);
            if (x.destination.find(r.destination) == std::wstring::npos)
                Join(x.destination, r.destination);
            if (x.originalPath.find(r.originalPath) == std::wstring::npos)
                Join(x.originalPath, r.originalPath);
            x.time = std::min(x.time, r.time);
            x.lastTime = std::max(x.lastTime, r.lastTime);
            if (r.kind != ItemKind::Unknown)
                x.kind = r.kind;
            if (x.result != r.result)
                x.result = L"Mixed results";
        }
    }
    if (!options.technical)
        for (auto &[key, row] : summaries)
            out.rows.push_back(std::move(row));
    std::sort(out.rows.begin(), out.rows.end(), [](const Row &a, const Row &b) {
        auto ar = a.rootApp.empty() ? a.app : a.rootApp, br = b.rootApp.empty() ? b.app : b.rootApp;
        KeyLess less;
        auto an = NormalizePath(Name(ar)), bn = NormalizePath(Name(br));
        if (an != bn)
            return an < bn;
        if (_wcsicmp(ar.c_str(), br.c_str()))
            return _wcsicmp(ar.c_str(), br.c_str()) < 0;
        if (a.kind != b.kind)
            return a.kind < b.kind;
        return less({a.app, a.path, a.operation}, {b.app, b.path, b.operation});
    });
    return out;
}
