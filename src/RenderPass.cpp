#include <RenderPass.h>

RenderPass::RenderPass(AppBase& app, RenderPassInfo& config) : app(app), config(config) {
    vk::SubpassDescription subpass {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = static_cast<uint32_t>(config.attachmentReferences.size()),
        .pColorAttachments = config.attachmentReferences.data(),
    };

    vk::RenderPassCreateInfo renderPassInfo {
        .attachmentCount = static_cast<uint32_t>(config.attachments.size()),
        .pAttachments = config.attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    this->vk_render_pass = app.vk_device.createRenderPass(renderPassInfo);
}

void RenderPass::Destroy() {
    app.vk_device.destroyRenderPass(this->vk_render_pass);
}

vk::Framebuffer RenderPass::createFrameBuffer(std::vector<vk::ImageView> attachmentViews, uint32_t width, uint32_t height) {
    assert(attachmentViews.size() == config.attachments.size() && "wrong number of views");
    vk::FramebufferCreateInfo createInfo {
        .renderPass = this->vk_render_pass,
        .attachmentCount = static_cast<uint32_t>(attachmentViews.size()),
        .pAttachments = attachmentViews.data(),
        .width = width,
        .height = height,
        .layers = 1,
    };

    return app.vk_device.createFramebuffer(createInfo);
}
