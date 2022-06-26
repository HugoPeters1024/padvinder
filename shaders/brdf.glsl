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
                          out vec3 wi, out vec3 reflectance, out vec3 wm)
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
    wm = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

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

//====================================================================
float SmithGGXMasking(vec3 wi, vec3 wo, float a2)
{
    float dotNL = BsdfNDot(wi);
    float dotNV = BsdfNDot(wo);
    float denomC = sqrt(a2 + (1.0f - a2) * dotNV * dotNV) + dotNV;

    return 2.0f * dotNV / denomC;
}


vec3 SampleGGXVNDF(vec3 V_, float alpha_x, float alpha_y, float U1, float U2)
{
    // stretch view
    vec3 V = normalize(vec3(alpha_x * V_.x, alpha_y * V_.y, V_.z));
    // orthonormal basis
    vec3 T1 = (V.z < 0.9999) ? normalize(cross(V, vec3(0,0,1))) : vec3(1,0,0);
    vec3 T2 = cross(T1, V);
    // sample point with polar coordinates (r, phi)
    float a = 1.0 / (1.0 + V.z);
    float r = sqrt(U1);
    float phi = (U2<a) ? U2/a * PI : PI + (U2-a)/(1.0-a) * PI;
    float P1 = r*cos(phi);
    float P2 = r*sin(phi)*((U2<a) ? 1.0 : V.z);
    // compute normal
    vec3 N = P1*T1 + P2*T2 + sqrt(max(0.0, 1.0 - P1*P1 - P2*P2))*V;
    // unstretch
    N = normalize(vec3(alpha_x*N.x, alpha_y*N.y, max(0.0, N.z)));
    return N;
}

//====================================================================
void ImportanceSampleGgxVdn(vec3 wo, Material material,
                            out vec3 wi, out vec3 reflectance, out vec3 wm)
{
    vec3 specularColor = material.diffuse_color.xyz;
    float a = material.roughness;
    float a2 = a * a;

    float r0 = randf(); 
    float r1 = randf(); 
    wm = SampleGGXVNDF(wo, a,a,r0,r1);

    // -- Calculate wi by reflecting wo about wm
    wi = 2.0f * dot(wo, wm) * wm - wo;

    if(BsdfNDot(wo) > 0.0f && BsdfNDot(wi) > 0.0f && dot(wi, wm) > 0.0f) {
        vec3 F = SchlickFresnel(specularColor, dot(wi, wm));
        float G1 = SmithGGXMasking(wi, wo, a2);
        float G2 = SmithGGXMaskingShadowing(wi, wo, a2);

        reflectance = F * (G2 / G1);
    }
    else {
        reflectance = vec3(0);
    }
}
