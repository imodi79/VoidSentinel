#include "monitor.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <tdh.h>
#include <tlhelp32.h>

namespace
{
const GUID FileGuid = {0x90cbdc39, 0x4a3e, 0x11d1, {0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3}};
const GUID ThreadGuid = {0x3d6fa8d1, 0xfe05, 0x11d0, {0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c}};
const GUID KernelProcessGuid = {0x22fb2cd6, 0x0e7b, 0x422b, {0xa0, 0xc7, 0x2f, 0xad, 0x1f, 0xd0, 0xe7, 0x16}};
const GUID KernelFileGuid = {0xedd08927, 0x9cc4, 0x4e65, {0xb9, 0x70, 0xc2, 0x56, 0x0f, 0xb5, 0xc2, 0x89}};
std::vector<BYTE> Property(EVENT_RECORD *e, const wchar_t *name)
{
    PROPERTY_DATA_DESCRIPTOR d{};
    d.PropertyName = reinterpret_cast<ULONGLONG>(name);
    d.ArrayIndex = ULONG_MAX;
    ULONG size = 0;
    if (TdhGetPropertySize(e, 0, nullptr, 1, &d, &size) != ERROR_SUCCESS || size > 65536)
        return {};
    std::vector<BYTE> data(size);
    if (TdhGetProperty(e, 0, nullptr, 1, &d, size, data.data()) != ERROR_SUCCESS)
        return {};
    return data;
}
ULONGLONG Number(EVENT_RECORD *e, const wchar_t *name)
{
    auto data = Property(e, name);
    ULONGLONG n = 0;
    if (!data.empty())
        memcpy(&n, data.data(), std::min(data.size(), sizeof(n)));
    return n;
}
std::wstring String(EVENT_RECORD *e, const wchar_t *name)
{
    auto data = Property(e, name);
    std::wstring s;
    for (size_t i = 0; i + 1 < data.size(); i += 2)
    {
        wchar_t c;
        memcpy(&c, data.data() + i, 2);
        if (!c)
            break;
        s += c;
    }
    return s;
}
bool Same(const std::wstring &a, const std::wstring &b)
{
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}
std::wstring Timestamp(LONGLONG ticks)
{
    FILETIME ft{static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32)}, local{};
    SYSTEMTIME st{};
    FileTimeToLocalFileTime(&ft, &local);
    FileTimeToSystemTime(&local, &st);
    wchar_t buf[64];
    swprintf_s(buf, L"%04u-%02u-%02u %02u:%02u:%02u.%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
               st.wSecond, st.wMilliseconds);
    return buf;
}
} // namespace
std::string Utf8(const std::wstring &s)
{
    int n =
        WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
    return out;
}
std::wstring Wide(const std::string &s)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}
std::wstring ErrorText(ULONG code)
{
    wchar_t *p = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<LPWSTR>(&p), 0, nullptr);
    std::wstring s = p ? p : L"Unknown error";
    if (p)
        LocalFree(p);
    return std::to_wstring(code) + L": " + s;
}
std::wstring ExecutablePath(DWORD pid)
{
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h)
        return {};
    std::wstring s(32768, 0);
    DWORD n = static_cast<DWORD>(s.size());
    if (QueryFullProcessImageNameW(h, 0, s.data(), &n))
        s.resize(n);
    else
        s.clear();
    CloseHandle(h);
    return s;
}
Monitor::~Monitor()
{
    Stop();
}
bool Monitor::Start(const std::vector<std::wstring> &targets, bool children, std::wstring &error)
{
    Stop();
    targets_ = targets;
    children_ = children;
    recorded = 0;
    dropped = 0;
    lost = 0;
    unresolved = 0;
    unmatched = 0;
    traceError = 0;
    results_.Clear();
    sequence_ = 0;
    renamePaths_.clear();
    objectKinds_.clear();
    processes_.clear();
    threads_.clear();
    names_.clear();
    pending_.clear();
    volumes_.clear();
    lease_ = CreateMutexW(nullptr, FALSE, L"Global\\VoidSentinel.Capture.v1");
    auto leaseError = GetLastError();
    if (!lease_ || leaseError == ERROR_ALREADY_EXISTS)
    {
        error = lease_ ? L"Another VoidSentinel instance is already monitoring."
                       : L"Cannot reserve the monitoring session.\n" + ErrorText(leaseError);
        if (lease_)
            CloseHandle(lease_);
        lease_ = nullptr;
        return false;
    }
    for (wchar_t c = L'A'; c <= L'Z'; ++c)
    {
        wchar_t drive[] = {c, L':', 0};
        wchar_t buf[32768];
        if (QueryDosDeviceW(drive, buf, 32768))
            volumes_.emplace_back(buf, drive);
    }
    sessionName_ = L"VoidSentinel.FileMonitor.v1";
    props_.assign(sizeof(EVENT_TRACE_PROPERTIES) + (sessionName_.size() + 1) * sizeof(wchar_t), 0);
    auto p = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(props_.data());
    p->Wnode.BufferSize = static_cast<ULONG>(props_.size());
    p->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    p->Wnode.ClientContext = 2;
    p->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE;
    p->EnableFlags = EVENT_TRACE_FLAG_FILE_IO | EVENT_TRACE_FLAG_FILE_IO_INIT |
                     EVENT_TRACE_FLAG_DISK_FILE_IO | EVENT_TRACE_FLAG_THREAD;
    p->BufferSize = 64;
    p->MinimumBuffers = 32;
    p->MaximumBuffers = 128;
    p->FlushTimer = 1;
    p->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    auto originalProperties = props_;
    ULONG rc = StartTraceW(&session_, sessionName_.c_str(), p);
    if (rc == ERROR_ALREADY_EXISTS)
    {
        // Our process-wide lease excludes a live capture owner. Recover only our own stale session.
        auto stale = originalProperties;
        if (ControlTraceW(0, sessionName_.c_str(), reinterpret_cast<EVENT_TRACE_PROPERTIES *>(stale.data()),
                          EVENT_TRACE_CONTROL_STOP) == ERROR_SUCCESS)
        {
            props_ = originalProperties;
            p = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(props_.data());
            rc = StartTraceW(&session_, sessionName_.c_str(), p);
        }
    }
    if (rc != ERROR_SUCCESS)
    {
        session_ = 0;
        error = L"ETW could not start. Run as administrator.\n" + ErrorText(rc);
        Stop();
        return false;
    }
    rc = EnableTraceEx2(session_, &KernelProcessGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                        TRACE_LEVEL_INFORMATION, 0x10, 0, 0, nullptr);
    if (rc != ERROR_SUCCESS)
    {
        error = L"Process tracking could not start.\n" + ErrorText(rc);
        Stop();
        return false;
    }
    // Snapshot after enabling events, so short-lived processes also have queued start events.
    rc = EnableTraceEx2(session_, &KernelFileGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                        TRACE_LEVEL_INFORMATION, 0x800, 0, 0, nullptr);
    if (rc != ERROR_SUCCESS)
    {
        error = L"Rename path tracking could not start.\n" + ErrorText(rc);
        Stop();
        return false;
    }
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, 0);
    if (snap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W pe{sizeof(pe)};
        if (Process32FirstW(snap, &pe))
            do
            {
                auto path = ExecutablePath(pe.th32ProcessID);
                bool sel = false;
                for (auto &t : targets_)
                    if (Same(t, path))
                        sel = true;
                if (pe.th32ProcessID == GetCurrentProcessId())
                    sel = false;
                processes_[pe.th32ProcessID] = {path, sel};
                if (sel)
                {
                    processes_[pe.th32ProcessID].root = path;
                    processes_[pe.th32ProcessID].lineage = path;
                }
            } while (Process32NextW(snap, &pe));
        if (children_ && Process32FirstW(snap, &pe))
        {
            for (int i = 0; i < 32; ++i)
            {
                bool changed = false;
                Process32FirstW(snap, &pe);
                do
                {
                    auto parent = processes_.find(pe.th32ParentProcessID);
                    auto &child = processes_[pe.th32ProcessID];
                    if (pe.th32ProcessID != GetCurrentProcessId() && !child.selected &&
                        parent != processes_.end() && parent->second.selected)
                    {
                        child.selected = true;
                        child.root = parent->second.root;
                        child.lineage = parent->second.lineage + L" -> " + child.path;
                        changed = true;
                    }
                } while (Process32NextW(snap, &pe));
                if (!changed)
                    break;
            }
        }
        THREADENTRY32 te{sizeof(te)};
        if (Thread32First(snap, &te))
            do
            {
                threads_[te.th32ThreadID] = te.th32OwnerProcessID;
            } while (Thread32Next(snap, &te));
        CloseHandle(snap);
    }
    EVENT_TRACE_LOGFILEW log{};
    log.LoggerName = sessionName_.data();
    log.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    log.EventRecordCallback = Callback;
    log.Context = this;
    consumer_ = OpenTraceW(&log);
    if (consumer_ == INVALID_PROCESSTRACE_HANDLE)
    {
        error = ErrorText(GetLastError());
        Stop();
        return false;
    }

    traceThread_ = std::thread([this] {
        auto rc = ProcessTrace(&consumer_, 1, nullptr, nullptr);
        if (rc != ERROR_SUCCESS && rc != ERROR_CANCELLED)
            traceError = rc;
    });
    return true;
}
void Monitor::RefreshStats()
{
    if (session_)
    {
        auto p = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(props_.data());
        if (ControlTraceW(session_, sessionName_.c_str(), p, EVENT_TRACE_CONTROL_QUERY) == ERROR_SUCCESS)
            lost = p->EventsLost;
    }
}
void Monitor::Stop()
{
    bool stopped = true;
    if (session_)
    {
        auto p = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(props_.data());
        auto rc = ControlTraceW(session_, sessionName_.c_str(), p, EVENT_TRACE_CONTROL_STOP);
        if (rc == ERROR_SUCCESS)
            lost = p->EventsLost;
        else
        {
            stopped = false;
            traceError = rc;
        }
        session_ = 0;
    }
    if (!stopped && consumer_ != INVALID_PROCESSTRACE_HANDLE)
        CloseTrace(consumer_);
    if (traceThread_.joinable())
        traceThread_.join();
    if (stopped && consumer_ != INVALID_PROCESSTRACE_HANDLE)
        CloseTrace(consumer_);
    consumer_ = INVALID_PROCESSTRACE_HANDLE;
    for (auto &[irp, p] : pending_)
    {
        (void)irp;
        if (p.opcode == 64)
            p.row.operation = L"Open/create request";
        p.row.result = L"Completion unavailable";
        Emit(std::move(p.row));
        ++unmatched;
    }
    pending_.clear();
    if (lease_)
    {
        CloseHandle(lease_);
        lease_ = nullptr;
    }
}
DWORD Monitor::ProcessFor(DWORD tid)
{
    auto it = threads_.find(tid);
    if (it != threads_.end())
        return it->second;
    HANDLE h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
    if (!h)
        return 0;
    DWORD pid = GetProcessIdOfThread(h);
    CloseHandle(h);
    if (pid)
        threads_[tid] = pid;
    return pid;
}
bool Monitor::Selected(DWORD pid, std::wstring &path)
{
    if (!pid || pid == GetCurrentProcessId())
        return false;
    auto it = processes_.find(pid);
    if (it == processes_.end())
    {
        path = ExecutablePath(pid);
        bool selected = false;
        for (auto &t : targets_)
            if (Same(t, path))
                selected = true;
        it =
            processes_.emplace(pid, Proc{path, selected, selected ? path : L"", selected ? path : L""}).first;
    }
    path = it->second.path;
    return it->second.selected;
}
void WINAPI Monitor::Callback(EVENT_RECORD *e)
{
    auto self = static_cast<Monitor *>(e->UserContext);
    try
    {
        self->Event(e);
    }
    catch (...)
    {
        ++self->dropped;
    }
}
void Monitor::Event(EVENT_RECORD *e)
{
    auto op = e->EventHeader.EventDescriptor.Opcode;
    if (IsEqualGUID(e->EventHeader.ProviderId, KernelProcessGuid))
    {
        auto id = e->EventHeader.EventDescriptor.Id;
        DWORD pid = static_cast<DWORD>(Number(e, L"ProcessID"));
        if (!pid)
            return;
        if (id == 2)
        {
            processes_.erase(pid);
            return;
        }
        if (id != 1)
            return;
        auto path = String(e, L"ImageName");
        for (auto &[device, drive] : volumes_)
            if (path.compare(0, device.size(), device) == 0 &&
                (path.size() == device.size() || path[device.size()] == L'\\'))
            {
                path = drive + path.substr(device.size());
                break;
            }
        bool selected = false;
        for (auto &target : targets_)
            if (Same(target, path))
                selected = true;
        bool directSelected = selected;
        auto parent = processes_.find(static_cast<DWORD>(Number(e, L"ParentProcessID")));
        if (children_ && parent != processes_.end() && parent->second.selected)
            selected = true;
        if (pid == GetCurrentProcessId())
            selected = false;
        Proc proc{path, selected, path, path};
        if (!directSelected && children_ && parent != processes_.end() && parent->second.selected)
        {
            proc.root = parent->second.root;
            proc.lineage = parent->second.lineage + L" -> " + path;
        }
        processes_[pid] = std::move(proc);
        return;
    }
    if (IsEqualGUID(e->EventHeader.ProviderId, ThreadGuid))
    {
        DWORD tid = static_cast<DWORD>(Number(e, L"TThreadId")),
              pid = static_cast<DWORD>(Number(e, L"ProcessId"));
        if (op == 2)
            threads_.erase(tid);
        else if ((op == 1 || op == 3) && tid && pid)
            threads_[tid] = pid;
        return;
    }
    if (IsEqualGUID(e->EventHeader.ProviderId, KernelFileGuid))
    {
        if (e->EventHeader.EventDescriptor.Id == 27)
        {
            auto irp = Number(e, L"Irp");
            if (!irp)
                irp = Number(e, L"IrpPtr");
            auto name = String(e, L"FilePath");
            if (name.empty())
                name = String(e, L"FileName");
            if (irp && !name.empty() && pending_.contains(irp))
            {
                if (renamePaths_.size() < 50000)
                    renamePaths_[irp] = name;
            }
        }
        return;
    }
    if (!IsEqualGUID(e->EventHeader.ProviderId, FileGuid))
        return;
    if (op == 0 || op == 32 || op == 35 || op == 36)
    {
        auto key = Number(e, L"FileObject");
        auto name = String(e, L"FileName");
        if (key && !name.empty())
        {
            if (names_.size() > 200000)
                names_.clear();
            names_[key] = name;
        }
        return;
    }
    if (op == 76)
    {
        auto irp = Number(e, L"IrpPtr");
        auto it = pending_.find(irp);
        if (it == pending_.end())
            return;
        auto p = std::move(it->second);
        pending_.erase(it);
        auto status = static_cast<ULONG>(Number(e, L"NtStatus"));
        if (p.opcode == 64)
        {
            if (static_cast<LONG>(status) >= 0 && status != 0x103)
            {
                auto info = Number(e, L"ExtraInfo");
                if (info == 1)
                    return;
                p.row.operation = info == 2                  ? L"Create"
                                  : (info == 0 || info == 3) ? L"Overwrite"
                                                             : L"Open/create";
            }
            else
            {
                if (p.disposition == 1)
                    return;
                p.row.operation = L"Create/overwrite request";
            }
        }
        if (static_cast<LONG>(status) < 0)
        {
            wchar_t s[32];
            swprintf_s(s, L"Failed: 0x%08lX", status);
            p.row.result = s;
        }
        else
            p.row.result = status == 0x103 ? L"Pending (not confirmed)" : L"Success";
        if (p.opcode == 71)
        {
            auto rename = renamePaths_.find(irp);
            if (rename != renamePaths_.end())
            {
                p.row.destination = rename->second;
                for (auto &[device, drive] : volumes_)
                    if (p.row.destination.starts_with(device + L"\\"))
                    {
                        p.row.destination = drive + p.row.destination.substr(device.size());
                        break;
                    }
                if (p.row.result == L"Success")
                {
                    names_[p.object] = p.row.destination;
                }
                renamePaths_.erase(rename);
            }
        }
        Emit(std::move(p.row));
        return;
    }
    if (op == 66)
    {
        objectKinds_.erase(Number(e, L"FileObject"));
        names_.erase(Number(e, L"FileObject"));
        return;
    }
    if (op != 64 && op != 68 && op != 69 && op != 70 && op != 71)
        return;
    DWORD tid = static_cast<DWORD>(Number(e, L"TTID"));
    if (!tid)
        tid = static_cast<DWORD>(Number(e, L"IssuingThreadId"));
    if (!tid)
        tid = e->EventHeader.ThreadId;
    DWORD pid = ProcessFor(tid);
    if (!pid && e->EventHeader.ProcessId != ULONG_MAX)
        pid = e->EventHeader.ProcessId;
    std::wstring app;
    if (!Selected(pid, app))
        return;
    auto object = Number(e, L"FileObject");
    auto key = Number(e, L"FileKey");
    std::wstring path;
    if (op == 64)
    {
        path = String(e, L"OpenPath");
        if (!path.empty())
            names_[object] = path;
    }
    if (path.empty())
    {
        auto it = names_.find(object);
        if (it == names_.end())
            it = names_.find(key);
        if (it != names_.end())
            path = it->second;
    }
    if (path.empty())
        path = L"[unresolved file object " + std::to_wstring(object) + L"]";
    for (auto &[device, drive] : volumes_)
        if (path.compare(0, device.size(), device) == 0 &&
            (path.size() == device.size() || path[device.size()] == L'\\'))
        {
            path = drive + path.substr(device.size());
            break;
        }
    const wchar_t *operation = op == 64   ? L"Open/create"
                               : op == 68 ? L"Write"
                               : op == 69 ? L"Set information"
                               : op == 70 ? L"Delete request"
                                          : L"Rename request";
    if (op == 64)
    {
        auto options = Number(e, L"CreateOptions");
        objectKinds_[object] = (options & 1)      ? ItemKind::Directory
                               : (options & 0x40) ? ItemKind::File
                                                  : ItemKind::Unknown;
    }
    Row row{Timestamp(e->EventHeader.TimeStamp.QuadPart), app, operation, path, L"Requested", pid};
    row.sequence = ++sequence_;
    if (auto proc = processes_.find(pid); proc != processes_.end())
    {
        row.rootApp = proc->second.root;
        row.lineage = proc->second.lineage;
    }
    if (auto kind = objectKinds_.find(object); kind != objectKinds_.end())
        row.kind = kind->second;
    if (op == 69)
    {
        auto info = Number(e, L"InfoClass");
        row.operation = info == 19   ? L"Allocation size"
                        : info == 20 ? L"End of file"
                        : info == 4  ? L"Attributes / timestamps"
                                     : L"Set information (class " + std::to_wstring(info) + L")";
    }
    auto disposition = op == 64 ? static_cast<ULONG>(Number(e, L"CreateOptions") >> 24) : 0;
    auto irp = Number(e, L"IrpPtr");
    if (irp)
    {
        if (pending_.size() >= 50000)
        {
            ++dropped;
            return;
        }
        auto previous = pending_.find(irp);
        if (previous != pending_.end())
        {
            previous->second.row.result = L"Completion unavailable";
            Emit(std::move(previous->second.row));
            ++unmatched;
        }
        pending_[irp] = {std::move(row), op, object, disposition};
    }
    else
    {
        ++unmatched;
        Emit(std::move(row));
    }
}
void Monitor::Emit(Row row)
{
    if (!row.sequence)
        row.sequence = ++sequence_;
    if (row.path.starts_with(L"[unresolved file object"))
        ++unresolved;
    std::lock_guard lock(mutex_);
    results_.Add(std::move(row));
    ++recorded;
}
std::vector<Row> Monitor::RawSnapshot()
{
    std::lock_guard lock(mutex_);
    return results_.Copy();
}
std::vector<Row> Monitor::Snapshot()
{
    ResultSet copy;
    {
        std::lock_guard lock(mutex_);
        copy = results_;
    }
    return copy.Sorted();
}
