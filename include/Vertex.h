#pragma once

#include <precomp.h>

template<typename T>
struct vertex_info {
    static vk::VertexInputBindingDescription BindingDescription(uint32_t binding);
    static std::vector<vk::VertexInputAttributeDescription> AttributeDescriptions(uint32_t binding);
};

struct Vertex2D {
    glm::vec2 pos;
};

struct Vertex3D {
    glm::vec3 pos;
};

struct GLTFVertex {
    glm::vec4 pos;
    glm::vec4 normal;
};

template<>
struct vertex_info<Vertex2D> {
    static vk::VertexInputBindingDescription BindingDescription(uint32_t binding) {
        return vk::VertexInputBindingDescription {
            .binding = binding,
            .stride = sizeof(Vertex2D),
            .inputRate = vk::VertexInputRate::eVertex,
        };
    }
    static std::vector<vk::VertexInputAttributeDescription> AttributeDescriptions(uint32_t binding) {
        return {
            vk::VertexInputAttributeDescription {
                .location = 0,
                .binding = binding,
                .format = vk::Format::eR32G32Sfloat,
                .offset = offsetof(Vertex2D, pos),
            },
        };
    }
};


template<>
struct vertex_info<Vertex3D> {
    static vk::VertexInputBindingDescription BindingDescription(uint32_t binding) {
        return vk::VertexInputBindingDescription {
            .binding = binding,
            .stride = sizeof(Vertex3D),
            .inputRate = vk::VertexInputRate::eVertex,
        };
    }
    static std::vector<vk::VertexInputAttributeDescription> AttributeDescriptions(uint32_t binding) {
        return {
            vk::VertexInputAttributeDescription {
                .location = 0,
                .binding = binding,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex3D, pos),
            },
        };
    }
};
