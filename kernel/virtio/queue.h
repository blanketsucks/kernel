#pragma once

#include <kernel/virtio/virtio.h>

#include <std/memory.h>
#include <std/optional.h>

namespace kernel::virtio {

class Queue {
public:
    class Chain {
    public:
        Chain(Queue* queue) : m_queue(queue) {};
        Chain(Queue* queue, u16 start, u16 end, size_t length) : m_queue(queue), m_start(start), m_end(end), m_length(length) {};

        size_t length() const { return m_length; }

        void reset();

        void add_buffer(PhysicalAddress address, size_t length, bool writable = false);
        void submit();

        void release();

        template<typename F>
        void for_each_buffer(F&& callback) {
            if (!m_start.has_value()) {
                return;
            }

            u16 index = m_start.value();
            for (size_t i = 0; i < m_length; i++) {
                auto& descriptor = m_queue->m_descriptors[index];
                callback(descriptor.address, descriptor.length);

                index = descriptor.next;
            }

        }

    private:
        Queue* m_queue;

        Optional<u16> m_start;
        u16 m_end;
        size_t m_length;
    };

    static OwnPtr<Queue> create(u16 size, u16 notify_offset);

    u16 size() const { return m_size; }
    u16 notify_offset() const { return m_notify_offset; }

    bool has_available_data() const;
    void drain();

    u16 find_free_descriptor();

    Chain create_chain() { return Chain(this); }
    void reclaim(u16 start, u16 end, size_t length);

    Chain dequeue();
    
    QueueDescriptor* descriptors() { return m_descriptors; }
    QueueDriver* driver() { return m_driver; }
    QueueDevice* device() { return m_device; }

    PhysicalAddress get_physical_address(void* ptr);
private:

    Queue(u16 size, u16 notify_offset);

    u16 m_size;
    u16 m_notify_offset;

    u16 m_free_index;
    u16 m_driver_index;
    u16 m_used_index;

    u8* m_buffer;

    QueueDescriptor* m_descriptors;
    QueueDriver* m_driver;
    QueueDevice* m_device;
};


}