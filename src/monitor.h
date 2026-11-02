#pragma once
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "results.h"

std::string Utf8(const std::wstring &s);
std::wstring Wide(const std::string &s);
std::wstring ErrorText(ULONG code);
std::wstring ExecutablePath(DWORD pid);

class Monitor
{
  public:
    ~Monitor();
    bool Start(const std::vector<std::wstring> &targets, bool children, std::wstring &error);
    void Stop();
    std::vector<Row> Snapshot();
    void RefreshStats();
    std::atomic<unsigned long long> recorded{0}, dropped{0}, unresolved{0}, unmatched{0};
    std::atomic<ULONG> lost{0}, traceError{0};

  private:
    static void WINAPI Callback(EVENT_RECORD *e);
    void Event(EVENT_RECORD *e);
    DWORD ProcessFor(DWORD tid);
    bool Selected(DWORD pid, std::wstring &path);
    void Emit(Row row);
    struct Proc
    {
        std::wstring path;
        bool selected = false;
    };
    struct Pending
    {
        Row row;
        UCHAR opcode = 0;
        ULONGLONG object = 0;
        ULONG disposition = 0;
    };
    std::vector<std::wstring> targets_;
    bool children_ = true;
    TRACEHANDLE session_ = 0, consumer_ = INVALID_PROCESSTRACE_HANDLE;
    HANDLE lease_ = nullptr;
    std::vector<BYTE> props_;
    std::wstring sessionName_;
    std::thread traceThread_;
    std::mutex mutex_;
    std::unordered_map<DWORD, Proc> processes_;
    std::unordered_map<DWORD, DWORD> threads_;
    std::unordered_map<ULONGLONG, std::wstring> names_;
    std::unordered_map<ULONGLONG, Pending> pending_;
    std::vector<std::pair<std::wstring, std::wstring>> volumes_;
    ResultSet results_;
    std::unordered_map<ULONGLONG, ItemKind> objectKinds_;
};
