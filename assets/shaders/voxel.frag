#version 300 es
precision mediump float;

in vec2 vUV;
in vec3 vLight;
in float vFogDist;

uniform sampler2D uTextureAtlas;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform float uAlpha;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(uTextureAtlas, vUV);

    // Recorte alfa para hojas, cristal, plantas y antorchas
    if (texColor.a < 0.1) {
        discard;
    }

    vec3 litColor = texColor.rgb * vLight;
    float fogFactor = clamp((vFogDist - uFogStart) / max(uFogEnd - uFogStart, 1.0), 0.0, 1.0);
    fragColor = vec4(mix(litColor, uFogColor, fogFactor), texColor.a * uAlpha);
}
