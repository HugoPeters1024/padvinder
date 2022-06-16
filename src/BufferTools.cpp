#include <BufferTools.h>
#include <AppBase.h>

namespace buffertools {
    Buffer CreateBufferH(AppBase& app, vk::BufferUsageFlags usage, size_t size) {
        Buffer ret;
        auto bufferInfo = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo { .size = size, .usage = usage });
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_ONLY };
        VkBuffer retBuffer;
        vmaCreateBuffer(app.vma_allocator, &bufferInfo, &allocInfo, &retBuffer, &ret.allocation, nullptr);
        ret.handle = vk::Buffer(retBuffer);
        return ret;
    }

    Buffer CreateBufferH(AppBase& app, vk::BufferUsageFlags usage, size_t size, void* data) {
        Buffer ret = CreateBufferH(app, usage, size);
        auto dst = MapBuffer(app, ret);
        memcpy(dst, data, size);
        UnmapBuffer(app, ret);
        return ret;
    }

    Buffer CreateBufferH2D(AppBase& app, vk::BufferUsageFlags usage, size_t size) {
        Buffer ret;
        auto bufferInfo = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo { .size = size, .usage = usage });
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
        VkBuffer retBuffer;
        vmaCreateBuffer(app.vma_allocator, &bufferInfo, &allocInfo, &retBuffer, &ret.allocation, nullptr);
        ret.handle = vk::Buffer(retBuffer);
        return ret;
    }

    Buffer CreateBufferH2D(AppBase& app, vk::BufferUsageFlags usage, size_t size, void* data) {
        auto ret = CreateBufferH2D(app, usage, size);
        void* data_dst;
        vmaMapMemory(app.vma_allocator, ret.allocation, &data_dst);
        memcpy(data_dst, data, size);
        vmaUnmapMemory(app.vma_allocator, ret.allocation);
        return ret;
    }

    Buffer CreateBufferD(AppBase& app, vk::BufferUsageFlags usage, size_t size) {
        Buffer ret;
        auto bufferInfo = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo { .size = size, .usage = usage });
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
        VkBuffer retBuffer;
        vmaCreateBuffer(app.vma_allocator, &bufferInfo, &allocInfo, &retBuffer, &ret.allocation, nullptr);
        ret.handle = vk::Buffer(retBuffer);
        return ret;
    }

    Buffer CreateBufferD(AppBase& app, vk::BufferUsageFlags usage, size_t size, void* data) {
        Buffer ret = CreateBufferD(app, usage | vk::BufferUsageFlagBits::eTransferDst, size);
        Buffer staging = CreateBufferH(app, vk::BufferUsageFlagBits::eTransferSrc, size, data);

        app.WithSingleTimeCommandBuffer([&](vk::CommandBuffer cmdBuffer) {
            logger::info("Transfering {} bytes to the gpu for buffer initialisation", size);
            vk::BufferCopy copyRegion { .size = size };
            cmdBuffer.copyBuffer(staging.handle, ret.handle, copyRegion);
        });

        DestroyBuffer(app, staging);

        return ret;
    }

    uint64_t GetBufferDeviceAddress(AppBase& app, const Buffer& buffer) {
        vk::BufferDeviceAddressInfoKHR info { .buffer = buffer.handle };
        return app.vk_device.getBufferAddressKHR(info, app.vk_ext_dispatcher);
    }

    void DestroyBuffer(AppBase& app, Buffer& buffer) {
        vmaDestroyBuffer(app.vma_allocator, buffer.handle, buffer.allocation);
    }

    void* MapBuffer(AppBase& app, Buffer buffer) {
        void* ret;
        vmaMapMemory(app.vma_allocator, buffer.allocation, &ret);
        return ret;
    }

    void UnmapBuffer(AppBase& app, Buffer buffer) {
        vmaUnmapMemory(app.vma_allocator, buffer.allocation);
    }
}
