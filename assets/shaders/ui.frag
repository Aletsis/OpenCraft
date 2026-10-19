#version 300 es
precision mediump float;

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 fragColor;

void main() {
    if (uUseTexture) {
        vec4 tex = texture(uTexture, vUV);
        fragColor = tex * vColor;
    } else {
        fragColor = vColor;
    }
}
