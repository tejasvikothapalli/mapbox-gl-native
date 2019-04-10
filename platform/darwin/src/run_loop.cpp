#include <mbgl/actor/scheduler.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/thread_local.hpp>

#include <CoreFoundation/CoreFoundation.h>

namespace mbgl {

extern util::ThreadLocal<Scheduler> g_currentScheduler;

namespace util {

class RunLoop::Impl {
public:
    std::unique_ptr<AsyncTask> async;
};

RunLoop* RunLoop::Get() {
    assert(g_currentScheduler.get());
    return static_cast<RunLoop*>(g_currentScheduler.get());
}

RunLoop::RunLoop(Type)
  : impl(std::make_unique<Impl>()) {
    assert(!g_currentScheduler.get());
    g_currentScheduler.set(this);
    impl->async = std::make_unique<AsyncTask>(std::bind(&RunLoop::process, this));
}

RunLoop::~RunLoop() {
    assert(g_currentScheduler.get());
    g_currentScheduler.set(nullptr);
}

void RunLoop::wake() {
    impl->async->send();
}

void RunLoop::run() {
    CFRunLoopRun();
}

void RunLoop::runOnce() {
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
}

void RunLoop::stop() {
    invoke([&] { CFRunLoopStop(CFRunLoopGetCurrent()); });
}

} // namespace util
} // namespace mbgl
