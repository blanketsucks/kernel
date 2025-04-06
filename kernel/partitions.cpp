#include <kernel/partitions.h>

namespace kernel {

MBRPartitionTable::MBRPartitionTable(BlockDevice* device) : PartitionTable(device) {
    MasterBootRecord mbr = {};
    device->read_blocks(reinterpret_cast<u8*>(&mbr), 1, 0);

    if (mbr.signature != SIGNATURE) {
        return;
    } else if (mbr.is_protective()) {
        return;
    }

    for (auto& partition : mbr.partitions) {
        if (!partition.offset) {
            continue;
        }

        m_partitions.append({ partition.offset, partition.sectors });
    }
}

}