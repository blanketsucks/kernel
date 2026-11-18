#pragma once

#include <kernel/common.h>

#include <std/memory.h>
#include <std/vector.h>

namespace kernel {

class InterruptController;

enum class InterruptHandlerType : u8 {
    Generic = 0,
    IRQ = 1,
    IRQShared = 2
};

class InterruptHandler {
public:
    virtual ~InterruptHandler() = default;

    virtual void handle_interrupt() = 0;
    virtual void eoi() {}

    virtual InterruptHandlerType handler_type() const { return InterruptHandlerType::Generic; }

    void register_interrupt_handler();
    void unregister_interrupt_handler();

    u8 interrupt() const { return m_interrupt; }
    bool is_registered() const { return m_registered; }

protected:
    InterruptHandler(u8 interrupt) : m_interrupt(interrupt) {}

private:
    u8 m_interrupt;
    bool m_registered = false;
};

class IRQHandlerBase : public InterruptHandler {
public:

    virtual void handle_irq() = 0;
    void handle_interrupt() override { handle_irq(); }

    virtual void eoi() override = 0;

    InterruptController const& controller() const { return *m_controller; }

    bool is_registered() const { return m_registered; }

protected:
    IRQHandlerBase(u8 irq);

    RefPtr<InterruptController> m_controller;

private:
    bool m_registered = false;
};

class IRQHandler : public IRQHandlerBase {
public:
    IRQHandler(u8 irq) : IRQHandlerBase(irq) {}

    virtual void handle_irq() override = 0;
    void eoi() override;

    InterruptHandlerType handler_type() const override { return InterruptHandlerType::IRQ; }

    void enable_irq();
    void disable_irq();

    bool is_shared() const { return m_is_shared; }
    void set_shared(bool shared) { m_is_shared = shared; }

    bool is_enabled() const { return m_enabled; }

private:
    bool m_is_shared = false;
    bool m_enabled = false;
};

class SharedIRQHandler : public IRQHandlerBase {
public:
    SharedIRQHandler(u8 irq) : IRQHandlerBase(irq) {}

    void handle_irq() override;
    void eoi() override;

    InterruptHandlerType handler_type() const override { return InterruptHandlerType::IRQShared; }

    void enable_irq();
    void disable_irq();

    void add_irq_handler(IRQHandlerBase*);
    void remove_irq_handler(IRQHandlerBase*);

    bool is_enabled() const { return m_enabled; }
    size_t handlers_count() const { return m_handlers.size(); }

private:
    Vector<IRQHandlerBase*> m_handlers;
    bool m_enabled = false;
};

}
