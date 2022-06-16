#include <GraphicsPipelineConfig.h>

GraphicsPipelineConfig::GraphicsPipelineConfig(vk::RenderPass renderPass, vk::ShaderModule vert, vk::ShaderModule frag) {
    this->renderPass = renderPass;

    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState{};

    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone; 
    rasterizer.frontFace = vk::FrontFace::eClockwise;

    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    colorBlendAttachment.colorWriteMask = vk::inits::componentsRGBA();
    colorBlendAttachment.blendEnable = VK_FALSE;

    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    vertexShaderInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertexShaderInfo.module = vert;
    vertexShaderInfo.pName = "main";

    fragmentShaderInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragmentShaderInfo.module = frag;
    fragmentShaderInfo.pName = "main";
}

