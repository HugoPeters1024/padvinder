#pragma once
#include <precomp.h>
#include <AppBase.h>
#include <Vertex.h>

typedef uint32_t MaterialID;
typedef uint32_t TextureID;

struct GLTFTexture {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> data;
};

struct alignas(16) GLTFMaterial {
    glm::vec4 diffuse_color = glm::vec4(1,1,1,1);
    glm::vec4 emission = glm::vec4(0);
    glm::vec4 glass = glm::vec4(0,0,0,1);
    float roughness = 1.0f;
    TextureID texture_id;
    TextureID normal_texture_id;
};

struct GLTFPrimitive {
    uint32_t index_count;
    uint32_t index_offset;
    uint32_t vertex_count;
    uint32_t vertex_offset;
    GLTFMaterial material;
    glm::mat4 transform;
};

struct GLTFMesh {
    std::vector<GLTFPrimitive> primitives;
    std::vector<GLTFVertex> vertices;
    std::vector<uint32_t> indices;
};

class Scene {
public:
    Scene(AppBase& app) : app(app) {}

    void LoadModel(const char* filename, bool binary = false);

    std::vector<GLTFMesh> meshes;
    std::vector<GLTFTexture> textures;

private:
    AppBase& app;
};
