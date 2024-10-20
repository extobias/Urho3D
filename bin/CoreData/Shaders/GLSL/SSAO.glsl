#version 450

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Constants.glsl"

varying highp vec2 vScreenPos;
//varying vec4 vProjInfo;

#ifdef COMPILEVS

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPosPreDiv(gl_Position);

    //mat4 projMat = cViewInv * cViewProj;
    //float tan_half_fov_x = 1/projMat[0][0];
    //float tan_half_fov_y = 1/projMat[1][1];
    //vProjInfo.x = tan_half_fov_x * 2.0;
    //vProjInfo.y = tan_half_fov_y *-2.0;
    //vProjInfo.z =-tan_half_fov_x;
    //vProjInfo.w = tan_half_fov_y;
}

#endif


#ifdef COMPILEPS
uniform sampler2D sRnd0; // Random noise
uniform sampler2D sDep1; // Depth Buffer
uniform vec2 cNoiseScale;
uniform vec4 cProjInfo;

float camClip = cFarClipPS - cNearClipPS;

const float TWO_PI = M_PI * 2.0;
const int samples = 8;
const int steps = 3;

const vec2 kernel[8] = vec2[](
    vec2(  1, 0),
    vec2(  0, 1),
    vec2( -1, 0),
    vec2(  0,-1),
    vec2( 0.5,0),
    vec2(0, 0.5),
    vec2(-0.5,0),
    vec2(0,-0.5)
);

float GetDepth(sampler2D depthSampler, vec2 uv)
{
    #ifdef HWDEPTH
        return ReconstructDepth(texture2D(depthSampler, uv).r);
    #else
        return DecodeDepth(texture2D(depthSampler, uv).rgb);
    #endif
}

vec3 GetPosFromDepth(float depth, vec2 uv)
{
    float eyeZ = depth * camClip;
    vec2 tmp = uv;
    tmp.y = 1-tmp.y;
    vec3 p = vec3((cProjInfo.xy * tmp + cProjInfo.zw) * eyeZ, eyeZ);
    return p;
}

vec3 GetPos(vec2 uv)
{
    return GetPosFromDepth(GetDepth(sDep1, uv), uv);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
  vec3 V1 = Pr - P;
  vec3 V2 = P - Pl;
  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 GenNrm(vec3 p0, vec2 texcoord)
{

    vec3 p1X = GetPos(texcoord+vec2(cGBufferInvSize.x, 0.0));
    vec3 p2X = GetPos(texcoord-vec2(cGBufferInvSize.x, 0.0));

    vec3 p1Y = GetPos(texcoord+vec2(0.0, cGBufferInvSize.y));
    vec3 p2Y = GetPos(texcoord-vec2(0.0, cGBufferInvSize.y));

    return -normalize(cross(MinDiff(p0, p1X, p2X), MinDiff(p0, p1Y, p2Y)));
}

vec2 GetCoord(int i, float radius, mat2 rotationMatrix)
{
    vec2 ray = kernel[i] * rotationMatrix;

    return radius * ray;
}

float GenAO(vec2 texcoord, vec3 p, vec3 n, float radius)
{
    vec3 pos = GetPos(texcoord);
    vec3 diff = pos - p;
    float distSqr = dot(diff, diff);
    float invLength = inversesqrt(distSqr);
    float angle = dot(n, diff) * invLength;

    // ao attenuation/falloff
    float f = 1.0/(1.0+distSqr);

    return max(0, angle) * f;
}

const float KERNEL_RADIUS = 6;
uniform vec2 cBlurDir;

float Blur(vec2 texcoord, float baseD, float r, inout float w_total)
{
    float ao = texture2D(sDiffMap, texcoord).r;
    float d = GetDepth(sDep1, texcoord) * camClip;

    const float BlurSigma = float(KERNEL_RADIUS) * 0.5;
    const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);

    float ddiff = max(0.001, 0.5 - abs(baseD - d) * 2.0);
    float w = exp2(-r*r*BlurFalloff) * ddiff;
    w_total += w;

    return ao*w;
}

void PS()
{
    #if defined(GEN_AO)
        const float aoStrength = 2.0;
        const float radius = 0.35;

        highp vec2 tx = vScreenPos;

        float depth = GetDepth(sDep1, vScreenPos);
        vec3 pos = GetPosFromDepth(depth, vScreenPos);
        vec3 normal = GenNrm(pos, vScreenPos);
        vec3 random = texture2D(sRnd0, vScreenPos / (cNoiseScale * cGBufferInvSize)).xyz * 2.0 - 1.0;

        if( depth >= 1.0 ) {
		    // no SSAO on the skybox !
		    gl_FragColor = vec4(1.0);
		    return;
        }

        // avoids darkening at screen edges. taken from:
        // https://github.com/GameTechDev/ASSAO/blob/master/Projects/ASSAO/ASSAO/ASSAO.hlsl
        float edge = min( min( tx.x, 1.0 - tx.x ), min( tx.y, 1.0 - tx.y ) );
        edge = clamp( 5.0 * edge + 0.001, 0.0, 1.0);
        
        mat2 rotationMatrix = mat2(random.x, -random.y, random.y,  random.x);

        float radBase = edge * radius / pos.z;
        float radStep = radBase / float(steps);

        float ao = 0.0;

        for (int i = 0; i < samples; i++) {
            float rad = radStep;

            for (int j = 0; j < steps; j++)
            {
                float currentStep = float(1+j);
                vec2 coord = (random.z + currentStep) * GetCoord(i, rad, rotationMatrix);
                ao += GenAO(tx + coord, pos, normal, radius) / currentStep;
                rad += radStep;
            }
        }

        ao = 1.0-clamp(ao/float(samples), 0.0, 1.0);
        ao = pow(ao, aoStrength);

        gl_FragColor.rgb = vec3(ao);
        gl_FragColor.a = 1.0;
    #elif defined(GEN_BLUR)
        float ao = texture2D(sDiffMap, vScreenPos).r;
        float depth = GetDepth(sDep1, vScreenPos) * camClip;

        float w_total = 1.0;

        for (lowp float r = 1; r <= KERNEL_RADIUS; ++r)
        {
            vec2 offset = cBlurDir * cGBufferInvSize * r;
            ao += Blur(vScreenPos+offset, depth, r, w_total);
            ao += Blur(vScreenPos-offset, depth, r, w_total); 
        }

        gl_FragColor.rgb = vec3(ao/w_total);
        gl_FragColor.a = 1.0;
    #else
        float ao = texture2D(sDiffMap, vScreenPos).r;
        gl_FragColor.rgb = vec3(ao);
        gl_FragColor.a = 1.0;
    #endif
}

#endif