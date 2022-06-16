#pragma once
#include <precomp.h>
#include <AppBase.h>
#include <BufferTools.h>

struct Image {
    vk::Image handle;
    vk::ImageView view;
    VmaAllocation allocation{};
};

namespace ImageTools {
    inline Image CreateImageD(AppBase& app, uint32_t width, uint32_t height, vk::ImageUsageFlags usage, vk::Format format, vk::ImageLayout initialLayout) {
        auto createInfo = static_cast<VkImageCreateInfo>(vk::inits::imageCreateInfo(width, height, format, usage, vk::ImageLayout::eUndefined));
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
        VkImage c_handle;
        
        VmaAllocation allocation;
        VK_CHECK_RESULT(vmaCreateImage(app.vma_allocator, &createInfo, &allocInfo, &c_handle, &allocation, nullptr));

        auto handle = vk::Image(c_handle);

        app.WithSingleTimeCommandBuffer([&](vk::CommandBuffer cmdBuffer) {
            auto range = vk::inits::imageSubresourceRange(vk::ImageAspectFlagBits::eColor);
            vk::tools::insertImageMemoryBarrier(cmdBuffer, handle, vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eMemoryWrite, vk::ImageLayout::eUndefined, initialLayout, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe, range);
        });

        auto viewInfo = vk::inits::imageViewCreateInfo(handle, vk::ImageAspectFlagBits::eColor, format);

        return Image {
            .handle = handle,
            .view = app.vk_device.createImageView(viewInfo),
            .allocation = allocation,
        };
    }

    inline Image CreateImageD(AppBase& app, uint32_t width, uint32_t height, vk::ImageUsageFlags usage, vk::Format format, vk::ImageLayout initialLayout, void* data, size_t stride = 4) {
        auto ret = CreateImageD(app, width, height, usage | vk::ImageUsageFlagBits::eTransferDst, format, vk::ImageLayout::eTransferDstOptimal);

        const size_t imageSize = width * height * stride;
        auto staging = buffertools::CreateBufferH(app, vk::BufferUsageFlagBits::eTransferSrc, imageSize, data);

        app.WithSingleTimeCommandBuffer([&](vk::CommandBuffer cmdBuffer) {
            auto copyRegion = vk::inits::imageCopy(width, height);
            cmdBuffer.copyBufferToImage(staging.handle, ret.handle, vk::ImageLayout::eTransferDstOptimal, copyRegion);
            vk::tools::insertImageMemoryBarrier(cmdBuffer, ret.handle, vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eMemoryWrite, vk::ImageLayout::eTransferDstOptimal, initialLayout, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTopOfPipe, vk::inits::imageSubresourceRange(vk::ImageAspectFlagBits::eColor));
        });

        buffertools::DestroyBuffer(app, staging);

        return ret;
    }

    inline Image LoadImageD(AppBase& ctx, vk::ImageLayout initialLayout, vk::ImageUsageFlagBits usage, const char* filename) {
        int width, height, nrChannels;
        float* pixels = stbi_loadf(filename, &width, &height, &nrChannels, STBI_rgb_alpha);
        if (!pixels) {
            logger::error("Could not load image {}", filename);
            exit(1);
        }

        assert(nrChannels >= 3 && "Image loading should produce a 4 channel buffer");

        std::vector<float> data(width*height*4);
        for(size_t i=0; i<width*height; i++) {
            data[4*i+0] = pixels[4*i+0];
            data[4*i+1] = pixels[4*i+1];
            data[4*i+2] = pixels[4*i+2];
            data[4*i+3] = 1.0f;
        }
        stbi_image_free(pixels);

        auto ret = CreateImageD(ctx, width, height, usage, vk::Format::eR32G32B32A32Sfloat, initialLayout, data.data(), 4 * sizeof(float));
        return ret;
    }

    inline void DestroyImage(AppBase& app, Image& image) {
        app.vk_device.destroyImageView(image.view);
        vmaDestroyImage(app.vma_allocator, image.handle, image.allocation);
    }
}
