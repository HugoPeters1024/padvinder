#pragma once

#include <precomp.h>
#include <Vertex.h>

class GraphicsPipelineConfig {
public:
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    vk::PipelineShaderStageCreateInfo vertexShaderInfo{};
    vk::PipelineShaderStageCreateInfo fragmentShaderInfo{};

    std::optional<vk::VertexInputBindingDescription> vertexBinding;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    std::vector<vk::DescriptorSetLayoutBinding> bindingDescriptors;

    vk::RenderPass renderPass;

    GraphicsPipelineConfig(vk::RenderPass renderPass, vk::ShaderModule vert, vk::ShaderModule frag);

    GraphicsPipelineConfig& setPrimitiveMode(vk::PrimitiveTopology mode) { inputAssembly.topology = mode; return *this; }
    GraphicsPipelineConfig& setFillMode(vk::PolygonMode mode) { rasterizer.polygonMode = mode; return *this; }
    GraphicsPipelineConfig& setLineWidth(float width) { rasterizer.lineWidth = width; return *this; }

    template<typename T>
    GraphicsPipelineConfig& enableVertexBuffer(uint32_t binding) {
        vertexBinding = vertex_info<T>::BindingDescription(binding);
        vertexAttributes = vertex_info<T>::AttributeDescriptions(binding);
        return *this;
    }

    GraphicsPipelineConfig& useUBO(uint32_t binding) {
        bindingDescriptors.push_back(vk::DescriptorSetLayoutBinding {
            .binding = binding,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        });
        return *this;
    }

    GraphicsPipelineConfig& useTexture(uint32_t binding) {
        bindingDescriptors.push_back(vk::DescriptorSetLayoutBinding {
            .binding = binding,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        });
        return *this;
    }
};
