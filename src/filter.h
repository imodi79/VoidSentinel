#pragma once
#include "results.h"

enum class RuleAction
{
    Include,
    Exclude,
    Background
};
enum class RuleMatch
{
    Folder,
    Exact,
    Wildcard
};
struct Rule
{
    bool enabled = true;
    RuleAction action = RuleAction::Exclude;
    RuleMatch match = RuleMatch::Folder;
    std::wstring pattern, application, description;
};
struct ViewOptions
{
    bool technical = false, errors = false, unconfirmed = false, background = false, transient = false,
         metadata = false;
    std::vector<Rule> rules;
};
struct ViewResult
{
    std::vector<Row> rows;
    unsigned long long excluded = 0, background = 0, transient = 0, errors = 0, unconfirmed = 0, metadata = 0;
};
std::vector<Rule> DefaultRules();
std::wstring NormalizePath(std::wstring path);
std::wstring PortablePath(const std::wstring &path);
bool RuleMatches(const Rule &rule, const std::wstring &path, const std::wstring &app);
bool ValidateRules(const std::vector<Rule> &rules, std::wstring &error);
bool LoadRules(const std::wstring &path, std::vector<Rule> &rules, std::wstring &error);
bool SaveRules(const std::wstring &path, const std::vector<Rule> &rules, std::wstring &error);
ViewResult BuildView(const std::vector<Row> &events, const ViewOptions &options);
