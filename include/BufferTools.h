#pragma once
#include <precomp.h>

struct Buffer {
    vk::Buffer handle;
    VmaAllocation allocation{};
};


class AppBase;
namespace buffertools {
    Buffer CreateBufferH(AppBase& app, vk::BufferUsageFlags usage, size_t size);
    Buffer CreateBufferH(AppBase& app, vk::BufferUsageFlags usage, size_t size, void* data);

    Buffer CreateBufferH2D(AppBase& app, vk::BufferUsageFlags usage, size_t size);
    Buffer CreateBufferH2D(AppBase& app, vk::BufferUsageFlags usage, size_t size, void* data);

    Buffer CreateBufferD(AppBase& app, vk::BufferUsageFlags usage, size_t size);
    Buffer CreateBufferD(AppBase& app, vk::BufferUsageFlags usage, size_t size, void* data);

    uint64_t GetBufferDeviceAddress(AppBase& app, const Buffer& buffer);

    void* MapBuffer(AppBase& app, Buffer buffer);
    void UnmapBuffer(AppBase& app, Buffer buffer);


    void DestroyBuffer(AppBase& app, Buffer& buffer);
}
