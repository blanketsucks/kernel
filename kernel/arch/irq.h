#pragma once

#include <kernel/common.h>
#include <std/vector.h>

namespace kernel {

enum class IRQHandlerType : u8 {
    Exclusive = 1,
    Shared = 2
};

class IRQHandlerBase {
public:
    virtual ~IRQHandlerBase() = default;

    virtual void handle_irq() = 0;
    virtual void eoi() = 0;

    virtual IRQHandlerType handler_type() const = 0;

    void register_interrupt_handler();
    void unregister_interrupt_handler();

    u8 irq() const { return m_irq; }
    bool is_registered() const { return m_registered; }

protected:
    IRQHandlerBase(u8 irq) : m_irq(irq) {}

private:
    u8 m_irq;
    bool m_registered = false;
};

class IRQHandler : public IRQHandlerBase {
public:
    IRQHandler(u8 irq) : IRQHandlerBase(irq) {}

    virtual void handle_irq() override = 0;
    void eoi() override;

    IRQHandlerType handler_type() const override { return IRQHandlerType::Exclusive; }

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

    IRQHandlerType handler_type() const override { return IRQHandlerType::Shared; }

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