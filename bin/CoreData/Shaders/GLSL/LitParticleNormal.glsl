#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Lighting.glsl"
#include "Fog.glsl"

#ifdef NORMALMAP
    varying vec4 vTexCoord;
    varying vec4 vTangent;
#else
    varying vec2 vTexCoord;
#endif
varying vec3 vNormal;
varying vec4 vWorldPos;
varying vec4 vWorldPos2;
#ifdef VERTEXCOLOR
    varying vec4 vColor;
#endif
#ifdef SOFTPARTICLES
    varying vec4 vScreenPos;
    uniform float cSoftParticleFadeScale;
#endif
#ifdef PERPIXEL
    #ifdef SHADOW
        #ifndef GL_ES
            varying vec4 vShadowPos[NUMCASCADES];
        #else
            varying highp vec4 vShadowPos[NUMCASCADES];
        #endif
    #endif
    #ifdef SPOTLIGHT
        varying vec4 vSpotPos;
    #endif
    #ifdef POINTLIGHT
        varying vec3 vCubeMaskVec;
    #endif
#else
    varying vec3 vVertexLight;
#endif

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

// mat3 rotationMatrix(vec3 axis, float angle)
// {
//     axis = normalize(axis);
//     float s = sin(angle);
//     float c = cos(angle);
//     float oc = 1.0 - c;
//     
//     return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
//                 oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
//                 oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c );
// }

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    
    vNormal = GetWorldNormal(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    // vTexCoord = GetTexCoord(iTexCoord);
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));
    vWorldPos2 = vec4((iPos * modelMatrix).xyz + vec3(iTexCoord1.x, iTexCoord1.y, 0.0) * cBillboardRot, GetDepth(gl_Position));

    #ifdef SOFTPARTICLES
        vScreenPos = GetScreenPos(gl_Position);
    #endif

    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif
    
    #ifdef NORMALMAP
       float M_DEGTORAD = 3.14159 / 180.0;
//         float Sign = 1.0 - step(128.0, iColor.r)*2.0;
//         float Exponent = 2.0 * mod(iColor.r,128.0) + step(128.0,iColor.g) - 127.0; 
//         float Mantissa = mod(iColor.g, 128.0)*65536.0 + iColor.b*256.0 +iColor.a + float(0x800000);
//         float rotation =  Sign * exp2(Exponent) * (Mantissa * exp2(-23.0 )); 
//         // rotation = 45.0;
//         float cosf = cos(rotation * M_DEGTORAD);
//         float sinf = sin(rotation * M_DEGTORAD);
        // float rotation = vColor.r * 360.0;
        
        float rotation = M_DEGTORAD * ((vColor.r * 360.0));
        // float rotation = 0.0;
        
        mat4 rotMat = rotationMatrix(vNormal, -rotation);
    
        // vec4 tangent = GetWorldTangent(modelMatrix);
        
        // vec3 right = rotMat * vec3(1.0, 0.0, 0.0);
        vec3 right = vec3(1.0, 0.0, 0.0);
        vec4 tangent = vec4(normalize(right * cBillboardRot), 1.0);
        tangent = rotMat * tangent;
        
        vec3 bitangent = cross(tangent.xyz, vNormal) * tangent.w;
        vTexCoord = vec4(GetTexCoord(iTexCoord), bitangent.xy);
        vTangent = vec4(tangent.xyz, bitangent.z);
    #else
        vTexCoord = GetTexCoord(iTexCoord);
    #endif

    #ifdef PERPIXEL
        // Per-pixel forward lighting
        vec4 projWorldPos = vec4(worldPos, 1.0);

        #ifdef SHADOW
            // Shadow projection: transform from world space to shadow space
            for (int i = 0; i < NUMCASCADES; i++)
                vShadowPos[i] = GetShadowPos(i, vec3(0, 0, 0), projWorldPos);
        #endif

        #ifdef SPOTLIGHT
            // Spotlight projection: transform from world space to projector texture coordinates
            vSpotPos = projWorldPos * cLightMatrices[0];
        #endif
    
        #ifdef POINTLIGHT
            vCubeMaskVec = (worldPos - cLightPos.xyz) * mat3(cLightMatrices[0][0].xyz, cLightMatrices[0][1].xyz, cLightMatrices[0][2].xyz);
        #endif
    #else
        // Ambient & per-vertex lighting
        vVertexLight = GetAmbient(GetZonePos(worldPos));

        #ifdef NUMVERTEXLIGHTS
            for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
                vVertexLight += GetVertexLightVolumetric(i, worldPos) * cVertexLights[i * 3].rgb;
        #endif
    #endif
}

void PS()
{
    // Get material diffuse albedo
    #ifdef DIFFMAP
        vec4 diffInput = texture2D(sDiffMap, vTexCoord.xy);
        #ifdef ALPHAMASK
            if (diffInput.a < 0.1)
                discard;
        #endif
        vec4 diffColor = cMatDiffColor * diffInput;
    #else
        vec4 diffColor = cMatDiffColor;
    #endif

    #ifdef VERTEXCOLOR
        // diffColor *= vColor;
    #endif

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(vWorldPos.w, vWorldPos.y);
    #else
        float fogFactor = GetFogFactor(vWorldPos.w);
    #endif

    // Soft particle fade
    // In expand mode depth test should be off. In that case do manual alpha discard test first to reduce fill rate
    #ifdef SOFTPARTICLES
        #ifdef EXPAND
            if (diffColor.a < 0.01)
                discard;
        #endif

        float particleDepth = vWorldPos.w;
        #ifdef HWDEPTH
            float depth = ReconstructDepth(texture2DProj(sDepthBuffer, vScreenPos).r);
        #else
            float depth = DecodeDepth(texture2DProj(sDepthBuffer, vScreenPos).rgb);
        #endif

        #ifdef EXPAND
            float diffZ = max(particleDepth - depth, 0.0) * (cFarClipPS - cNearClipPS);
            float fade = clamp(diffZ * cSoftParticleFadeScale, 0.0, 1.0);
        #else
            float diffZ = (depth - particleDepth) * (cFarClipPS - cNearClipPS);
            float fade = clamp(1.0 - diffZ * cSoftParticleFadeScale, 0.0, 1.0);
        #endif
    #endif
    
    // Get normal
    #ifdef NORMALMAP
        mat3 tbn = mat3(vTangent.xyz, vec3(vTexCoord.zw, vTangent.w), vNormal);
        vec3 normal = normalize(tbn * DecodeNormal(texture2D(sNormalMap, vTexCoord.xy)));
    #else
        vec3 normal = normalize(vNormal);
    #endif

    #ifdef PERPIXEL
        // Per-pixel forward lighting
        vec3 lightColor;
        vec3 lightDir;
        vec3 finalColor;

        // float diff = GetDiffuseVolumetric(vWorldPos.xyz);
        // vWorldPos tiene q ver con la direccion de la iluminacion, ver como aplicar/desaplicar?
        // la rotacion que viene iTexCoord1
        // float diff = GetDiffuse(normal, vWorldPos2.xyz, lightDir);
        float diff = max(dot(normal, cLightDirPS), 0.0);

//         #ifdef SHADOW
//             diff *= GetShadow(vShadowPos, vWorldPos.w);
//         #endif
    
//         #if defined(SPOTLIGHT)
//             lightColor = vSpotPos.w > 0.0 ? texture2DProj(sLightSpotMap, vSpotPos).rgb * cLightColor.rgb : vec3(0.0, 0.0, 0.0);
//         #elif defined(CUBEMASK)
//             lightColor = textureCube(sLightCubeMap, vCubeMaskVec).rgb * cLightColor.rgb;
//         #else
//             lightColor = cLightColor.rgb;
//         #endif
//         
//         vec3 specColor = cMatSpecColor.rgb;
//         #ifdef SPECULAR
//             float spec = GetSpecular(normal, cCameraPosPS - vWorldPos.xyz, lightDir, cMatSpecColor.a);
//             finalColor = diff * lightColor * (diffColor.rgb + spec * specColor * cLightColor.a);
//         #else
//             finalColor = diff * lightColor * diffColor.rgb;
//         #endif

        // finalColor = diff * lightColor * diffColor.rgb;
        // gl_FragColor = vec4(GetLitFog(finalColor, fogFactor), diffColor.a);
        //
        gl_FragColor = vec4(vec3(diff), diffColor.a);
        float Sign = 1.0 - step(128.0, vColor.r)*2.0;
        float Exponent = 2.0 * mod(vColor.r,128.0) + step(128.0,vColor.g) - 127.0; 
        float Mantissa = mod(vColor.g, 128.0) * 65536.0 + vColor.b * 256.0 + vColor.a + float(0x800000);
        float rotation =  Sign * exp2(Exponent) * (Mantissa * exp2(-23.0 )); 
        
        // gl_FragColor = vec4(normalize(vec3(rotation, rotation, rotation)), diffColor.a);
    #else
        // Ambient & per-vertex lighting
        vec3 finalColor = vVertexLight * diffColor.rgb;

        gl_FragColor = vec4(GetFog(finalColor, fogFactor), diffColor.a);
    #endif
}
