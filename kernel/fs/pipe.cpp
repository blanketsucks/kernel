#include <kernel/fs/pipe.h>
#include <kernel/fs/fd.h>

namespace kernel::fs {

Pipe::Pipe() {
    m_buffer = std::RingBuffer::create(SIZE);
}

RefPtr<FileDescriptor> Pipe::create_reader() {
    this->add_reader();
    return FileDescriptor::create(RefPtr<File>(this), O_RDONLY);
}

RefPtr<FileDescriptor> Pipe::create_writer() {
    this->add_writer();
    return FileDescriptor::create(RefPtr<File>(this), O_WRONLY);
}

ErrorOr<size_t> Pipe::read(void* buffer, size_t size, size_t) {
    if (m_writers == 0) {
        return 0;
    } else if (m_buffer->empty()) {
        return 0;
    }

    return m_buffer->read(reinterpret_cast<u8*>(buffer), size);
}

ErrorOr<size_t> Pipe::write(const void* buffer, size_t size, size_t) {
    if (m_readers == 0) {
        return Error(EPIPE);
    } else if (m_buffer->full()) {
        return 0;
    }
 
    return m_buffer->write(reinterpret_cast<const u8*>(buffer), size);
}

}