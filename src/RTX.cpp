#include <RTX.h>

RTX::RTX(AppBase& app, Scene& scene, RTXConfig& config) : app(app), config(config), scene(scene) {
    getProperties();
    resources.skybox = ImageTools::LoadImageD(app, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageUsageFlagBits::eSampled, "./skybox.jpg");
    createBottomLevelAS();
    createTopLevelAS();
    createMaterialBuffer();
    createTextureBuffer();
    createStorageImage();
    createPipeline();
    createShaderBindingTable();
    createDescriptorSet();
}

void RTX::Destroy() {
    ImageTools::DestroyImage(app, storage_image);
    ImageTools::DestroyImage(app, resources.skybox);


    for(auto& instance : resources.instances) {
        app.vk_device.destroyAccelerationStructureKHR(instance.acceleration_structure.handle, nullptr, app.vk_ext_dispatcher);
        buffertools::DestroyBuffer(app, instance.acceleration_structure.buffer);
        buffertools::DestroyBuffer(app, instance.vertex_buffer);
        buffertools::DestroyBuffer(app, instance.index_buffer);
        buffertools::DestroyBuffer(app, instance.transformation_buffer);
    }

    for(auto& texture : resources.textures) {
        ImageTools::DestroyImage(app, texture);
    }

    buffertools::UnmapBuffer(app, resources.uniform_buffer);
    buffertools::DestroyBuffer(app, resources.uniform_buffer);
    app.vk_device.destroyAccelerationStructureKHR(resources.top.handle, nullptr, app.vk_ext_dispatcher);
    buffertools::DestroyBuffer(app, resources.top.buffer);
    buffertools::DestroyBuffer(app, resources.material_buffer);



    buffertools::DestroyBuffer(app, binding_table.raygen);
    buffertools::DestroyBuffer(app, binding_table.miss);
    buffertools::DestroyBuffer(app, binding_table.hit);
    app.vk_device.destroyPipeline(this->pipeline);
    app.vk_device.destroyPipelineLayout(this->pipeline_layout);
    app.vk_device.destroyDescriptorSetLayout(this->descr_layout);
}

void RTX::Record(vk::CommandBuffer cmdBuffer, uint32_t tick, glm::mat4 viewMatrix) {
    float aspectRatio = static_cast<float>(config.width) / static_cast<float>(config.height);
    resources.uniform_buffer_data->proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.0001f, 10000.0f);
    resources.uniform_buffer_data->proj[1][1] *= -1;
    resources.uniform_buffer_data->projInverse = glm::inverse(resources.uniform_buffer_data->proj);
    resources.uniform_buffer_data->view = viewMatrix;
    resources.uniform_buffer_data->viewInverse = glm::inverse(viewMatrix);
    resources.uniform_buffer_data->time = static_cast<float>(glfwGetTime());
    resources.uniform_buffer_data->tick = tick;
    const uint32_t handleSizeAlligned = vk::tools::allignedSize(pipeline_properties.shaderGroupHandleSize, pipeline_properties.shaderGroupHandleAlignment);
    vk::StridedDeviceAddressRegionKHR raygenEntry { .deviceAddress = binding_table.raygen_address, .stride = handleSizeAlligned, .size = handleSizeAlligned };
    vk::StridedDeviceAddressRegionKHR missEntry { .deviceAddress = binding_table.miss_address, .stride = handleSizeAlligned, .size = handleSizeAlligned };
    vk::StridedDeviceAddressRegionKHR hitEntry { .deviceAddress = binding_table.hit_address, .stride = handleSizeAlligned, .size = handleSizeAlligned };
    vk::StridedDeviceAddressRegionKHR callableEntry{};

    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline);
    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipeline_layout, 0, descriptor_set, {});
    cmdBuffer.traceRaysKHR(&raygenEntry, &missEntry, &hitEntry, &callableEntry, config.width, config.height, 1, app.vk_ext_dispatcher);
}

vk::Sampler RTX::CreateStorageImageSampler() {
    vk::SamplerCreateInfo createInfo {
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
    };

    return app.vk_device.createSampler(createInfo);
}

void RTX::getProperties() {
    vk::PhysicalDeviceProperties2 deviceProperties{};
    deviceProperties.pNext = &pipeline_properties;
    app.vk_physical_device.getProperties2(&deviceProperties);

    vk::PhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.pNext = &acceleration_structure_features;
    app.vk_physical_device.getFeatures2(&deviceFeatures);
}

void RTX::createStorageImage() {
    this->storage_image = ImageTools::CreateImageD(app, config.width, config.height, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void RTX::createTextureBuffer() {
    for(const auto& texture : scene.textures) {
        resources.textures.push_back(ImageTools::CreateImageD(app, texture.width, texture.height, vk::ImageUsageFlagBits::eSampled, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eShaderReadOnlyOptimal, (void*)texture.data.data()));
    }
}


void RTX::createBottomLevelAS() {
    uint32_t runningGeometryCount = 0;
    for(const auto& mesh : scene.meshes) {
        InstanceData instanceData{};
        instanceData.primitives = mesh.primitives;
        instanceData.geometry_offset = runningGeometryCount;
        runningGeometryCount += mesh.primitives.size();

        std::vector<vk::AccelerationStructureGeometryKHR> meshGeometries;
        std::vector<uint32_t> meshGeometiesTriangleCounts;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> meshGeomtriesBuildRanges;

        const auto usage = vk::BufferUsageFlagBits::eShaderDeviceAddress;
        instanceData.vertex_buffer = buffertools::CreateBufferD(app, usage | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eStorageBuffer, mesh.vertices.size() * sizeof(GLTFVertex), (void*)mesh.vertices.data());
        vk::DeviceOrHostAddressConstKHR vertexBufferAddress { .deviceAddress = buffertools::GetBufferDeviceAddress(app, instanceData.vertex_buffer), };
        instanceData.index_buffer = buffertools::CreateBufferD(app, usage | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eStorageBuffer, mesh.indices.size() * sizeof(uint32_t), (void*)mesh.indices.data());
        vk::DeviceOrHostAddressConstKHR indexBufferAddress { .deviceAddress = buffertools::GetBufferDeviceAddress(app, instanceData.index_buffer), };

        std::vector<vk::TransformMatrixKHR> transformMatrices;
        for(const auto& primitive : mesh.primitives) {
            transformMatrices.push_back(vk::TransformMatrixKHR {
                .matrix = std::array<std::array<float,4>,3> {
                    std::array<float,4> { primitive.transform[0][0], primitive.transform[1][0], primitive.transform[2][0], primitive.transform[3][0] },
                    std::array<float,4> { primitive.transform[0][1], primitive.transform[1][1], primitive.transform[2][1], primitive.transform[3][1] },
                    std::array<float,4> { primitive.transform[0][2], primitive.transform[1][2], primitive.transform[2][2], primitive.transform[3][2] },
                }
            });
        }
        instanceData.transformation_buffer = buffertools::CreateBufferD(app, usage | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, transformMatrices.size() * sizeof(vk::TransformMatrixKHR), transformMatrices.data());
        vk::DeviceOrHostAddressConstKHR transformationBufferAddress { .deviceAddress = buffertools::GetBufferDeviceAddress(app, instanceData.transformation_buffer), };



        for(uint32_t primitiveID = 0; primitiveID < mesh.primitives.size(); primitiveID++) {
            const auto& primitive = mesh.primitives[primitiveID];
            vk::AccelerationStructureGeometryKHR geometry {
                .geometryType = vk::GeometryTypeKHR::eTriangles,
                .geometry = vk::AccelerationStructureGeometryDataKHR {
                    .triangles = vk::AccelerationStructureGeometryTrianglesDataKHR {
                        .vertexFormat = vk::Format::eR32G32B32Sfloat,
                        .vertexData = vertexBufferAddress,
                        .vertexStride = sizeof(GLTFVertex),
                        .maxVertex = primitive.vertex_offset + primitive.vertex_count,
                        .indexType = vk::IndexType::eUint32,
                        .indexData = indexBufferAddress,
                        .transformData = transformationBufferAddress,
                    },
                },
                .flags = vk::GeometryFlagBitsKHR::eOpaque,
            };
            meshGeometries.push_back(geometry);

            meshGeometiesTriangleCounts.push_back(primitive.index_count / 3);

            vk::AccelerationStructureBuildRangeInfoKHR buildRange {
                .primitiveCount = primitive.index_count / 3,
                .primitiveOffset = primitive.index_offset * static_cast<uint32_t>(sizeof(uint32_t)),
                .firstVertex = primitive.vertex_offset,
                .transformOffset = primitiveID * static_cast<uint32_t>(sizeof(vk::TransformMatrixKHR)),
            };
            meshGeomtriesBuildRanges.push_back(buildRange);
        }

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo {
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            .geometryCount = static_cast<uint32_t>(meshGeometries.size()),
            .pGeometries = meshGeometries.data(),
        };

        auto buildSizesInfo = app.vk_device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, meshGeometiesTriangleCounts, app.vk_ext_dispatcher);
        instanceData.acceleration_structure.buffer = buffertools::CreateBufferD(app, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, buildSizesInfo.accelerationStructureSize);

        vk::AccelerationStructureCreateInfoKHR createInfo {
            .buffer = instanceData.acceleration_structure.buffer.handle,
            .size = buildSizesInfo.accelerationStructureSize,
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
        };
        instanceData.acceleration_structure.handle = app.vk_device.createAccelerationStructureKHR(createInfo, nullptr, app.vk_ext_dispatcher);

        Buffer scratchBuffer = buffertools::CreateBufferD(app, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, buildSizesInfo.buildScratchSize);

        buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        buildInfo.dstAccelerationStructure = instanceData.acceleration_structure.handle;
        buildInfo.scratchData.deviceAddress = buffertools::GetBufferDeviceAddress(app, scratchBuffer);



        app.WithSingleTimeCommandBuffer([&](vk::CommandBuffer cmdBuffer) {
            cmdBuffer.buildAccelerationStructuresKHR(buildInfo, meshGeomtriesBuildRanges.data(), app.vk_ext_dispatcher);
        });

        buffertools::DestroyBuffer(app, scratchBuffer);

        resources.instances.push_back(instanceData);
    }
}

void RTX::createTopLevelAS() {
    vk::TransformMatrixKHR transformMatrix {
        .matrix = std::array<std::array<float,4>,3> {
            std::array<float,4> { 1.0f, 0.0f, 0.0f, 0.0f },
            std::array<float,4> { 0.0f, 1.0f, 0.0f, 0.0f },
            std::array<float,4> { 0.0f, 0.0f, 1.0f, 0.0f },
        }
    };

    std::vector<vk::AccelerationStructureInstanceKHR> instances;

    for(const auto& instanceData : resources.instances) {
        instances.push_back(vk::AccelerationStructureInstanceKHR {
            .transform = transformMatrix,
            .instanceCustomIndex = instanceData.geometry_offset,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = buffertools::GetBufferDeviceAddress(app, instanceData.acceleration_structure.buffer),
        });
    }

    Buffer instancesBuffer = buffertools::CreateBufferD(
            app, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, 
            instances.size() * sizeof(vk::AccelerationStructureInstanceKHR), instances.data());

    vk::DeviceOrHostAddressConstKHR instancesBufferAddress { .deviceAddress = buffertools::GetBufferDeviceAddress(app, instancesBuffer)};

    vk::AccelerationStructureGeometryKHR geometry {
        .geometryType = vk::GeometryTypeKHR::eInstances,
        .geometry = vk::AccelerationStructureGeometryDataKHR {
            .instances = vk::AccelerationStructureGeometryInstancesDataKHR {
                .arrayOfPointers = VK_FALSE,
                .data = instancesBufferAddress,
            },
        },
        .flags = vk::GeometryFlagBitsKHR::eOpaque,
    };

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo {
        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .geometryCount = 1,
        .pGeometries = &geometry,
    };


    const uint32_t primitiveCount = static_cast<uint32_t>(instances.size());
    auto sizeInfo = app.vk_device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, primitiveCount, app.vk_ext_dispatcher);

    resources.top.buffer = buffertools::CreateBufferD(app, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR, sizeInfo.accelerationStructureSize);

    vk::AccelerationStructureCreateInfoKHR createInfo {
        .buffer = resources.top.buffer.handle,
        .size = sizeInfo.accelerationStructureSize,
        .type = vk::AccelerationStructureTypeKHR::eTopLevel,
    };
    resources.top.handle = app.vk_device.createAccelerationStructureKHR(createInfo, nullptr, app.vk_ext_dispatcher);

    Buffer scratchBuffer = buffertools::CreateBufferD(app, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, sizeInfo.buildScratchSize);

    buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    buildInfo.dstAccelerationStructure = resources.top.handle;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;
    buildInfo.scratchData.deviceAddress = buffertools::GetBufferDeviceAddress(app, scratchBuffer);

    vk::AccelerationStructureBuildRangeInfoKHR buildRange {
        .primitiveCount = primitiveCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0,
    };

    app.WithSingleTimeCommandBuffer([&](vk::CommandBuffer cmdBuffer) {
        cmdBuffer.buildAccelerationStructuresKHR(buildInfo, &buildRange, app.vk_ext_dispatcher);
    });

    buffertools::DestroyBuffer(app, scratchBuffer);
    buffertools::DestroyBuffer(app, instancesBuffer);
}

void RTX::createMaterialBuffer() {
    std::vector<GLTFMaterial> materials;
    for(const auto& mesh : scene.meshes) {
        for (const auto& primitive : mesh.primitives) {
            materials.push_back(primitive.material);
        }
    }
    resources.material_buffer = buffertools::CreateBufferD(app, vk::BufferUsageFlagBits::eStorageBuffer, materials.size() * sizeof(GLTFMaterial), materials.data());
}

void RTX::createPipeline() {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
    });

    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
    });

    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 2,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
    });

    uint32_t totalBuffers = 0;
    for(const auto& mesh : scene.meshes) {
        totalBuffers += mesh.primitives.size();
    }
    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 3,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = static_cast<uint32_t>(totalBuffers),
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
    });

    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 4,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = static_cast<uint32_t>(totalBuffers),
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
    });

    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 5,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
    });

    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 6,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = static_cast<uint32_t>(scene.textures.size()),
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
    });

    bindings.push_back(vk::DescriptorSetLayoutBinding {
        .binding = 7,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
    });

    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    this->descr_layout = app.vk_device.createDescriptorSetLayout(layoutInfo);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &this->descr_layout,
    };

    this->pipeline_layout = app.vk_device.createPipelineLayout(pipelineLayoutInfo);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    // 1. Raygen
    shaderStages.push_back(
        vk::inits::shaderStageCreateInfo(app.LoadShader("./shaders_bin/raygen.rgen.spv"), vk::ShaderStageFlagBits::eRaygenKHR)
    );

    shader_groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR {
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = static_cast<uint32_t>(shaderStages.size()) - 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
    });

    // 2. Miss
    shaderStages.push_back(
        vk::inits::shaderStageCreateInfo(app.LoadShader("./shaders_bin/miss.rmiss.spv"), vk::ShaderStageFlagBits::eMissKHR)
    );

    shader_groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR {
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = static_cast<uint32_t>(shaderStages.size()) - 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
    });

    // 3. Closest Hit
    shaderStages.push_back(
        vk::inits::shaderStageCreateInfo(app.LoadShader("./shaders_bin/hit.rchit.spv"), vk::ShaderStageFlagBits::eClosestHitKHR)
    );

    shader_groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR {
            .type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
    });

    vk::RayTracingPipelineCreateInfoKHR pipelineInfo {
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .groupCount = static_cast<uint32_t>(shader_groups.size()),
        .pGroups = shader_groups.data(),
        .maxPipelineRayRecursionDepth = 1,
        .layout = this->pipeline_layout,
    };

    auto result = app.vk_device.createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo, nullptr, app.vk_ext_dispatcher);
    vk::resultCheck(result.result, "Error creating RTX pipeline");
    this->pipeline = result.value;

    for(auto& stage : shaderStages) {
        app.vk_device.destroyShaderModule(stage.module);
    }
}

void RTX::createShaderBindingTable() {
    const uint32_t handleSize = pipeline_properties.shaderGroupHandleSize;
    const uint32_t handleSizeAlligned = vk::tools::allignedSize(pipeline_properties.shaderGroupHandleSize, pipeline_properties.shaderGroupHandleAlignment);
    const uint32_t groupCount = static_cast<uint32_t>(shader_groups.size());
    const uint32_t sbtSize = groupCount * handleSizeAlligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    auto result = app.vk_device.getRayTracingShaderGroupHandlesKHR(pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data(), app.vk_ext_dispatcher);
    vk::resultCheck(result, "Error retrieving group handles");

    auto usage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
    binding_table.raygen = buffertools::CreateBufferD(app, usage, handleSize, shaderHandleStorage.data() + 0 * handleSizeAlligned);
    binding_table.miss   = buffertools::CreateBufferD(app, usage, handleSize, shaderHandleStorage.data() + 1 * handleSizeAlligned);
    binding_table.hit    = buffertools::CreateBufferD(app, usage, handleSize, shaderHandleStorage.data() + 2 * handleSizeAlligned);

    binding_table.raygen_address = buffertools::GetBufferDeviceAddress(app, binding_table.raygen);
    binding_table.miss_address = buffertools::GetBufferDeviceAddress(app, binding_table.miss);
    binding_table.hit_address = buffertools::GetBufferDeviceAddress(app, binding_table.hit);
}


void RTX::createDescriptorSet() {
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool = app.vk_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &this->descr_layout,
    };

    this->descriptor_set = app.vk_device.allocateDescriptorSets(allocInfo)[0];

    auto storageImageWriteInfo = vk::DescriptorImageInfo {
        .imageView = storage_image.view,
        .imageLayout = vk::ImageLayout::eGeneral,
    };
    auto storageImageWrite = vk::inits::writeDescriptorSetImage(descriptor_set, vk::DescriptorType::eStorageImage, 0, &storageImageWriteInfo);

    auto accelerationStructureInfo = vk::WriteDescriptorSetAccelerationStructureKHR {
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &resources.top.handle,
    };
    vk::WriteDescriptorSet accelerationStructureWrite {
        .pNext = &accelerationStructureInfo,
        .dstSet = descriptor_set,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
    };

    this->resources.uniform_buffer = buffertools::CreateBufferH2D(app, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformData));
    this->resources.uniform_buffer_data = reinterpret_cast<UniformData*>(buffertools::MapBuffer(app, resources.uniform_buffer));
    auto uniformBufferInfo = vk::DescriptorBufferInfo {
        .buffer = resources.uniform_buffer.handle,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    auto uniformBufferWrite = vk::inits::writeDescriptorSetBuffer(descriptor_set, vk::DescriptorType::eUniformBuffer, 2, &uniformBufferInfo);

    std::vector<vk::DescriptorBufferInfo> vertexBufferInfos;
    for(const auto& instance : resources.instances) {
        for(const auto& primitive : instance.primitives) {
            vertexBufferInfos.push_back(vk::DescriptorBufferInfo {
                .buffer = instance.vertex_buffer.handle,
                .offset = primitive.vertex_offset * sizeof(GLTFVertex),
                .range = primitive.vertex_count * sizeof(GLTFVertex),
            });
        }
    }
    auto vertexBufferWrite = vk::inits::writeDescriptorSetBuffer(descriptor_set, vk::DescriptorType::eStorageBuffer, 3, vertexBufferInfos.data(), static_cast<uint32_t>(vertexBufferInfos.size()));

    std::vector<vk::DescriptorBufferInfo> indexBufferInfos;
    for(const auto& instance : resources.instances) {
        for(const auto& primitive : instance.primitives) {
            indexBufferInfos.push_back(vk::DescriptorBufferInfo {
                .buffer = instance.index_buffer.handle,
                .offset = primitive.index_offset * sizeof(uint32_t),
                .range = primitive.index_count * sizeof(uint32_t),
            });
        }
    }
    auto indexBufferWrite = vk::inits::writeDescriptorSetBuffer(descriptor_set, vk::DescriptorType::eStorageBuffer, 4, indexBufferInfos.data(), static_cast<uint32_t>(indexBufferInfos.size()));

    vk::DescriptorBufferInfo materialBufferInfo {
        .buffer = resources.material_buffer.handle,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    auto materialBufferWrite = vk::inits::writeDescriptorSetBuffer(descriptor_set, vk::DescriptorType::eStorageBuffer, 5, &materialBufferInfo);

    std::vector<vk::DescriptorImageInfo> textureArrayInfos;
    for(const auto& texture : resources.textures) {
        textureArrayInfos.push_back(vk::DescriptorImageInfo {
            .sampler = app.vk_default_sampler,
            .imageView = texture.view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        });
    }
    auto textureArrayWrite = vk::inits::writeDescriptorSetImage(descriptor_set, vk::DescriptorType::eCombinedImageSampler, 6, textureArrayInfos.data(), static_cast<uint32_t>(textureArrayInfos.size()));

    vk::DescriptorImageInfo skyboxImageInfo {
        .sampler = app.vk_default_sampler,
        .imageView = resources.skybox.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    auto skyboxWrite = vk::inits::writeDescriptorSetImage(descriptor_set, vk::DescriptorType::eCombinedImageSampler, 7, &skyboxImageInfo);

    std::vector<vk::WriteDescriptorSet> writes {
        storageImageWrite,
        accelerationStructureWrite,
        uniformBufferWrite,
        vertexBufferWrite,
        indexBufferWrite,
        materialBufferWrite,
        textureArrayWrite,
        skyboxWrite,
    };


    app.vk_device.updateDescriptorSets(writes, {});
}
