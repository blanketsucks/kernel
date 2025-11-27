#include <loader/dlfcn.h>
#include <loader/loader.h>

extern "C" {

void* __dlopen(const char* filename, int) {
    if (filename == nullptr) {
        auto executable = DynamicLoader::executable();
        return executable.ptr();
    }

    auto result = DynamicLoader::open_library(StringView(filename));
    if (result.is_err()) {
        return nullptr;
    }

    auto object = result.value();
    return object.ptr();
}

void* __dlsym(void* hndl, const char* symbol) {
    auto* handle = reinterpret_cast<DynamicObject*>(hndl);
    if (!handle) {
        return reinterpret_cast<void*>(DynamicLoader::get_symbol_address(symbol));
    }

    Elf64_Sym* sym = handle->find_symbol(symbol);
    if (!sym) {
        return nullptr;
    }

    return reinterpret_cast<void*>(handle->base() + sym->st_value);
}

int __dlclose(void*) { return 0; }

}