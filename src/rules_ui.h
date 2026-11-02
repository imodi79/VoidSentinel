#pragma once
#include "filter.h"
bool EditRules(HWND owner, const std::wstring &storage, std::vector<Rule> &rules,
               const Rule *suggested = nullptr);
