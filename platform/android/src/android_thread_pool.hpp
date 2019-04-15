#pragma once
#include <mbgl/util/default_thread_pool.hpp>
#include <map>
#include "jni.hpp"

namespace mbgl {
namespace android {

class AndroidThreadPool : public mbgl::ThreadPool {
public:
    explicit AndroidThreadPool(size_t count);
};

} // namespace android
} // namespace mbgl
