#include "session.h"
#include <chrono>
#include <filesystem>
#include <fstream>

SessionWorker::SessionWorker()
{
    frame_ = std::make_shared<ViewFrame>();
    worker_ = std::thread([this] { Loop(); });
}
SessionWorker::~SessionWorker()
{
    {
        std::lock_guard guard(lock_);
        exit_ = true;
    }
    wake_.notify_one();
    worker_.join();
}
void SessionWorker::Post(std::function<void()> job)
{
    {
        std::lock_guard guard(lock_);
        jobs_.push_back(std::move(job));
    }
    wake_.notify_one();
}
std::shared_ptr<const ViewFrame> SessionWorker::Frame()
{
    std::lock_guard guard(lock_);
    return frame_;
}
void SessionWorker::Configure(ViewOptions options)
{
    Post([this, options = std::move(options)] {
        options_ = options;
        dirty_ = true;
    });
}
unsigned long long SessionWorker::Start(std::vector<std::wstring> targets, bool children)
{
    auto id = ++nextAction_;
    Post([this, targets = std::move(targets), children, id] {
        activeAction_ = id;
        busy_ = true;
        message_ = L"Starting capture...";
        Publish();
        std::wstring error;
        running_ = monitor_.Start(targets, children, error);
        busy_ = false;
        message_ = running_ ? L"" : error;
        dirty_ = true;
        completedAction_ = id;
        activeAction_ = 0;
    });
    return id;
}
unsigned long long SessionWorker::Stop()
{
    auto id = ++nextAction_;
    Post([this, id] {
        activeAction_ = id;
        busy_ = true;
        message_ = L"Stopping capture...";
        Publish();
        monitor_.Stop();
        running_ = false;
        busy_ = false;
        message_.clear();
        dirty_ = true;
        completedAction_ = id;
        activeAction_ = 0;
    });
    return id;
}
void SessionWorker::Export(std::wstring path, std::shared_ptr<const ViewFrame> frame)
{
    Post([this, path = std::move(path), frame] {
        message_ =
            SaveCsv(path, frame->content->data) ? L"Results saved." : L"ERROR: Results could not be saved.";
    });
}
void SessionWorker::PersistTargets(std::wstring path, std::vector<std::wstring> targets)
{
    Post([this, path = std::move(path), targets = std::move(targets)] {
        std::ofstream out(std::filesystem::path(path), std::ios::binary);
        for (auto &t : targets)
            out << Utf8(t) << '\n';
        out.flush();
        if (!out)
            message_ = L"ERROR: Application list could not be saved.";
    });
}
void SessionWorker::Publish()
{
    auto next = std::make_shared<ViewFrame>();
    auto count = monitor_.recorded.load();
    if (dirty_ || count != seen_)
    {
        auto content = std::make_shared<ViewContent>();
        auto view = BuildView(monitor_.RawSnapshot(), options_);
        content->data = std::move(view.rows);
        content->stats = std::move(view);
        std::wstring previous;
        for (const auto &row : content->data)
        {
            auto group = row.rootApp.empty() ? row.app : row.rootApp;
            if (_wcsicmp(previous.c_str(), group.c_str()))
            {
                Row header;
                header.app = group;
                content->display.push_back(std::move(header));
                previous = group;
            }
            content->display.push_back(row);
        }
        next->content = std::move(content);
        seen_ = count;
        dirty_ = false;
    }
    else
        next->content = Frame()->content;
    next->events = count;
    next->dropped = monitor_.dropped;
    next->unresolved = monitor_.unresolved;
    next->unmatched = monitor_.unmatched;
    next->lost = monitor_.lost;
    next->traceError = monitor_.traceError;
    next->running = running_;
    next->busy = busy_;
    next->message = message_;
    next->completedAction = completedAction_;
    {
        std::lock_guard guard(lock_);
        retired_.push_back(std::move(frame_));
        frame_ = std::move(next);
    }
    // Large superseded views are destroyed here, never by the GUI's pointer swap.
    std::erase_if(retired_, [](const auto &frame) { return frame.use_count() == 1; });
}
void SessionWorker::Loop()
{
    for (;;)
    {
        std::deque<std::function<void()>> work;
        {
            std::unique_lock guard(lock_);
            wake_.wait_for(guard, std::chrono::milliseconds(500), [this] { return exit_ || !jobs_.empty(); });
            if (exit_)
                break;
            work.swap(jobs_);
        }
        for (auto &job : work)
        {
            try
            {
                job();
            }
            catch (const std::exception &e)
            {
                message_ = L"ERROR: " + Wide(e.what());
                busy_ = false;
                dirty_ = true;
                if (activeAction_)
                    completedAction_ = activeAction_;
                activeAction_ = 0;
            }
        }
        try
        {
            monitor_.RefreshStats();
            Publish();
        }
        catch (const std::exception &e)
        {
            message_ = L"ERROR: " + Wide(e.what());
            busy_ = false;
            dirty_ = true;
            try
            {
                Publish();
            }
            catch (...)
            {
            }
        }
    }
    monitor_.Stop();
    {
        std::lock_guard guard(lock_);
        frame_.reset();
    }
    retired_.clear();
}
