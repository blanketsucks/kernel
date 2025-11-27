#include <loader/object.h>
#include <loader/loader.h>
#include <loader/dlfcn.h>

#include <string.h>

using Entry = int(*)(int, char**, char**);

[[gnu::naked]] void _invoke_entry(int argc, char** argv, char** environ, Entry entry) {
    asm volatile(
        "andq $-16, %rsp\n"
        "pushq $0\n"
        "jmp *%rcx\n"
    );
}

void __call_fini_functions() {
    auto executable = DynamicLoader::executable();
    executable->invoke_fini();
}

ErrorOr<Entry> link_main_executable(StringView name) {
    auto object = TRY(DynamicLoader::open_main_executable(name));

    TRY(object->perform_relocations());
    TRY(object->perform_plt_relocations());

    dbgln("Main executable {} @ {:#p}", name, object->base());
    dbgln("Loaded libraries:");
    for (auto& [name, library] : DynamicLoader::libraries()) {
        library->invoke_init();
        dbgln(" - {} @ {:#p}", name, library->base());
    }

    object->invoke_init();
    return reinterpret_cast<Entry>(object->entry());
}

int main(int argc, char** argv, char** environ) {
    if (argc < 2) {
        dbgln("Usage: {} <executable>", argv[0]);
        return 1;
    }

    DynamicLoader::add_symbol("__call_fini_functions", reinterpret_cast<uintptr_t>(&__call_fini_functions));

    DynamicLoader::add_symbol("__dlopen", reinterpret_cast<uintptr_t>(&__dlopen));
    DynamicLoader::add_symbol("__dlsym", reinterpret_cast<uintptr_t>(&__dlsym));
    DynamicLoader::add_symbol("__dlclose", reinterpret_cast<uintptr_t>(&__dlclose));

    auto result = link_main_executable(argv[1]);

    if (result.is_err()) {
        auto error = result.error();
        StringView message = error.message() ? error.message() : strerror(error.code());

        dbgln("Error loading dynamic object: {}", message);
        return 1;
    }

    Entry entry = result.value();
    _invoke_entry(argc - 1, argv + 1, environ, entry);
    
    return 0;
}