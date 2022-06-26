#include <WindowApp.h>

WindowApp::WindowApp() : AppBase() {
    if (!glfwInit()) {
        throw std::runtime_error("could not intitialize GLFW");
    }
    uint32_t count;
    auto glfwExts = glfwGetRequiredInstanceExtensions(&count);
    for(uint32_t i=0; i<count; i++) {
        this->instance_extensions.push_back(glfwExts[i]);
    }

    this->device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

WindowApp::~WindowApp() {
    logger::info("destroying window");
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
    for (auto sem : imageAvailableSemaphores) {
        vk_device.destroySemaphore(sem);
    }
    for (auto sem : renderFinishedSemaphores) {
        vk_device.destroySemaphore(sem);
    }
    for (auto fence : inFlightFences) {
        vk_device.destroyFence(fence);
    }
    for(auto view : vk_swapchain_views) {
        vk_device.destroyImageView(view);
    }
    vk_device.destroySwapchainKHR(vk_swapchain);
    vk_instance.destroySurfaceKHR(vk_surface);
}

void WindowApp::Init(WindowAppConfig config) {
    this->config = config;
    AppBase::Init();
    this->vk_present_queue = vk_device.getQueue(vk_present_family, 0);
    createSwapchain();
    createSyncObjects();
}

Frame WindowApp::WindowFrameStart() {
    vk::resultCheck(vk_device.waitForFences({inFlightFences[flightIdx]}, VK_TRUE, UINT64_MAX), "error waiting for fence");
    vk_device.resetFences({inFlightFences[flightIdx]});
    this->imageIdx = vk_device.acquireNextImageKHR(vk_swapchain, UINT64_MAX, imageAvailableSemaphores[flightIdx]).value;

    return Frame {
        .image = vk_swapchain_images[imageIdx],
        .view = vk_swapchain_views[imageIdx],
        .imageIdx = imageIdx,
        .flightIdx = flightIdx,
    };
}

void WindowApp::WindowFrameEnd(vk::CommandBuffer cmdBuffer) {
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailableSemaphores[flightIdx],
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphores[flightIdx],
    };

    vk_graphics_queue.submit({submitInfo}, inFlightFences[flightIdx]);

    vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphores[flightIdx],
        .swapchainCount = 1,
        .pSwapchains = &vk_swapchain,
        .pImageIndices = &imageIdx,
    };

    auto rest = vk_present_queue.presentKHR(presentInfo);
    flightIdx = (flightIdx + 1) % config.framesInFlight;
}


void WindowApp::onQueueCreateInfo(std::vector<vk::DeviceQueueCreateInfo>& queueInfos) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    this->glfw_window = glfwCreateWindow(config.width, config.height, config.name, nullptr, nullptr);

    VkSurfaceKHR surface;
    glfwCreateWindowSurface(static_cast<VkInstance>(vk_instance), glfw_window, nullptr, &surface);
    this->vk_surface = vk::SurfaceKHR(surface);

    std::optional<uint32_t> presentFamily;
    uint32_t i=0;
    for(auto family : vk_physical_device.getQueueFamilyProperties()) {
        if (!presentFamily.has_value() && vk_physical_device.getSurfaceSupportKHR(i, vk_surface)) {
            presentFamily = i;
            logger::debug("present queue family: {}", i);
        }
        i++;
    }

    if (!presentFamily.has_value()) { throw std::runtime_error("device has not present support"); }

    this->vk_present_family = presentFamily.value();

    bool familyAlreadyPresent = false;
    for(auto info : queueInfos) {
        familyAlreadyPresent |= (info.queueFamilyIndex == presentFamily.value());
    }

    if (!familyAlreadyPresent) {
        logger::error("present queue is not the same family as graphics, synchronization errors incoming.");

        static float priority = 1.0f;
        queueInfos.push_back(
            vk::DeviceQueueCreateInfo {
                .queueFamilyIndex = presentFamily.value(),
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }
        );
    }
}

void WindowApp::createSwapchain() {
    vk_surface_format = chooseSurfaceFormat();
    auto presentmode = choosePresentMode();
    auto extent = chooseSwapExtent();

    logger::info("Surface Format: {}", vk::to_string(vk_surface_format.format));
    logger::info("Present Mode: {}", vk::to_string(presentmode));

    auto caps = vk_physical_device.getSurfaceCapabilitiesKHR(vk_surface);
    uint32_t imageCount = caps.minImageCount + 1;

    if (caps.maxImageCount > 0) {
        imageCount = std::min(caps.maxImageCount, imageCount);
    }

    vk::SwapchainCreateInfoKHR createInfo {
        .surface = vk_surface,
        .minImageCount = imageCount,
        .imageFormat = vk_surface_format.format,
        .imageColorSpace = vk_surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = caps.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentmode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    this->vk_swapchain = vk_device.createSwapchainKHR(createInfo);
    this->vk_swapchain_images = vk_device.getSwapchainImagesKHR(vk_swapchain);

    for(uint32_t i=0; i<imageCount; i++) {
        auto createInfo = vk::inits::imageViewCreateInfo(vk_swapchain_images[i], vk::ImageAspectFlagBits::eColor, vk_surface_format.format);
        vk_swapchain_views.push_back(vk_device.createImageView(createInfo));
    }
}

void WindowApp::createSyncObjects() {
    vk::SemaphoreCreateInfo semInfo{};
    vk::FenceCreateInfo fenceInfo{ .flags = vk::FenceCreateFlagBits::eSignaled };
    for(uint32_t i=0; i<config.framesInFlight; i++) {
        this->imageAvailableSemaphores.push_back(vk_device.createSemaphore(semInfo));
        this->renderFinishedSemaphores.push_back(vk_device.createSemaphore(semInfo));
        this->inFlightFences.push_back(vk_device.createFence(fenceInfo));
    }
}

vk::SurfaceFormatKHR WindowApp::chooseSurfaceFormat() const {
    auto formats = vk_physical_device.getSurfaceFormatsKHR(vk_surface);
    for(auto format : formats) {
        if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }

    return formats[0];
}

vk::PresentModeKHR WindowApp::choosePresentMode() const {
    auto modes = vk_physical_device.getSurfacePresentModesKHR(vk_surface);
    for (auto mode : modes) {
        if (mode == vk::PresentModeKHR::eImmediate) {
            return mode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D WindowApp::chooseSwapExtent() const {
    auto caps = vk_physical_device.getSurfaceCapabilitiesKHR(vk_surface);
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(glfw_window, &width, &height);

        vk::Extent2D extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

        return extent;
    }
}
