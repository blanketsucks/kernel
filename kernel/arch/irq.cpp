#include <kernel/arch/irq.h>
#include <kernel/process/scheduler.h>
#include <kernel/arch/interrupts/manager.h>
#include <kernel/arch/processor.h>
#include <kernel/arch/registers.h>

#include <std/format.h>
#include <std/asserts.h>

namespace kernel {

static InterruptHandler* s_irq_handlers[IRQ_HANDLER_COUNT] = {};

extern "C" void _irq_handler(arch::InterruptRegisters* regs) {
    u8 irq = regs->intno - IRQ_VECTOR_BASE;
    auto* handler = s_irq_handlers[irq];

    ASSERT_MSG(handler, "No IRQ handler found for #{}", irq);

    handler->handle_interrupt();
    handler->eoi();

    Processor::current().check_scheduler_invoked_async();
}

static void register_irq_handler(InterruptHandler* handler) {
    u8 irq = handler->interrupt();
    if (irq >= IRQ_HANDLER_COUNT) {
        return;
    }

    auto& slot = s_irq_handlers[irq];
    if (!slot) {
        slot = handler;
        return;
    }

    if (slot->handler_type() == InterruptHandlerType::IRQ) {
        ASSERT(handler->handler_type() == InterruptHandlerType::IRQ);

        auto* exclusive = static_cast<IRQHandler*>(slot);
        exclusive->set_shared(true);

        auto* shared = new SharedIRQHandler(irq);

        shared->add_irq_handler(exclusive);
        shared->add_irq_handler(static_cast<IRQHandler*>(handler));

        slot = shared;
    } else if (slot->handler_type() == InterruptHandlerType::IRQShared) {
        ASSERT(handler->handler_type() == InterruptHandlerType::IRQ);

        auto* shared = static_cast<SharedIRQHandler*>(slot);
        shared->add_irq_handler(static_cast<IRQHandler*>(handler));
    }
}

static void unregister_irq_handler(InterruptHandler* handler) {
    u8 irq = handler->interrupt();
    if (irq >= IRQ_HANDLER_COUNT) {
        return;
    }

    auto& slot = s_irq_handlers[irq];
    if (!slot) {
        return;
    }

    if (slot->handler_type() == InterruptHandlerType::Generic) {
        slot = nullptr;
        return;
    } else if (slot->handler_type() == InterruptHandlerType::IRQ) {
        static_cast<IRQHandler*>(slot)->set_shared(false);
        slot = nullptr;

        return;
    }

    ASSERT(handler->handler_type() == InterruptHandlerType::IRQ);

    auto* shared = static_cast<SharedIRQHandler*>(slot);
    shared->remove_irq_handler(static_cast<IRQHandler*>(handler));

    if (!shared->handlers_count()) {
        slot = nullptr;
    }
}

void InterruptHandler::register_interrupt_handler() {
    if (m_registered) {
        return;
    }

    register_irq_handler(this);
    m_registered = true;
}

void InterruptHandler::unregister_interrupt_handler() {
    if (!m_registered) {
        return;
    }

    unregister_irq_handler(this);
    m_registered = false;
}

IRQHandlerBase::IRQHandlerBase(u8 irq) : InterruptHandler(irq), m_controller(InterruptManager::controller_for(irq)) {}

void IRQHandler::eoi() {
    if (m_is_shared) {
        return;
    }

    m_controller->eoi(interrupt());
}

void IRQHandler::enable_irq() {
    if (!is_registered()) {
        this->register_interrupt_handler();
    }

    m_enabled = true;
    if (!m_is_shared) {
        m_controller->enable(interrupt());
    }
}

void IRQHandler::disable_irq() {
    m_enabled = false;
    if (!m_is_shared) {
        m_controller->disable(interrupt());
    }
}

void SharedIRQHandler::handle_irq() {
    for (auto& handler : m_handlers) {
        handler->handle_irq();
    }
}

void SharedIRQHandler::eoi() {
    m_controller->eoi(interrupt());
}

void SharedIRQHandler::enable_irq() {
    if (m_enabled) {
        return;
    }

    m_enabled = true;
    m_controller->enable(interrupt());
}

void SharedIRQHandler::disable_irq() {
    if (!m_enabled) {
        return;
    }

    m_enabled = false;
    m_controller->disable(interrupt());
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
