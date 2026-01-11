#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>

namespace kernel {

ErrorOr<FlatPtr> Process::exec(StringView path, ProcessArguments arguments) {
    {
        auto vfs = fs::vfs();

        auto file = TRY(vfs->open(path, O_RDONLY, 0, m_cwd));
        auto elf = ELF(file);

        auto* process = new Process(m_id, path, false, nullptr, nullptr, m_cwd, move(arguments), m_tty);
        process->create_user_entry(move(elf));

        process->m_file_descriptors.resize(m_file_descriptors.size());
        for (size_t i = 0; i < m_file_descriptors.size(); i++) {
            process->m_file_descriptors[i] = m_file_descriptors[i];
        }

        process->m_parent_id = m_parent_id;

        m_parent_id = 0;
        m_id = -1; // FIXME: Actually replace the current process rather than making a new one

        Scheduler::add_process(process);
    }
    
    this->kill();
    return -1;
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