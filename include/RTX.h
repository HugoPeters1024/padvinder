#pragma once

#include <precomp.h>
#include <AppBase.h>
#include <RTXConfig.h>
#include <BufferTools.h>
#include <ImageTools.h>
#include <Scene.h>

struct RTXAccelerationStructure {
    vk::AccelerationStructureKHR handle;
    Buffer buffer;
};

struct InstanceData {
    std::vector<GLTFPrimitive> primitives;
    Buffer vertex_buffer;
    Buffer index_buffer;
    Buffer transformation_buffer;
    RTXAccelerationStructure acceleration_structure;
    uint32_t geometry_offset;
};

struct UniformData {
    glm::mat4 proj;
    glm::mat4 projInverse;
    glm::mat4 view;
    glm::mat4 viewInverse;
    float time;
    uint32_t tick;
};

class RTX {
public:
    vk::PipelineLayout pipeline_layout;
    vk::DescriptorSetLayout descr_layout;
    vk::Pipeline pipeline;
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR pipeline_properties;
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features;
    Image storage_image;
    vk::DescriptorSet descriptor_set;

    RTX(AppBase& app, Scene& scene, RTXConfig& config);
    void Destroy();
    void Record(vk::CommandBuffer cmdBuffer, uint32_t tick, glm::mat4 viewMatrix);

    vk::Sampler CreateStorageImageSampler();

private:
    AppBase& app;
    Scene& scene;
    RTXConfig config;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;


    struct {
        Buffer raygen;
        uint64_t raygen_address;
        Buffer miss;
        uint64_t miss_address;
        Buffer hit;
        uint64_t hit_address;
    } binding_table;

    struct {
        std::vector<InstanceData> instances;
        std::vector<Image> textures;
        RTXAccelerationStructure top;
        Buffer uniform_buffer;
        UniformData* uniform_buffer_data;
        Buffer material_buffer;
        Image skybox;
    } resources;

    void getProperties();
    void createBottomLevelAS();
    void createTopLevelAS();
    void createMaterialBuffer();
    void createTextureBuffer();
    void createStorageImage();
    void createPipeline();
    void createShaderBindingTable();
    void createDescriptorSet();
};

template <>
struct vk_init<RTX> {
    static void AddDeviceExtensions(std::vector<const char*>& exts) {
        exts.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        exts.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        exts.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        exts.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        exts.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        exts.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
        exts.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

        exts.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    }

    static void* EnableDeviceProperties(void* head) {
        static vk::PhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures;
        static vk::PhysicalDeviceRayTracingPipelineFeaturesKHR pipelineFeatures;
        static vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
        static vk::PhysicalDeviceDescriptorIndexingFeatures deviceFeatures;

        deviceFeatures.runtimeDescriptorArray = VK_TRUE;
        deviceFeatures.pNext = head;

        bufferAddressFeatures.bufferDeviceAddress = VK_TRUE;
        bufferAddressFeatures.pNext = &deviceFeatures;

        pipelineFeatures.rayTracingPipeline = VK_TRUE;
        pipelineFeatures.pNext = &bufferAddressFeatures;

        accelerationStructureFeatures.accelerationStructure = VK_TRUE;
        accelerationStructureFeatures.pNext = &pipelineFeatures;

        return &accelerationStructureFeatures;
    }
};
