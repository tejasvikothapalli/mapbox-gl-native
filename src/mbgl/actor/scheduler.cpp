#include <mbgl/actor/scheduler.hpp>
#include <mbgl/util/thread_local.hpp>
#include <mbgl/util/thread_pool.hpp>

#include <cassert>

namespace mbgl {

util::ThreadLocal<Scheduler> g_currentScheduler;

// static
Scheduler& Scheduler::GetCurrent() {
    assert(g_currentScheduler.get());

    return *(g_currentScheduler.get());
}

// static
std::shared_ptr<Scheduler> Scheduler::GetBackground() {
    static std::weak_ptr<Scheduler> weak;
    static std::mutex mtx;

    std::lock_guard<std::mutex> lock(mtx);
    std::shared_ptr<Scheduler> scheduler = weak.lock();

    if (!scheduler) {
        weak = scheduler = std::make_shared<ThreadPool>(4);
    }

    return scheduler;
}

} //namespace mbgl
