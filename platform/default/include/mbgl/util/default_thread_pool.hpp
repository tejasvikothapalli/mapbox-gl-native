#pragma once

#include <mbgl/actor/scheduler.hpp>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>

namespace mbgl {

struct ThreadLifecycle {
    using ThreadData = void*;
    ThreadLifecycle() = default;
    ThreadLifecycle(std::function<ThreadData()> onThreadCreated_,
                    std::function<void(ThreadData)> onThreadDestroyed_) :
            onThreadCreatedFn(onThreadCreated_),
            onThreadDestroyedFn(onThreadDestroyed_) {}

    ThreadData onThreadCreated() const {
        return onThreadCreatedFn();
    }

    void onThreadDestroyed(ThreadData threadData_) const {
        onThreadDestroyedFn(threadData_);
    }

private:
    std::function<ThreadData()> onThreadCreatedFn = [] { return nullptr; };
    std::function<void(ThreadData)> onThreadDestroyedFn = [](ThreadData) {};
};

class ThreadPool : public Scheduler {
public:
    explicit ThreadPool(std::size_t count, ThreadLifecycle _lifecycle = ThreadLifecycle());
    ~ThreadPool() override;

    void schedule(std::weak_ptr<Mailbox>) override;

private:
    ThreadLifecycle lifecycle;
    std::vector<std::thread> threads;
    std::queue<std::weak_ptr<Mailbox>> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool terminate { false };
};

} // namespace mbgl
