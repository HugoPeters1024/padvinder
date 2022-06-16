#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <iostream>

// Macro to check and display Vulkan return results
#define VK_CHECK_RESULT(f)																				\
{																										\
    VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vk::to_string(vk::Result(res)) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n" << std::flush; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace vk {
namespace inits {

constexpr vk::ImageViewCreateInfo imageViewCreateInfo(vk::Image image, vk::ImageAspectFlags aspect, vk::Format format) {
    return vk::ImageViewCreateInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .components = ComponentMapping {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = ImageSubresourceRange {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
}

constexpr vk::ColorComponentFlags componentsRGBA() {
    return vk::ColorComponentFlagBits::eR
         | vk::ColorComponentFlagBits::eG
         | vk::ColorComponentFlagBits::eB
         | vk::ColorComponentFlagBits::eA;
}

constexpr vk::ImageSubresourceRange imageSubresourceRange(vk::ImageAspectFlags aspect) {
    return vk::ImageSubresourceRange {
        .aspectMask = aspect,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
}

constexpr vk::Viewport viewport(uint32_t width, uint32_t height) {
    return vk::Viewport {
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
}

constexpr vk::ImageCreateInfo imageCreateInfo(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, vk::ImageLayout initialLayout)
{
    vk::ImageCreateInfo imageCreateInfo {};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = format;
    imageCreateInfo.usage = usage;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal,
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.initialLayout = initialLayout;
    return imageCreateInfo;
}

constexpr vk::PipelineShaderStageCreateInfo shaderStageCreateInfo(vk::ShaderModule module, vk::ShaderStageFlagBits stage) {
    return vk::PipelineShaderStageCreateInfo {
        .stage = stage,
        .module = module,
        .pName = "main",
    };
}

constexpr vk::WriteDescriptorSet writeDescriptorSetImage(
			vk::DescriptorSet dstSet,
			vk::DescriptorType type,
			uint32_t binding,
			vk::DescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
{
    vk::WriteDescriptorSet writeDescriptorSet {};
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pImageInfo = imageInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

constexpr vk::WriteDescriptorSet writeDescriptorSetBuffer(
			vk::DescriptorSet dstSet,
			vk::DescriptorType type,
			uint32_t binding,
			vk::DescriptorBufferInfo* bufferInfo,
			uint32_t descriptorCount = 1)
{
    vk::WriteDescriptorSet writeDescriptorSet {};
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pBufferInfo = bufferInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

constexpr vk::BufferImageCopy imageCopy(uint32_t width, uint32_t height) {
    vk::BufferImageCopy region {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
            .imageOffset = {0,0,0},
            .imageExtent = {width, height, 1},
    };
    return region;
}

} 

namespace tools {

inline void insertImageMemoryBarrier(
        vk::CommandBuffer cmdBuffer,
        vk::Image image,
        vk::AccessFlags srcAccess,
        vk::AccessFlags dstAccess,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStage,
        vk::PipelineStageFlags dstStage,
        vk::ImageSubresourceRange subresourceRange) {
    vk::ImageMemoryBarrier barrier {
        .srcAccessMask = srcAccess,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = subresourceRange
    };

    cmdBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlagBits::eByRegion, {}, {}, {barrier});
}

constexpr uint32_t allignedSize(uint32_t value, uint32_t allignment) {
    return (value + allignment - 1) & ~(allignment - 1);
}

}

}
