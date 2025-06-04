#include <kernel/devices/gpu/device.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/ioctl.h>

namespace kernel {

ErrorOr<GPUConnector*> GPUDevice::get_connector(int id) const {
    if (id < 0 || id >= static_cast<int>(m_connectors.size())) {
        return Error(EINVAL);
    }

    auto connector = m_connectors[id];
    if (!connector) {
        return Error(EINVAL);
    }

    return connector.ptr();
}

ErrorOr<int> GPUDevice::ioctl(unsigned request, unsigned arg) {
    auto* process = Process::current();
    switch (request) {
        case GPU_GET_CONNECTOR_COUNT: {
            return static_cast<int>(m_connectors.size());
        }
        case GPU_GET_CONNECTORS: {
            gpu_connector* out = reinterpret_cast<gpu_connector*>(arg);
            process->validate_write(out, sizeof(gpu_connector) * m_connectors.size());

            for (size_t i = 0; i < m_connectors.size(); ++i) {
                auto& connector = m_connectors[i];
                auto resolution = TRY(connector->get_resolution());

                out[i].id = connector->id();
                out[i].width = resolution.width;
                out[i].height = resolution.height;
                out[i].pitch = resolution.pitch;
                out[i].bpp = resolution.bpp;
            }

            return 0;
        }
        case GPU_CONNECTOR_MAP_FB: {
            gpu_connector_map_fb* map_fb = reinterpret_cast<gpu_connector_map_fb*>(arg);
            process->validate_read(map_fb, sizeof(gpu_connector_map_fb));

            auto connector = TRY(get_connector(map_fb->id));
            map_fb->framebuffer = TRY(connector->map_framebuffer(process));

            return 0;
        }
        case GPU_CONNECTOR_FLUSH: {
            gpu_connector_flush* flush = reinterpret_cast<gpu_connector_flush*>(arg);
            process->validate_read(flush, sizeof(gpu_connector_flush));

            auto connector = TRY(get_connector(flush->id));
            TRY(connector->flush());

            return 0;
        }
        default:
            return Error(EINVAL);
    }
}

}