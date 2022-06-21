const float PI = 3.141592653589793f;
const float EPS = 0.005f;

float max3(in vec3 v) { return max(v.x, max(v.y, v.z)); }


uint rand_xorshift(in uint seed)
{
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed;
}

uint wang_hash(in uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

uint g_seed = 22;

float randf()
{
    g_seed = rand_xorshift(g_seed);
    return g_seed * 2.3283064365387e-10f;
}

vec3 AlignToNormal(in vec3 normal, in vec3 i) {
    const vec3 w = normal;
    const vec3 u = normalize(cross((abs(w.x) > .1f ? vec3(0, 1, 0) : vec3(1, 0, 0)), w));
    const vec3 v = normalize(cross(w,u));

    return mat3(u, v, w) * i;
}

vec3 SampleHemisphereCosine(in vec3 normal)
{
    float r0 = randf();
    float r1 = randf();
    const float r = sqrt(r0);
    const float theta = 2.0f * PI * r1;
    const float x = r * cos(theta);
    const float y = r * sin(theta);
    const vec3 ret = vec3(x, y, sqrt(1.0f - r0));
    return AlignToNormal(normal, ret);
}

vec3 SampleSphere() {
    float r0 = 1.0f;
    float r1 = 1.0f;
    float r2 = 1.0f;
    do {
        r0 = randf() * 2 - 1;
        r1 = randf() * 2 - 1;
        r2 = randf() * 2 - 1;
    } while (r0 * r0 + r1 * r1 + r2 * r2 > 1);

    return normalize(vec3(r0, r1, r2));
}

bool InsideHexacon(vec2 p) {
    const float hr = 1.0f;
    const float q2x = abs(p.x);
    const float q2y = abs(p.y);
    if (q2x > hr || q2y > hr * 2) return false;
    return 2 * hr * hr - hr * q2x - hr * q2y >= 0;
}

vec2 SampleHexacon() {
    vec2 p;
    do {
        p = vec2(randf(),randf())* 2.0f - 1.0f;
    } while (!InsideHexacon(p));
    return p;
}

float saturate (float x)
{
    return min(1.0, max(0.0,x));
}
vec3 saturate (vec3 x)
{
    return min(vec3(1.,1.,1.), max(vec3(0.,0.,0.),x));
}

vec3 bump3y (vec3 x, vec3 yoffset)
{
	vec3 y = vec3(1.,1.,1.) - x * x;
	y = saturate(y-yoffset);
	return y;
}

// Based on GPU Gems
// Optimised by Alan Zucconi
vec3 WavelengthToRGB(float w) {
{
	// w: [400, 700]
	// x: [0,   1]
    float x = saturate((w - 400.0)/ 300.0);

	const vec3 cs = vec3(3.54541723, 2.86670055, 2.29421995);
	const vec3 xs = vec3(0.69548916, 0.49416934, 0.28269708);
	const vec3 ys = vec3(0.02320775, 0.15936245, 0.53520021);

	return bump3y (	cs * (x - xs), ys);
}
}




struct Material {
    vec4 diffuse_color;
    vec4 emission;
    // absorption + translucency
    vec4 glass;
    float roughness;
    uint textureID;
    uint normalTextureID;
};

struct GLTFVertex {
    vec4 pos;
    vec4 normal;
};

struct Triangle {
    GLTFVertex v0;
    GLTFVertex v1;
    GLTFVertex v2;
};
