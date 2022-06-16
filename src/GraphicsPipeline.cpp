#include <GraphicsPipeline.h>

GraphicsPipeline::GraphicsPipeline(const AppBase& app, const GraphicsPipelineConfig& config) {
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo {
        .bindingCount = static_cast<uint32_t>(config.bindingDescriptors.size()),
        .pBindings = config.bindingDescriptors.data(),
    };
    this->descriptorSetLayout = app.vk_device.createDescriptorSetLayout(descriptorSetLayoutInfo);


    vk::PipelineLayoutCreateInfo layoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &this->descriptorSetLayout
    };
    this->layout = app.vk_device.createPipelineLayout(layoutInfo);


    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &config.colorBlendAttachment
    };

    vk::PipelineViewportStateCreateInfo viewport {
        .viewportCount = 1,
        .scissorCount = 1,
    };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    std::vector<vk::PipelineShaderStageCreateInfo> stages = {
        config.vertexShaderInfo,
        config.fragmentShaderInfo,
    };

    auto vertexInputInfo = config.vertexInputInfo;

    vk::GraphicsPipelineCreateInfo pipelineInfo {
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &config.inputAssembly,
        .pViewportState = &viewport,
        .pRasterizationState = &config.rasterizer,
        .pMultisampleState = &config.multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = layout,
        .renderPass = config.renderPass,
    };

    if (config.vertexBinding.has_value()) {
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &config.vertexBinding.value();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.vertexAttributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = config.vertexAttributes.data();
    }

    this->pipeline = app.vk_device.createGraphicsPipeline(nullptr, pipelineInfo).value;
}

void GraphicsPipeline::Destroy(const AppBase& app) {
    app.vk_device.destroyPipeline(pipeline);
    app.vk_device.destroyPipelineLayout(layout);
    app.vk_device.destroyDescriptorSetLayout(descriptorSetLayout);
}

vk::DescriptorSet GraphicsPipeline::AllocateDescriptor(const AppBase& app) const {
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool = app.vk_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &this->descriptorSetLayout
    };

    return app.vk_device.allocateDescriptorSets(allocInfo)[0];
}
