#pragma once

#include <kernel/arch/irq.h>

#include <std/function.h>

namespace kernel {

class TimerBase {
public:
    virtual ~TimerBase() = default;

    void set_callback(Function<void()> callback) { m_callback = move(callback); }

protected:
    Function<void()> m_callback = nullptr;
};

template<typename Handler>
class Timer;

template<>
class Timer<IRQHandler> : public TimerBase, public IRQHandler {
public:
    virtual ~Timer() = default;

    virtual void handle_irq() override { m_callback(); }

protected:
    Timer(u8 irq) : IRQHandler(irq) {}
};

template<>
class Timer<InterruptHandler> : public TimerBase, public InterruptHandler {
public:
    virtual ~Timer() = default;

    virtual void handle_interrupt() override { m_callback(); }

protected:
    Timer(u8 irq) : InterruptHandler(irq) {}
};



}
