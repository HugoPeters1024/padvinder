#pragma once
#include <precomp.h>

template<typename T>
struct vk_init {
    static void AddDeviceExtensions(std::vector<const char*>& exts);
    static void* EnableDeviceProperties(void* head);
};

class AppBase : public NoCopy {
public:
    vk::Instance vk_instance;
    vk::PhysicalDevice vk_physical_device;
    vk::Device vk_device;
    VmaAllocator vma_allocator{};
    uint32_t vk_graphics_family{};
    vk::Queue vk_graphics_queue;
    vk::CommandPool vk_graphics_pool;
    vk::DescriptorPool vk_descriptor_pool;
    vk::DispatchLoaderDynamic vk_ext_dispatcher;
    vk::Sampler vk_default_sampler;

    virtual void Init();
    vk::CommandBuffer MakeGraphicsCommandBuffer();
    void WithSingleTimeCommandBuffer(std::function<void(vk::CommandBuffer)> record);
    vk::ShaderModule LoadShader(const std::string& filename);
    AppBase() = default;
    virtual ~AppBase();

    template<typename T>
    void Require() {
        vk_init<T>::AddDeviceExtensions(device_extensions);
        this->enabled_device_features = vk_init<T>::EnableDeviceProperties(enabled_device_features);
    }

protected:
    std::vector<const char*> instance_extensions;
    std::vector<const char*> device_extensions;
    std::vector<const char*> validation_layers;
    void* enabled_device_features{};

    virtual void onQueueCreateInfo(std::vector<vk::DeviceQueueCreateInfo>& queueInfos) { throw std::runtime_error("no override"); };

private:
    void createInstance();
    void pickPhysicalDevice();
    void findQueueFamilies();
    void createDevice();
    void allocateCommandPool();
    void allocateDescriptorPool();
    void initVMA();
    void createSampler();
};
