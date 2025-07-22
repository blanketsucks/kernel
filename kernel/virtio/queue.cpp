#include <kernel/virtio/queue.h>
#include <kernel/memory/manager.h>

namespace kernel::virtio {

OwnPtr<Queue> Queue::create(u16 size, u16 notify_offset) {
    return OwnPtr<Queue>(new Queue(size, notify_offset));
}

Queue::Queue(u16 size, u16 notify_offset) : m_size(size), m_notify_offset(notify_offset) {
    size_t descriptor_size = sizeof(QueueDescriptor) * size;
    size_t driver_size = sizeof(QueueDriver) + sizeof(u16) * size;
    size_t device_size = sizeof(QueueDevice) + sizeof(QueueDeviceElement) * size;

    size_t total_size = std::align_up(descriptor_size + driver_size + device_size, PAGE_SIZE);

    m_buffer = reinterpret_cast<u8*>(MUST(MM->allocate_kernel_region(total_size)));
    memset(m_buffer, 0, total_size);

    m_descriptors = reinterpret_cast<QueueDescriptor*>(m_buffer);
    m_driver = reinterpret_cast<QueueDriver*>(m_buffer + descriptor_size);
    m_device = reinterpret_cast<QueueDevice*>(m_buffer + descriptor_size + driver_size);

    for (size_t i = 0; i < size; ++i) {
        m_descriptors[i].next = (i + 1) % size;
        m_free_descriptors.push(size - i - 1);
    }

    m_driver->flags = 0;
}

bool Queue::has_available_data() const {
    return m_used_index != m_device->index;
}

void Queue::drain() {
    auto chain = this->dequeue();
    while (chain.length()) {
        chain.release();
        chain = this->dequeue();
    }
}

PhysicalAddress Queue::get_physical_address(void* ptr) {
    size_t offset = reinterpret_cast<u8*>(ptr) - m_buffer;
    PhysicalAddress address = MM->get_physical_address(m_buffer);

    return address + offset;
}

u16 Queue::find_free_descriptor() {
    if (m_free_descriptors.empty()) {
        return -1; // TODO: Return an Optional
    }

    return m_free_descriptors.pop();
}

Queue::Chain Queue::dequeue() {
    if (!this->has_available_data()) {
        return Chain(this, 0, 0, 0);
    }

    QueueDeviceElement item = m_device->ring[m_used_index];

    u16 start = item.id;
    u16 end = item.id;

    size_t length = 1;

    while (true) {
        auto& descriptor = m_descriptors[end];
        if (!(descriptor.flags & QueueDescriptor::Next)) {
            break;
        }

        end = descriptor.next;
        length++;
    }

    m_used_index = (m_used_index + 1) % m_size;
    return Chain(this, start, end, length);
}

void Queue::reclaim(u16 start, u16, size_t length) {
    u16 index = start;
    for (size_t i = 0; i < length; i++) {
        auto& descriptor = m_descriptors[index];

        m_free_descriptors.push(index);
        index = descriptor.next;
    }
}

void Queue::Chain::reset() {
    m_start = {};
    m_end = {};
    m_length = 0;
}

void Queue::Chain::add_buffer(PhysicalAddress address, size_t length, bool writable) {
    u16 index = m_queue->find_free_descriptor();
    if (!m_start.has_value()) {
        m_start = index;
    } else {
        auto& descriptor = m_queue->descriptors()[m_end];

        descriptor.flags |= QueueDescriptor::Next;
        descriptor.next = index;
    }

    m_end = index;
    m_length++;

    auto& descriptor = m_queue->descriptors()[index];

    descriptor.address = address;
    descriptor.length = length;
    descriptor.flags = writable ? QueueDescriptor::Write : 0;
}

void Queue::Chain::submit() {
    if (!m_start.has_value()) {
        return;
    }

    u16 next = m_queue->m_driver_index;
    m_queue->m_driver->ring[next] = m_start.value();

    m_queue->m_driver_index = (m_queue->m_driver_index + 1) % m_queue->m_size;
    m_queue->m_driver->index = m_queue->m_driver_index;

    this->reset();
}

void Queue::Chain::release() {
    if (!m_start.has_value()) {
        return;
    }

    m_queue->reclaim(m_start.value(), m_end, m_length);
    this->reset();
}

}