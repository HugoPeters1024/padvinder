#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "common.glsl"

layout(binding = 3) readonly buffer Vertices { GLTFVertex data[]; } vertBuffers[];
layout(binding = 4) readonly buffer Indices { uint data[]; } indexBuffers[];
layout(binding = 5) readonly buffer Materials { Material materials[]; };
layout(binding = 6) uniform sampler2D textures[];

hitAttributeEXT vec2 baryCoord;

layout(location = 0) rayPayloadInEXT Payload {
    mat3 tangentToWorld;
    bool hit;
    bool inside;
    float t;
    vec3 surface_normal;
    vec3 normal;
    Material material;
} payload;

vec3 baryWeights = vec3(1 - baryCoord.x - baryCoord.y, baryCoord.x, baryCoord.y);
uint geometryID = gl_GeometryIndexEXT + gl_InstanceCustomIndexEXT;


Triangle getTriangle() {
    const uint primitiveID = gl_PrimitiveID;
    uint i0 = indexBuffers[geometryID].data[primitiveID*3+0];
    uint i1 = indexBuffers[geometryID].data[primitiveID*3+1];
    uint i2 = indexBuffers[geometryID].data[primitiveID*3+2];
    GLTFVertex v0 = vertBuffers[geometryID].data[i0];
    GLTFVertex v1 = vertBuffers[geometryID].data[i1];
    GLTFVertex v2 = vertBuffers[geometryID].data[i2];
    return Triangle(v0, v1, v2);
}



vec3 triangleNormal(in Triangle t) {
    return baryWeights.x * t.v0.normal.xyz 
         + baryWeights.y * t.v1.normal.xyz 
         + baryWeights.z * t.v2.normal.xyz;
}

vec2 triangleUV(in Triangle t) {
    return baryWeights.x * vec2(t.v0.pos.w, t.v0.normal.w)
         + baryWeights.y * vec2(t.v1.pos.w, t.v1.normal.w)
         + baryWeights.z * vec2(t.v2.pos.w, t.v2.normal.w);
}

void shade() {
}


void main() {
    payload.hit = true;
    payload.t = gl_RayTmaxEXT;

    Triangle triangle = getTriangle();
    payload.surface_normal = triangleNormal(triangle);
    if (dot(payload.surface_normal, gl_WorldRayDirectionEXT) > 0) {
        payload.surface_normal *= -1;
        payload.inside = true;
    } else {
        payload.inside = false;
    }
    payload.normal = payload.surface_normal;

    payload.material = materials[geometryID];

    vec2 texUV = triangleUV(triangle);

    uint textureID = payload.material.textureID;
    if (textureID != -1) {
        payload.material.diffuse_color = texture(textures[textureID], texUV) * payload.material.diffuse_color;
    }


    uint normalTextureID = payload.material.normalTextureID;
    if (normalTextureID != -1) {
        vec3 edge1 = triangle.v1.pos.xyz - triangle.v0.pos.xyz;
        vec3 edge2 = triangle.v2.pos.xyz - triangle.v0.pos.xyz;

        const vec2 uv0 = vec2(triangle.v0.pos.w, triangle.v0.normal.w);
        const vec2 uv1 = vec2(triangle.v1.pos.w, triangle.v1.normal.w);
        const vec2 uv2 = vec2(triangle.v2.pos.w, triangle.v2.normal.w);
        vec2 deltaUV1 = uv1 - uv0;
        vec2 deltaUV2 = uv2 - uv0;

        float div = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        if (abs(div) > 0.001f) {
            const float f = 1.0f / div;
            vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent = normalize(tangent);

            vec3 bitangent = normalize(cross(payload.normal, tangent));
            mat3 TBN = mat3(tangent, bitangent, payload.normal);

            vec3 texNormal = texture(textures[normalTextureID], texUV).xyz * 2.0f - 1.0f;
            payload.normal = normalize(TBN * texNormal);
        }
    }

    payload.tangentToWorld = AlignToNormalM(payload.normal);
}


