#version 460


layout(binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
    const float gamma = 2.2f;
    const float exposure = 35.0f;

    vec4 bufferVal = texture(tex, uv);

    vec3 hdrColor = bufferVal.xyz / bufferVal.w;
    vec3 mapped = pow(vec3(1.0f) - exp(-hdrColor * exposure), vec3(1.0f / gamma));

    outColor = vec4(mapped, 1.0f);
}
