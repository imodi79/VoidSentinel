#pragma once
#include "filter.h"
#include "monitor.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>

struct ViewContent
{
    std::vector<Row> data, display;
    ViewResult stats;
};
struct ViewFrame
{
    std::shared_ptr<const ViewContent> content = std::make_shared<ViewContent>();
    unsigned long long events = 0, dropped = 0, unresolved = 0, unmatched = 0;
    ULONG lost = 0, traceError = 0;
    bool running = false, busy = false;
    unsigned long long completedAction = 0;
    std::wstring message;
};
class SessionWorker
{
  public:
    SessionWorker();
    ~SessionWorker();
    unsigned long long Start(std::vector<std::wstring> targets, bool children);
    unsigned long long Stop();
    void Configure(ViewOptions options);
    void Export(std::wstring path, std::shared_ptr<const ViewFrame> frame);
    void PersistTargets(std::wstring path, std::vector<std::wstring> targets);
    std::shared_ptr<const ViewFrame> Frame();

  private:
    void Post(std::function<void()> job);
    void Loop();
    void Publish();
    Monitor monitor_;
    ViewOptions options_;
    std::mutex lock_;
    std::condition_variable wake_;
    std::deque<std::function<void()>> jobs_;
    std::shared_ptr<const ViewFrame> frame_;
    std::vector<std::shared_ptr<const ViewFrame>> retired_;
    std::thread worker_;
    bool exit_ = false, running_ = false, busy_ = false, dirty_ = true;
    unsigned long long seen_ = ~0ULL;
    std::wstring message_;
    std::atomic<unsigned long long> nextAction_{0};
    unsigned long long completedAction_ = 0;
    unsigned long long activeAction_ = 0;
};
