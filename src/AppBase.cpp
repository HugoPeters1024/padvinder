#include <AppBase.h>

 
void AppBase::Init() {
    createInstance();
    pickPhysicalDevice();
    findQueueFamilies();
    createDevice();
    allocateCommandPool();
    allocateDescriptorPool();
    initVMA();
    createSampler();
}

vk::CommandBuffer AppBase::MakeGraphicsCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo {
        .commandPool = vk_graphics_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };

    return vk_device.allocateCommandBuffers(allocInfo)[0];
}

void AppBase::WithSingleTimeCommandBuffer(std::function<void(vk::CommandBuffer)> record) {
    auto cmdBuffer = MakeGraphicsCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit, };
    cmdBuffer.begin(beginInfo);
    record(cmdBuffer);
    cmdBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
    };
    vk_graphics_queue.submit(submitInfo);
    vk_graphics_queue.waitIdle();
}

vk::ShaderModule AppBase::LoadShader(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> code(fileSize);
    file.seekg(0);
    file.read(code.data(), fileSize);
    file.close();

    vk::ShaderModuleCreateInfo createInfo {
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    return vk_device.createShaderModule(createInfo);
}

AppBase::~AppBase() {
    logger::info("destroying app");
    vk_device.destroySampler(vk_default_sampler);
    vmaDestroyAllocator(vma_allocator);
    vk_device.destroyDescriptorPool(vk_descriptor_pool);
    vk_device.destroyCommandPool(vk_graphics_pool);
    vk_device.destroy();
    vk_instance.destroy();
}

void AppBase::createInstance() {
#ifndef NDEBUG
    validation_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    vk::ApplicationInfo applicationInfo {
        .pApplicationName = "App",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Engine",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    vk::InstanceCreateInfo createInfo {
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames = validation_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
        .ppEnabledExtensionNames = instance_extensions.data(),
    };

    this->vk_instance = vk::createInstance(createInfo);
    logger::debug("vk_instance created");
}

void AppBase::pickPhysicalDevice() {
    uint32_t deviceCount;
    auto devices = vk_instance.enumeratePhysicalDevices();
    for (auto device : devices) {
        logger::debug("device: {}", device.getProperties().deviceName);
    }

    if (devices.empty()) {
        throw std::runtime_error("No vulkan capable devices detected");
    }

    this->vk_physical_device = devices[0];
    logger::debug("picked device: {}", vk_physical_device.getProperties().deviceName);
}

void AppBase::findQueueFamilies() {
    uint32_t i=0;
    std::optional<uint32_t> graphics;

    for(auto family : vk_physical_device.getQueueFamilyProperties()) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            if (!graphics.has_value()) {
                graphics = i;
                logger::debug("graphics queue family: {}", i);
            }
        }

        i++;
    }

    if (!graphics.has_value()) { throw std::runtime_error("No graphics family on device"); }
    this->vk_graphics_family = graphics.value();
}

void AppBase::createDevice() {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    float priority = 1.0f;
    queueCreateInfos.push_back(
        vk::DeviceQueueCreateInfo {
            .queueFamilyIndex = this->vk_graphics_family,
            .queueCount = 1,
            .pQueuePriorities = &priority,
        }
    );

    this->onQueueCreateInfo(queueCreateInfos);

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::DeviceCreateInfo createInfo {
        .pNext = this->enabled_device_features,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames = validation_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    logger::info("enabled layers:");
    for(const auto& layer : validation_layers) {
        logger::info("- {}", layer);
    }

    logger::info("enabled device extensions:");
    for(const auto& ext : device_extensions) {
        logger::info("-  {}", ext);
    }


    this->vk_device = vk_physical_device.createDevice(createInfo);
    logger::debug("created logical device");

    this->vk_graphics_queue = vk_device.getQueue(vk_graphics_family, 0);

    this->vk_ext_dispatcher = vk::DispatchLoaderDynamic(vk_instance, glfwGetInstanceProcAddress, vk_device);
}

void AppBase::allocateCommandPool() {
    vk::CommandPoolCreateInfo createInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = vk_graphics_family,
    };

    this->vk_graphics_pool = vk_device.createCommandPool(createInfo);
}

void AppBase::allocateDescriptorPool() {
    vk::DescriptorPoolSize pool_sizes[] =
	{
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};

    vk::DescriptorPoolCreateInfo createInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
        .poolSizeCount = std::size(pool_sizes),
        .pPoolSizes = pool_sizes
    };

    this->vk_descriptor_pool = vk_device.createDescriptorPool(createInfo);
}

void AppBase::initVMA() {
    VmaAllocatorCreateInfo createInfo {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = vk_physical_device,
        .device = vk_device,
        .instance = vk_instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    vmaCreateAllocator(&createInfo, &this->vma_allocator);
}

void AppBase::createSampler() {
    vk::SamplerCreateInfo createInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
    };
    this->vk_default_sampler = vk_device.createSampler(createInfo);
}
