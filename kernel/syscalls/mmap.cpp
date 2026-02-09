#include <kernel/process/process.h>
#include <kernel/memory/manager.h>

namespace kernel {

ErrorOr<FlatPtr> Process::sys$mmap(mmap_args* args) {
    this->validate_pointer_access(args, sizeof(mmap_args), false);

    size_t size = args->size;
    VirtualAddress hint = VirtualAddress { args->addr };
    int prot = args->prot;
    int flags = args->flags;
    int fileno = args->fd;

    size = std::align_up(size, PAGE_SIZE);

    PageFlags pflags = PageFlags::NoExecute;
    if (prot & PROT_WRITE) {
        pflags |= PageFlags::Write;
    } 

    if (prot & PROT_EXEC) {
        pflags &= ~PageFlags::NoExecute;
    }

    void* address = nullptr;
    memory::Region* region = nullptr;

    if (flags & MAP_ANONYMOUS) {
        if (flags & MAP_FIXED) {
            address = TRY(this->allocate_at(hint, size, pflags));
        } else {
            address = TRY(this->allocate(size, pflags));
        }

        if (!address) {
            return Error(ENOMEM);
        }

        region = m_allocator->find_region(VirtualAddress { address }, true);
    } else {
        if (fileno < 0 || static_cast<size_t>(fileno) >= m_file_descriptors.size()) {
            return Error(EBADF);
        }

        auto& fd = m_file_descriptors[fileno];
        if (!fd) {
            return Error(EBADF);
        }

        if (flags & MAP_FIXED) {
            region = m_allocator->create_file_backed_region(fd->file(), size, hint);
        } else {
            region = m_allocator->create_file_backed_region(fd->file(), size);
        }

        if (!region) {
            return Error(ENOMEM);
        }

        address = region->base().to_ptr();
    }

    region->set_prot(prot);
    region->set_offset(args->offset);

    if (flags & MAP_SHARED) {
        region->set_shared(true);
    }

    return (FlatPtr)address;
}

ErrorOr<FlatPtr> Process::sys$munmap(FlatPtr address, size_t size) {
    if (address % PAGE_SIZE != 0) {
        return Error(EINVAL);
    } else if (size % PAGE_SIZE != 0) {
        return Error(EINVAL);
    }

    VirtualAddress va { address };

    auto* region = m_allocator->find_region(va, true);
    if (!region) {
        return Error(EINVAL);
    }

    if (size > region->size()) {
        return Error(EINVAL);
    } else if (region->end() < address + size) {
        return Error(EINVAL);
    }

    if (region->base() == address) {
        if (region->size() == size) {
            MM->free(m_allocator->page_directory(), region->base(), region->size());
            m_allocator->free(region);

            return 0;
        }

        region->set_range({ region->offset_by(size), region->size() - size });
        MM->free(m_page_directory, va, size);

        return 0;
    } else {
        if (region->end() == address + size) {
            region->set_range({ region->base(), region->size() - size });
            MM->free(m_page_directory, va, size);

            return 0;
        }

        auto* new_region = memory::Region::create(va.offset(size), region->end() - (address + size));
        new_region->set_prot(region->prot());
        new_region->set_shared(region->is_shared());

        region->set_range({ region->base(), address - region->base() });

        m_allocator->insert_after(region, new_region);
        MM->free(m_page_directory, va, size);
    }

    return 0;
}

ErrorOr<FlatPtr> Process::sys$mmap_set_name(FlatPtr address, const char* name, size_t length) {
    auto* region = m_allocator->find_region(reinterpret_cast<void*>(address));
    if (!region) {
        return Error(EINVAL);
    }
    
    StringView str = this->validate_string(name, length);
    region->set_name(str);

    return 0;
}

}