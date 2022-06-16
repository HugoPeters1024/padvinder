#pragma once

#include <precomp.h>
#include <AppBase.h>

struct Frame {
    vk::Image image;
    vk::ImageView view;
    uint32_t imageIdx;
    uint32_t flightIdx;
};

struct WindowAppConfig {
    uint32_t width = 640;
    uint32_t height = 480;
    const char* name = "VulkanApp";
    uint32_t framesInFlight = 2;
};

class WindowApp : public AppBase {
public:
    GLFWwindow* glfw_window;
    vk::SurfaceKHR vk_surface;
    vk::SurfaceFormatKHR vk_surface_format;
    uint32_t vk_present_family;
    vk::Queue vk_present_queue;
    vk::SwapchainKHR vk_swapchain;
    std::vector<vk::Image> vk_swapchain_images;
    std::vector<vk::ImageView> vk_swapchain_views;
    std::vector<vk::Fence> inFlightFences;



    WindowApp();
    virtual ~WindowApp();
    virtual void Init(WindowAppConfig config);

    bool WindowShouldClose() const { return glfwWindowShouldClose(glfw_window); }
    vk::Format getSurfaceFormat() const { return vk_surface_format.format; }
    Frame WindowFrameStart();
    void WindowFrameEnd(vk::CommandBuffer cmdBuffer);

protected:
    virtual void onQueueCreateInfo(std::vector<vk::DeviceQueueCreateInfo>& queueInfos) override;

private:
    WindowAppConfig config;
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    uint32_t imageIdx;
    uint32_t flightIdx = 0;
    void createSwapchain();
    void createSyncObjects();

    vk::SurfaceFormatKHR chooseSurfaceFormat() const;
    vk::PresentModeKHR choosePresentMode() const;
    vk::Extent2D chooseSwapExtent() const;
};
