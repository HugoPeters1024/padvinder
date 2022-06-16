#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <VkDefaults.h>
#include <vk_mem_alloc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SSE2
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/quaternion.hpp>

#include <stb_image.h>
#include <tiny_gltf.h>



#include <cstdio>
#include <vector>
#include <optional>
#include <limits>
#include <algorithm>
#include <fstream>

#include <string>

#include <spdlog/spdlog.h>
namespace logger = spdlog;


class NoCopy {
public:
    NoCopy(NoCopy const&) = delete;
    NoCopy& operator=(NoCopy const&) = delete;
    NoCopy() = default;
};
