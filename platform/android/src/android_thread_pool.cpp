#include "android_thread_pool.hpp"
#include <mbgl/util/platform.hpp>
#include <cassert>

namespace mbgl {
namespace android {

AndroidThreadPool::AndroidThreadPool(size_t count)
        : mbgl::ThreadPool(count, {
        [] {
            JNIEnv* env = nullptr;
            attach_jni_thread(theJVM, &env, platform::getCurrentThreadName());
            return env;
        },
        [](ThreadLifecycle::ThreadData threadData_) {
            assert(threadData_);
            auto env = static_cast<JNIEnv*>(threadData_);
            detach_jni_thread(theJVM, &env, true);
        }
}) {}

} // namespace android
} // namespace mbgl
