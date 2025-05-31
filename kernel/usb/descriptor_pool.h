#pragma once

#include <kernel/common.h>
#include <kernel/memory/manager.h>

#include <std/stack.h>
#include <std/memory.h>

namespace kernel::usb {

template<typename T>
class DescriptorPool {
public:
    static OwnPtr<DescriptorPool<T>> create() {
        return OwnPtr<DescriptorPool<T>>(new DescriptorPool<T>());
    }

    T* allocate() {
        if (m_free_descriptors.empty()) {
            return nullptr;
        }
        
        return m_free_descriptors.pop();
    }

    void free(T* descriptor) {
        if (!descriptor) {
            return;
        }
        
        m_free_descriptors.push(descriptor);
    }

private:
    DescriptorPool() {
        m_region = reinterpret_cast<T*>(MM->allocate_kernel_region(PAGE_SIZE));
        
        PhysicalAddress address = MM->get_physical_address(m_region);
        for (size_t i = 0; i < PAGE_SIZE / sizeof(T); i++) {
            auto* descriptor = &m_region[i];
            new (descriptor) T(address + i * sizeof(T));
            
            m_free_descriptors.push(descriptor);
        }
    }

    T* m_region;
    std::Stack<T*> m_free_descriptors;
};

}