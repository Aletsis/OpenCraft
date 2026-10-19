#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in float aAO;
layout(location = 4) in float aSkyLight;
layout(location = 5) in float aBlockLight;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uSunlightFactor; // 1.0 de día, ~0.2 de noche
uniform float uGamma;          // 0.0 = sombrío, 1.0 = brillante

out vec2 vUV;
out vec3 vLight;
out float vFogDist;

// Curva de brillo clásica por nivel de luz (0..1) con el ajuste de brillo del jugador
float brightness(float level) {
    float inv = 1.0 - level;
    float base = level / (inv * 3.0 + 1.0);
    float lifted = 1.0 - pow(1.0 - base, 4.0);
    return max(mix(base, lifted, uGamma), 0.035);
}

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vec4 viewPos = uView * worldPos;
    gl_Position = uProjection * viewPos;
    vUV = aUV;

    float sun = clamp(uSunlightFactor, 0.0, 1.0);
    float sky = brightness(clamp(aSkyLight / 15.0, 0.0, 1.0) * sun);
    float blk = brightness(clamp(aBlockLight / 15.0, 0.0, 1.0));
    // La luz de antorchas y lava es cálida; la del cielo, neutra
    vec3 light = max(vec3(sky), vec3(blk) * vec3(1.0, 0.88, 0.72));

    // aAO ya incluye el sombreado direccional de cada cara
    vLight = light * aAO;
    vFogDist = length(viewPos.xyz);
}
