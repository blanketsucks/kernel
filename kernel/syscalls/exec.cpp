#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>
#include <kernel/memory/manager.h>
#include <kernel/arch/interrupts.h>

namespace kernel {

ErrorOr<FlatPtr> Process::exec(StringView path, ProcessArguments arguments) {
    auto* current_thread = Thread::current();
    ASSERT(current_thread->process() == this, "Current thread should belong to this process");

   for (auto& [_, thread] : m_threads) {
        if (thread == current_thread) {
            continue;
        }

        thread->kill();
    }

    auto vfs = fs::vfs();

    auto file = TRY(vfs->open(path, O_RDONLY, 0, m_cwd));
    auto elf = ELF(file);

    m_page_directory = arch::PageDirectory::create_user_page_directory();
    m_allocator = memory::RegionAllocator::create_user_allocator(m_page_directory);

    m_arguments = move(arguments);
    m_name = path;

    auto* entry = TRY(this->create_user_entry(move(elf)));
    
    current_thread->reset_arguments_region();
    current_thread->init_exec_state(reinterpret_cast<FlatPtr>(entry), false);

    auto& processor = Processor::instance();
    processor.switch_to(current_thread);

    ASSERT(false, "Unreachable");
    __builtin_unreachable();

    return 0;
}

ErrorOr<FlatPtr> Process::sys$execve(const char* pathname, char* const argv[], char* const envp[]) {
    ProcessArguments args;
    StringView path = this->validate_string(pathname);

    if (!argv) {
        args.argv = { path };
    } else {
        size_t count = 0;
        while (argv[count]) {
            count++;
        }

        for (size_t i = 0; i < count; i++) {
            args.argv.append(this->validate_string(argv[i]));
        }
    }

    if (!envp) {
        args.envp = { nullptr };
    }

    return this->exec(path, args);
}

}
