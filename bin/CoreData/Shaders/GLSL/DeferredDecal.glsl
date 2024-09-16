#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"
#include "Fog.glsl"

varying vec2 vTexCoord;
varying vec4 vWorldPos;
varying vec2 vScreenPos;
#ifdef VERTEXCOLOR
    varying vec4 vColor;
#endif
varying mat4 vModelMatrix;
varying mat4 vModelInv;
varying mat4 vViewInv;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = GetTexCoord(iTexCoord);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
    vWorldPos = vec4(worldPos, 1.0) * cView;
    // vWorldPos = vec4(worldPos, GetDepth(gl_Position));
    vModelMatrix = iModelMatrix;
    vModelInv = cModelInv;
    // vModelInv = mat4(1.0);
    vViewInv = cViewInv;
    
    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif
}

void PS()
{
    //Sample a value from the depth buffer
    float depth = ReconstructDepth(texture2D(sDepthBuffer, vScreenPos.xy).r);
    
    vec3 viewRay = vWorldPos.xyz * (cFarClipPS / vWorldPos.z);
    vec3 viewPosition = viewRay * depth;
    
    vec3 worldPosition = (vec4(viewPosition, 1.0) * vViewInv).xyz;
    vec4 objectPosition = vec4(worldPosition, 1.0) * vModelInv;
    
    float threshold = 0.5;
    float posAbsX = abs(objectPosition.x);
    float posAbsY = abs(objectPosition.y);
    float posAbsZ = abs(objectPosition.z);
    if (posAbsX > threshold || posAbsY > threshold || posAbsZ > threshold)
        discard;

    float xTexOffset = 0.5;
    float yTexOffset = 0.5;
    vec4 diffColor = cMatDiffColor * texture2D(sDiffMap, vec2(objectPosition.x + xTexOffset, objectPosition.y + yTexOffset));
    
    if (diffColor.a < 0.5)
        discard;
        
     gl_FragColor = diffColor;
}
