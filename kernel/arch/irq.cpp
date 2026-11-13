#include <kernel/arch/irq.h>
#include <kernel/process/scheduler.h>
#include <kernel/arch/interrupts/manager.h>
#include <kernel/arch/processor.h>

#include <std/format.h>

namespace kernel {

static IRQHandlerBase* s_irq_handlers[IRQ_HANDLER_COUNT] = {};

extern "C" void _irq_handler(arch::InterruptRegisters* regs) {
    u8 irq = regs->intno - IRQ_VECTOR_BASE;
    IRQHandlerBase* handler = s_irq_handlers[irq];

    ASSERT_MSG(handler, "No IRQ handler found for #{}", irq);

    handler->handle_irq();
    handler->eoi();

    Processor::current().check_scheduler_invoked_async();
}

static void register_irq_handler(IRQHandlerBase* handler) {
    u8 irq = handler->irq();
    if (irq >= IRQ_HANDLER_COUNT) {
        return;
    }

    auto& slot = s_irq_handlers[irq];
    if (!slot) {
        slot = handler;
        return;
    }

    if (slot->handler_type() == IRQHandlerType::Exclusive) {
        auto* exclusive = static_cast<IRQHandler*>(slot);
        exclusive->set_shared(true);

        auto* shared = new SharedIRQHandler(irq);

        shared->add_irq_handler(exclusive);
        shared->add_irq_handler(handler);

        slot = shared;
    } else {
        // Shared handler
        static_cast<SharedIRQHandler*>(slot)->add_irq_handler(handler);
    }
}

static void unregister_irq_handler(IRQHandlerBase* handler) {
    u8 irq = handler->irq();
    if (irq >= IRQ_HANDLER_COUNT) {
        return;
    }

    auto& slot = s_irq_handlers[irq];
    if (!slot) {
        return;
    }

    if (slot->handler_type() == IRQHandlerType::Exclusive) {
        static_cast<IRQHandler*>(slot)->set_shared(false);
        slot = nullptr;

        return;
    }

    auto* shared = static_cast<SharedIRQHandler*>(slot);
    shared->remove_irq_handler(handler);

    if (!shared->handlers_count()) {
        slot = nullptr;
    }
}

IRQHandlerBase::IRQHandlerBase(u8 irq) : m_controller(InterruptManager::controller_for(irq)), m_irq(irq) {}

void IRQHandlerBase::register_interrupt_handler() {
    if (m_registered) {
        return;
    }

    register_irq_handler(this);
    m_registered = true;
}

void IRQHandlerBase::unregister_interrupt_handler() {
    if (!m_registered) {
        return;
    }

    unregister_irq_handler(this);
    m_registered = false;
}

// TODO: Make another class for interrupts that don't have a corresponding controller (such as the APIC Timer)
InterruptController& IRQHandlerBase::ensure_controller() {
    ASSERT_MSG(m_controller, "No interrupt controller was set for IRQ #{}.", irq());
    return *m_controller;
}

void IRQHandler::eoi() {
    if (m_is_shared) {
        return;
    }

    ensure_controller().eoi(irq());
}

void IRQHandler::enable_irq() {
    if (!is_registered()) {
        this->register_interrupt_handler();
    }

    m_enabled = true;
    if (!m_is_shared) {
        ensure_controller().enable(irq());
    }
}

void IRQHandler::disable_irq() {
    m_enabled = false;
    if (!m_is_shared) {
        ensure_controller().disable(irq());
    }
}

void SharedIRQHandler::handle_irq() {
    for (auto& handler : m_handlers) {
        handler->handle_irq();
    }
}

void SharedIRQHandler::eoi() {
    ensure_controller().eoi(irq());
}

void SharedIRQHandler::enable_irq() {
    if (m_enabled) {
        return;
    }

    m_enabled = true;
    ensure_controller().enable(irq());
}

void SharedIRQHandler::disable_irq() {
    if (!m_enabled) {
        return;
    }

    m_enabled = false;
    ensure_controller().disable(irq());
}

void SharedIRQHandler::add_irq_handler(IRQHandlerBase* handler) {
    m_handlers.append(handler);
    this->enable_irq();
}

void SharedIRQHandler::remove_irq_handler(IRQHandlerBase* handler) {
    auto iterator = m_handlers.find(handler);
    if (iterator != m_handlers.end()) {
        m_handlers.remove(iterator);
    }

    if (m_handlers.empty()) {
        this->disable_irq();
    }
}


}
