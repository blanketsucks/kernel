#pragma once

#include <kernel/arch/irq.h>

#include <std/function.h>

namespace kernel {

class Timer : public IRQHandler {
public:
    virtual ~Timer() = default;

    virtual void handle_irq() override {
        m_callback();
    }

    void set_callback(Function<void()> callback) {
        m_callback = move(callback);
    }

protected:
    Timer(u8 irq) : IRQHandler(irq) {}

private:
    Function<void()> m_callback = nullptr;
};

}