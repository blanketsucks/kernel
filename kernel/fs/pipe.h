#pragma once

#include <kernel/fs/file.h>
#include <kernel/process/blocker.h>

#include <std/ring_buffer.h>

namespace kernel::fs {

class Pipe : public File {
public:
    static constexpr size_t SIZE = 8 * KB;

    static RefPtr<Pipe> create() {
        return RefPtr<Pipe>(new Pipe());
    }

    void add_reader() { m_readers++; }
    void add_writer() { m_writers++; }
    
    void remove_reader() { m_readers--; }
    void remove_writer() { m_writers--; }

    RefPtr<FileDescriptor> create_reader();
    RefPtr<FileDescriptor> create_writer();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    bool can_read(FileDescriptor const&) const override  { return !m_buffer->empty(); }
    bool can_write(FileDescriptor const&) const override { return !m_buffer->full(); }

    size_t size() const override { return 0; }

private:
    Pipe();

    OwnPtr<std::RingBuffer> m_buffer = nullptr;

    u32 m_readers = 0;
    u32 m_writers = 0;
};

}