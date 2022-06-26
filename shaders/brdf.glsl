// Gladly stolen from: https://gist.github.com/soma-arc/5d53816885e64628869ed54bfb95e31d
#include "common.glsl"

const vec3 dielectricSpecular = vec3(0.04);
const vec3 BLACK = vec3(0);
// This G term is used in glTF-WebGL-PBR
// Microfacet Models for Refraction through Rough Surfaces
float G1_GGX(float alphaSq, float NoX) {
    float tanSq = (1.0 - NoX * NoX) / max((NoX * NoX), 0.00001);
    return 2. / (1. + sqrt(1. + alphaSq * tanSq));
}

// 1 / (1 + delta(l)) * 1 / (1 + delta(v))
float Smith_G(float alphaSq, float NoL, float NoV) {
    return G1_GGX(alphaSq, NoL) * G1_GGX(alphaSq, NoV);
}

// Height-Correlated Masking and Shadowing
// Smith Joint Masking-Shadowing Function
float GGX_Delta(float alphaSq, float NoX) {
    return (-1. + sqrt(1. + alphaSq * (1. / (NoX * NoX) - 1.))) / 2.;
}

float SmithJoint_G(float alphaSq, float NoL, float NoV) {
    return 1. / (1. + GGX_Delta(alphaSq, NoL) + GGX_Delta(alphaSq, NoV));
}

float GGX_D(float alphaSq, float NoH) {
    float c = (NoH * NoH * (alphaSq - 1.) + 1.);
    return alphaSq / (c * c)  * INVPI;
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 baseColor, float metallic, float roughness) {
    vec3 H = normalize(L+V);

    float LoH = dot(L, H);
    float NoH = dot(N, H);
    float VoH = dot(V, H);
    float NoL = dot(N, L);
    float NoV = dot(N, V);

    vec3 F0 = mix(dielectricSpecular, baseColor, metallic);
    vec3 cDiff = mix(baseColor * (1. - dielectricSpecular.r),
                     BLACK,
                     metallic);
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    // Schlick's approximation
    vec3 F = F0 + (vec3(1.) - F0) * pow((1. - VoH), 5.);

    vec3 diffuse = (vec3(1.) - F) * cDiff * INVPI;

    float G = SmithJoint_G(alphaSq, NoL, NoV);
    //    float G = Smith_G(alphaSq, NoL, NoV);    

    float D = GGX_D(alphaSq, NoH);

    vec3 specular = (F * G * D) / (4. * NoL * NoV);
    return diffuse + specular;
}




float BsdfNDot(vec3 v) { return v.z; }


//====================================================================
vec3 SchlickFresnel(vec3 r0, float rads)
{
    // -- The common Schlick Fresnel approximation
    float exponential = pow(1.0f - rads, 5.0f);
    return r0 + (1.0f - r0) * exponential;
}

//====================================================================
// non height-correlated masking-shadowing function is described here:
float SmithGGXMaskingShadowing(vec3 wi, vec3 wo, float a2)
{
    float dotNL = BsdfNDot(wi);
    float dotNV = BsdfNDot(wo);

    float denomA = dotNV * sqrt(a2 + (1.0f - a2) * dotNL * dotNL);
    float denomB = dotNL * sqrt(a2 + (1.0f - a2) * dotNV * dotNV);

    return 2.0f * dotNL * dotNV / (denomA + denomB);
}

//====================================================================
void ImportanceSampleGgxD(vec3 wo, in Material material,
                          out vec3 wi, out vec3 reflectance)
{
    float a = material.roughness;
    float a2 = a * a;

    // -- Generate uniform random variables between 0 and 1
    float e0 = randf();
    float e1 = randf();

    // -- Calculate theta and phi for our microfacet normal wm by
    // -- importance sampling the Ggx distribution of normals
    float theta = acos(sqrt((1.0f - e0) / ((a2 - 1.0f) * e0 + 1.0f)));
    float phi   = 2.0f * PI * e1;

    // -- Convert from spherical to Cartesian coordinates
    vec3 wm = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

    // -- Calculate wi by reflecting wo about wm
    wi = 2.0f * dot(wo, wm) * wm - wo;

    // -- Ensure our sample is in the upper hemisphere
    // -- Since we are in tangent space with a y-up coordinate
    // -- system BsdfNDot(wi) simply returns wi.y
    if(BsdfNDot(wi) > 0.0f && dot(wi, wm) > 0.0f) {

    	float dotWiWm = dot(wi, wm);

        // -- calculate the reflectance to multiply by the energy
        // -- retrieved in direction wi
        vec3 F = SchlickFresnel(material.diffuse_color.xyz, dotWiWm);
        float G = SmithGGXMaskingShadowing(wi, wo, a2);
        float weight = abs(dot(wo, wm))
                     / (BsdfNDot(wo) * BsdfNDot(wm));

        reflectance = F * G * weight; 
    }
    else {
        reflectance = vec3(0);
    }
}
