#include <Scene.h>

template<typename DST_T, typename SRC_T>
inline void vector_insert_cast(std::vector<DST_T>& dst, SRC_T* src, size_t nrElements) {
    for(size_t i=0; i<nrElements; i++) {
        dst.push_back(static_cast<DST_T>(src[i]));
    }
}

void Scene::LoadModel(const char* filename, bool binary) {
    // TODO: this dumb
    this->textures.push_back(GLTFTexture {
        .width = 32,
        .height = 32,
        .data = std::vector<uint8_t>(32*32*4),
    });

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;


   // bool res = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    bool res = binary
        ? loader.LoadBinaryFromFile(&model, &err, &warn, filename)
        : loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) {
        logger::warn("{}", warn);
    }

    if (!err.empty()) {
        logger::error("{}", err);
    }

    if (!res) {
        logger::error("Failed to load glTF: {}", filename);
        exit(1);
    }

    assert(model.scenes.size() == 1 && "Currently only support single scenes");
    auto& scene = model.scenes[0];
    

    assert(scene.nodes.size() >= 1 && "Currently only support single nodes");
    auto& node = model.nodes[scene.nodes[0]];

    glm::vec3 scale = glm::vec3(1);
    if (node.scale.size() == 3) {
        scale.x = static_cast<float>(node.scale[0]);
        scale.y = static_cast<float>(node.scale[1]);
        scale.z = static_cast<float>(node.scale[2]);
    }

    glm::vec3 translation = glm::vec3(0);
    if (node.translation.size() == 3) {
        translation.x = static_cast<float>(node.translation[0]);
        translation.y = static_cast<float>(node.translation[1]);
        translation.z = static_cast<float>(node.translation[2]);
    }

    glm::quat rotation = glm::identity<glm::quat>();
    if (node.rotation.size() == 4) {
        rotation.x = static_cast<float>(node.rotation[0]);
        rotation.y = static_cast<float>(node.rotation[1]);
        rotation.z = static_cast<float>(node.rotation[2]);
        rotation.w = static_cast<float>(node.rotation[3]);
    }

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, translation);
    transform = transform * glm::toMat4(rotation);
    transform = glm::scale(transform, scale);


    // hack to eliminate non mesh nodes
    while(node.mesh == -1) {
        assert(node.children.size() == 1 && "Expected a linear node chain for a quick hack");
        node = model.nodes[node.children[0]];
    }
    assert(node.mesh != -1 && "Invalid mesh");

    auto& mesh = model.meshes[node.mesh];

    uint32_t runningVertexCount = 0;
    uint32_t runningIndexCount = 0;
    GLTFMesh resmesh{};
    for(auto& primitive : mesh.primitives) {
        GLTFPrimitive res{};
        res.transform = transform;

        assert(primitive.attributes.find("POSITION") != primitive.attributes.end());
        assert(primitive.attributes.find("NORMAL") != primitive.attributes.end());
        //assert(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end());

        uint32_t posBufAccIdx = primitive.attributes["POSITION"];
        auto& posAcc = model.accessors[posBufAccIdx];
        auto& posView = model.bufferViews[posAcc.bufferView];
        auto& posBuf = model.buffers[posView.buffer];
        assert(posAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        assert(posAcc.type == TINYGLTF_TYPE_VEC3);
        glm::vec3* posHead = reinterpret_cast<glm::vec3*>(posBuf.data.data() + posAcc.byteOffset + posView.byteOffset);

        uint32_t normalBufAccIdx = primitive.attributes["NORMAL"];
        auto& normalAcc = model.accessors[normalBufAccIdx];
        auto& normalView = model.bufferViews[normalAcc.bufferView];
        auto& normalBuf = model.buffers[normalView.buffer];
        assert(normalAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        assert(normalAcc.type == TINYGLTF_TYPE_VEC3);
        glm::vec3* normalHead = reinterpret_cast<glm::vec3*>(normalBuf.data.data() + normalAcc.byteOffset + normalView.byteOffset);

        glm::vec2* texHead = nullptr;
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
            uint32_t texBufAccIdx = primitive.attributes["TEXCOORD_0"];
            auto& texAcc = model.accessors[texBufAccIdx];
            auto& texView = model.bufferViews[texAcc.bufferView];
            auto& texBuf = model.buffers[texView.buffer];
            assert(texAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            assert(texAcc.type == TINYGLTF_TYPE_VEC2);
            texHead = reinterpret_cast<glm::vec2*>(texBuf.data.data() + texAcc.byteOffset + texView.byteOffset);
            assert(posAcc.count == texAcc.count);
        }

        assert(posAcc.count == normalAcc.count);

        const size_t nrVertices = posAcc.count;

        for(size_t i=0; i<nrVertices; i++) {
            glm::vec2 uv = texHead == nullptr ? glm::vec2(0) : *(texHead+i);
            glm::vec3& normal = *(normalHead+i);
            normal = glm::normalize((transform * glm::vec4(normal,0)).xyz());
            resmesh.vertices.push_back(GLTFVertex {
                .pos = glm::vec4(*(posHead+i), uv.x),
                .normal = glm::vec4(normal, uv.y),
            });
        }


        assert(primitive.indices != -1);
        uint32_t indexBufAccIdx = primitive.indices;
        auto& indexAcc = model.accessors[indexBufAccIdx];
        auto& indexView = model.bufferViews[indexAcc.bufferView];
        auto& indexBuf = model.buffers[indexView.buffer];
        assert(indexAcc.type == TINYGLTF_TYPE_SCALAR);
        uint16_t* indexHead = reinterpret_cast<uint16_t*>(indexBuf.data.data() + indexAcc.byteOffset + indexView.byteOffset);
        const size_t nrIndices = indexAcc.count;

        switch(indexAcc.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: vector_insert_cast<uint32_t, uint16_t>(resmesh.indices, reinterpret_cast<uint16_t*>(indexHead), nrIndices); break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: vector_insert_cast<uint32_t, uint32_t>(resmesh.indices, reinterpret_cast<uint32_t*>(indexHead), nrIndices); break;
            default: throw std::runtime_error("Unsupported index type"); break;
        }

        res.material.emission = glm::vec4(0);
        res.material.texture_id = -1;
        res.material.normal_texture_id = -1;
        if (primitive.material != -1) {
            const auto& m = model.materials[primitive.material];
            assert(m.pbrMetallicRoughness.baseColorFactor.size() == 4 && "Four channel base color expected");
            res.material.diffuse_color.x = m.pbrMetallicRoughness.baseColorFactor[0];
            res.material.diffuse_color.y = m.pbrMetallicRoughness.baseColorFactor[1];
            res.material.diffuse_color.z = m.pbrMetallicRoughness.baseColorFactor[2];
            res.material.diffuse_color.w = m.pbrMetallicRoughness.baseColorFactor[3];
            res.material.roughness = m.pbrMetallicRoughness.roughnessFactor;

            const TextureID textureID = m.pbrMetallicRoughness.baseColorTexture.index;
            if (textureID != -1) {
                const auto& t = model.textures[textureID];
                const auto& t_image = model.images[t.source];

                GLTFTexture texture {
                    .width = static_cast<uint32_t>(t_image.width),
                    .height = static_cast<uint32_t>(t_image.height),
                    .data = t_image.image,
                };

                this->textures.push_back(texture);
                res.material.texture_id = static_cast<uint32_t>(textures.size()) - 1;
            }

            const TextureID normalTexturID = m.normalTexture.index;
            if (normalTexturID != -1) {
                const auto& t = model.textures[normalTexturID];
                const auto& t_image = model.images[t.source];

                GLTFTexture normalTexture {
                    .width = static_cast<uint32_t>(t_image.width),
                    .height = static_cast<uint32_t>(t_image.height),
                    .data = t_image.image,
                };

                this->textures.push_back(normalTexture);
                res.material.normal_texture_id = static_cast<uint32_t>(textures.size()) - 1;
            }
        } 

        res.index_count = nrIndices;
        res.index_offset = runningIndexCount;
        res.vertex_count = nrVertices;
        res.vertex_offset = runningVertexCount;

        runningIndexCount += nrIndices;
        runningVertexCount += nrVertices;

        while(runningIndexCount % 4 != 0) {
            resmesh.indices.push_back(0);
            runningIndexCount++;
        }

        logger::info("Loaded a primitive of {}, #vertices: {}  ---  #indices: {}", filename, nrVertices, nrIndices);
        resmesh.primitives.push_back(res);
    }
    meshes.push_back(resmesh);
}
