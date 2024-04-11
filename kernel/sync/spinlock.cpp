#include <kernel/sync/spinlock.h>

namespace kernel {

// FIXME: Implement
bool SpinLock::is_locked() {
    return false;
}

void SpinLock::acquire() {
    while (this->is_locked()) {
        __builtin_ia32_pause();
    }
}

void SpinLock::release() {
    
}

}