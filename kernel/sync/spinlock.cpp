#include <kernel/sync/spinlock.h>

namespace kernel {

bool SpinLock::is_locked() {
    return atomic_flag_test_and_set_explicit(&this->lock, memory_order_acquire);
}

void SpinLock::acquire() {
    while (this->is_locked()) {
        __builtin_ia32_pause();
    }
}

void SpinLock::release() {
    atomic_flag_clear_explicit(&this->lock, memory_order_release);
}

}