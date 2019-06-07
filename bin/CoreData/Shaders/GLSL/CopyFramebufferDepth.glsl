#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"

varying vec2 vScreenPos;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}

void PS()
{
    // gl_FragColor = texture2D(sDiffMap, vScreenPos);
    float x = vScreenPos.x;
    float y = 1.0f - vScreenPos.y;
    // float depth = ReconstructDepth(texture2D(sDepthBuffer, vec2(x, y)).r);
    float depth = DecodeDepth(texture2D(sDepthBuffer, vec2(x, y)).rgb);
    // float depth = ReconstructDepth(texture2DProj(sDepthBuffer, vScreenPos).r);
    // float depth = DecodeDepth(texture2D(sDepthBuffer, vScreenPos).rgb);

    vec3 color = vec3(depth, depth, depth);
    gl_FragColor = vec4(color, 1.0);
}

