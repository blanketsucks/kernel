#include <kernel/arch/irq.h>
#include <kernel/arch/pic.h>
#include <kernel/process/scheduler.h>

#include <std/format.h>

namespace kernel {

static IRQHandlerBase* s_irq_handlers[16] = {};

extern "C" void _irq_handler(arch::InterruptRegisters* regs) {
    u8 irq = regs->intno - 32;
    IRQHandlerBase* handler = s_irq_handlers[irq];

    if (!handler) {
        return pic::eoi(irq);
    }

    handler->handle_irq();
    handler->eoi();

    if (Scheduler::is_invoked_async()) {
        Scheduler::yield();
    }
}

static void register_irq_handler(IRQHandlerBase* handler) {
    u8 irq = handler->irq();
    if (irq >= 16) {
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
    if (irq >= 16) {
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

void IRQHandler::eoi() {
    if (m_is_shared) {
        return;
    }

    pic::eoi(irq());
}

void IRQHandler::enable_irq() {
    if (!is_registered()) {
        this->register_interrupt_handler();
    }

    m_enabled = true;
    if (!m_is_shared) {
        pic::enable(irq());
    }
}

void IRQHandler::disable_irq() {
    m_enabled = false;
    if (!m_is_shared) {
        pic::disable(irq());
    }
}

void SharedIRQHandler::handle_irq() {
    for (auto& handler : m_handlers) {
        handler->handle_irq();
    }
}

void SharedIRQHandler::eoi() {
    pic::eoi(irq());
}

void SharedIRQHandler::enable_irq() {
    if (m_enabled) {
        return;
    }

    m_enabled = true;
    pic::enable(irq());
}

void SharedIRQHandler::disable_irq() {
    if (!m_enabled) {
        return;
    }

    m_enabled = false;
    pic::disable(irq());
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