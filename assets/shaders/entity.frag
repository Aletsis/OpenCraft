#version 300 es
precision mediump float;

in vec2 vUV;
in vec4 vColor;
in float vFogDist;

uniform sampler2D uTextureAtlas;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

out vec4 fragColor;

void main() {
    vec4 tex = texture(uTextureAtlas, vUV);
    vec4 c = tex * vColor;
    if (c.a < 0.05) {
        discard;
    }
    float fogFactor = clamp((vFogDist - uFogStart) / max(uFogEnd - uFogStart, 1.0), 0.0, 1.0);
    fragColor = vec4(mix(c.rgb, uFogColor, fogFactor), c.a);
}
