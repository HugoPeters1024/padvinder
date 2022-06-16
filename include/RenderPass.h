#pragma once

#include <precomp.h>
#include <AppBase.h>

struct RenderPassInfo {
    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::AttachmentReference> attachmentReferences;

    RenderPassInfo& addColorAttachment(vk::Format format) {
        attachments.push_back(vk::AttachmentDescription {
            .format = format,
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR,
        });

        attachmentReferences.push_back(vk::AttachmentReference {
            .attachment = static_cast<uint32_t>(attachmentReferences.size()),
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        });
        return *this;
    }
};

class RenderPass {
public:
    vk::RenderPass vk_render_pass;

    RenderPass(AppBase& app, RenderPassInfo& config);
    void Destroy();

    vk::Framebuffer createFrameBuffer(std::vector<vk::ImageView> attachmentViews, uint32_t width, uint32_t height);

private:
    AppBase& app;
    RenderPassInfo config;
};
