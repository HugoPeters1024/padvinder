#pragma once

#include <precomp.h>
#include <AppBase.h>
#include <GraphicsPipelineConfig.h>

class GraphicsPipeline {
public:
    vk::PipelineLayout layout;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::Pipeline pipeline;

    GraphicsPipeline(const AppBase& app, const GraphicsPipelineConfig& config);
    void Destroy(const AppBase& app);

    vk::DescriptorSet AllocateDescriptor(const AppBase& app) const;
};
