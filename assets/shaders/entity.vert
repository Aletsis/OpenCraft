#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aLight; // x = cielo, y = bloque (0..15)

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uSunlightFactor;
uniform float uGamma;
uniform float uFullBright; // 1.0 para geometría sin iluminación (contornos, mano en pantalla)

out vec2 vUV;
out vec4 vColor;
out float vFogDist;

float brightness(float level) {
    float inv = 1.0 - level;
    float base = level / (inv * 3.0 + 1.0);
    float lifted = 1.0 - pow(1.0 - base, 4.0);
    return max(mix(base, lifted, uGamma), 0.035);
}

void main() {
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position = uProjection * viewPos;
    vUV = aUV;
    float sky = brightness(clamp(aLight.x / 15.0, 0.0, 1.0) * clamp(uSunlightFactor, 0.0, 1.0));
    float blk = brightness(clamp(aLight.y / 15.0, 0.0, 1.0));
    vec3 light = mix(max(vec3(sky), vec3(blk) * vec3(1.0, 0.88, 0.72)), vec3(1.0), uFullBright);
    vColor = vec4(aColor.rgb * light, aColor.a);
    vFogDist = length(viewPos.xyz);
}
