#include <precomp.h>
#include <WindowApp.h>
#include <GraphicsPipelineConfig.h>
#include <GraphicsPipeline.h>
#include <Vertex.h>
#include <BufferTools.h>
#include <RTX.h>
#include <RenderPass.h>
#include <Camera.h>
#include <Scene.h>

constexpr uint32_t WINDOW_WIDTH = 1920;
constexpr uint32_t WINDOW_HEIGHT = 1080;

struct UniformBuffer {
    glm::mat4 camera;
    glm::mat4 proj;
    float time;
};

constexpr WindowAppConfig windowConfig {
    .width = WINDOW_WIDTH,
    .height = WINDOW_HEIGHT,
    .name = "Vulkan App",
    .framesInFlight = 1,
};

uint32_t wang_hash(uint32_t seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}


static uint32_t g_seed = 665;

uint32_t randi32() {
    g_seed = wang_hash(g_seed);
    return g_seed;
}

float randf() {
    return randi32() * 2.3283064365387e-10f;
}


int main(int argc, char** argv) {
    logger::set_level(logger::level::debug);

    WindowApp app;
    app.Require<RTX>();
    app.Init(windowConfig);

    Camera camera(app.glfw_window);
    camera.eye.y = 5;
    camera.eye.x = -5;

    Scene scene(app);
//    {
//        scene.LoadModel("models/rungholt.glb", true);
//    }
//    {
//        scene.LoadModel("models/san_miguel.glb", true);
//        scene.meshes.back().primitives[124].material.diffuse_color.w = 0.0f;
//        scene.meshes.back().primitives[124].material.glass.w = 1.52f;
//        scene.meshes.back().primitives[124].material.roughness = 0.0f;
//        scene.meshes.back().primitives[5].material.roughness = 0.00;
//        scene.meshes.back().primitives[5].material.diffuse_color.w = 0.0f;
//        scene.meshes.back().primitives[5].material.glass.w = 1.33;
//        scene.meshes.back().primitives[13].material.emission = glm::vec4(2,1,1,0);
//        // plate
//        scene.meshes.back().primitives[122].material.metallic = 0.8f;
//        scene.meshes.back().primitives[122].material.roughness = 0.3f;
//        scene.meshes.back().primitives[151].material.roughness = 0.0f;
//        
//        // chair
//        scene.meshes.back().primitives[133].material.roughness = 0.2f;
//    }
      {
//          scene.LoadModel("models/sibenik.glb", true);
      }
      {
          scene.LoadModel("models/bistro.glb", true);
          scene.meshes.back().primitives[33].material.roughness = 0.4;
//          scene.meshes.back().primitives[38].material.emission = 0.0f * glm::vec4(4, 2, 3,0);
//          scene.meshes.back().primitives[39].material.emission = 0.0f * glm::vec4(2, 1, 1.5,0);
          scene.meshes.back().primitives[8].material.diffuse_color.w = 0.0f;
          scene.meshes.back().primitives[8].material.glass.w = 1.46f;
      }

//    scene.LoadModel("models/fireplace_room.glb", true);
//    scene.meshes.back().primitives[1].material.diffuse_color.w = 0.0f;
//    scene.meshes.back().primitives[1].material.glass.w = 1.46f;

    

//    scene.LoadModel("models/holebox.glb", true);
//    scene.meshes[0].primitives[0].material.diffuse_color = glm::vec4(0.30,0.3, 0.35f, 1.0f);
//    scene.meshes[0].primitives[0].material.glass = glm::vec4(0,0,0,1.0f);
//    scene.meshes[0].primitives[0].material.roughness = 1.00;

    // floor
    //scene.meshes[0].primitives[46].material.roughness = 0.4f;

    //// basin
    //scene.meshes[0].primitives[95].material.emission = glm::vec4(0.2f, 0.3f, 0.4f, 0) * 0.0f;

    //// flowerpots
    //for(uint32_t i=82; i<=92; i++) {
    //    scene.meshes[0].primitives[i].material.roughness = 0.2f;
    //}

//    scene.LoadModel("./models/prism.glb", true);
//    scene.meshes[1].primitives[0].material.diffuse_color = glm::vec4(1.0,1.0, 1.0f, 0.0f);
//    scene.meshes[1].primitives[0].material.glass = glm::vec4(0.0,0.0,0.0,1.5f);
//    scene.meshes[1].primitives[0].material.roughness = 0.0;
//

    RTXConfig rtxConfig {
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
    };

    RTX rtx(app, scene, rtxConfig);

    auto rtxSampler = rtx.CreateStorageImageSampler();

    auto renderPassConfig = RenderPassInfo()
        .addColorAttachment(app.getSurfaceFormat());
    RenderPass renderPass(app, renderPassConfig);

    std::vector<vk::Framebuffer> framebuffers;
    for(const auto& view : app.vk_swapchain_views) {
        framebuffers.push_back(renderPass.createFrameBuffer({view}, WINDOW_WIDTH, WINDOW_HEIGHT));
    }

    auto vertShader = app.LoadShader("./shaders_bin/triangle.vert.spv");
    auto fragShader = app.LoadShader("./shaders_bin/triangle.frag.spv");

    auto graphicsPipelineConfig = GraphicsPipelineConfig(renderPass.vk_render_pass, vertShader, fragShader)
        .useTexture(0);
    GraphicsPipeline pipeline(app, graphicsPipelineConfig);

    auto descriptor = pipeline.AllocateDescriptor(app);
    
    {
        vk::DescriptorImageInfo texDescr {
            .sampler = rtxSampler,
            .imageView = rtx.storage_image.view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };

        vk::WriteDescriptorSet write {
            .dstSet = descriptor,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &texDescr,
        };

        app.vk_device.updateDescriptorSets({write}, {});
    }

    vk::CommandBuffer cmdBuffer = app.MakeGraphicsCommandBuffer();

    uint32_t tick = 0;
    double ping = glfwGetTime();
    float lastFrameTime = static_cast<float>(glfwGetTime());
    while(!app.WindowShouldClose()) {
        double pong = glfwGetTime();
        if (tick % 500 == 0) {
            logger::info("FPS: {}", 1.0 / (pong - ping));
        }
        ping = pong;
        glfwPollEvents();
        tick++;
        if (camera.getHasMoved()) {
            tick = 0;
        }
        float currentTime = static_cast<float>(glfwGetTime());
        float dt = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        camera.update(dt);
        auto frame = app.WindowFrameStart();

        cmdBuffer.reset();
        cmdBuffer.begin(vk::CommandBufferBeginInfo());

        vk::tools::insertImageMemoryBarrier(cmdBuffer, rtx.storage_image.handle, 
                vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eMemoryWrite,
                vk::ImageLayout::eReadOnlyOptimal, vk::ImageLayout::eGeneral,
                vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                vk::inits::imageSubresourceRange(vk::ImageAspectFlagBits::eColor));
        rtx.Record(cmdBuffer, tick, camera);
        vk::tools::insertImageMemoryBarrier(cmdBuffer, rtx.storage_image.handle,
                vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::PipelineStageFlagBits::eFragmentShader,
                vk::inits::imageSubresourceRange(vk::ImageAspectFlagBits::eColor));

        vk::ClearValue clearColor { .color = { .float32 = std::array<float, 4>{0.0f,0.0f,0.0f,0.0f} } };
        vk::RenderPassBeginInfo renderPassBeginInfo {
            .renderPass = renderPass.vk_render_pass,
            .framebuffer = framebuffers[frame.imageIdx],
            .renderArea = {
                {0, 0},
                {WINDOW_WIDTH, WINDOW_HEIGHT},
            },
            .clearValueCount = 1,
            .pClearValues = &clearColor
        };


        auto viewport = vk::inits::viewport(WINDOW_WIDTH, WINDOW_HEIGHT);
        auto scissor = vk::Rect2D { {0,0}, { WINDOW_WIDTH, WINDOW_HEIGHT}};

        cmdBuffer.setViewport(0, {viewport});
        cmdBuffer.setScissor(0, {scissor});

        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, {descriptor}, {});

        cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        cmdBuffer.draw(3, 1, 0, 0);
        cmdBuffer.endRenderPass();

        cmdBuffer.end();
        app.WindowFrameEnd(cmdBuffer);
    }

    app.vk_device.waitIdle();

    for(const auto& framebuffer : framebuffers) {
        app.vk_device.destroyFramebuffer(framebuffer);
    }

    app.vk_device.destroySampler(rtxSampler);
    app.vk_device.destroyShaderModule(vertShader);
    app.vk_device.destroyShaderModule(fragShader);

    rtx.Destroy();
    pipeline.Destroy(app);
    renderPass.Destroy();
    return 0;
}
