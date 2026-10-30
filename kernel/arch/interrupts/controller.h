#pragma once

#include <kernel/common.h>

#include <std/string_view.h>

namespace kernel {

class InterruptController {
public:
    virtual ~InterruptController() = default;

    virtual void enable(u16 irq)  = 0;
    virtual void disable(u16 irq) = 0;

    virtual u16 gsi_base() const = 0;
    virtual size_t vector_count() const = 0;

    virtual void eoi(u16 irq) = 0;
    
    virtual StringView name() const = 0; 
};

}