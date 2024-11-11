#pragma once

const char* vertexShaderSource = R"(#version 300 es
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
})";

const char* fragmentShaderSource = R"(#version 300 es
precision highp float;
uniform sampler2D yTexture;
uniform sampler2D uvTexture;
in vec2 TexCoord;
out vec4 FragColor;

void main() {
    float y = texture(yTexture, TexCoord).r;
    vec2 uv = texture(uvTexture, TexCoord).rg - vec2(0.5, 0.5);
    
    // YUV to RGB conversion
    vec3 rgb;
    rgb.r = y + 1.403 * uv.y;
    rgb.g = y - 0.344 * uv.x - 0.714 * uv.y;
    rgb.b = y + 1.770 * uv.x;
    
    FragColor = vec4(rgb, 1.0);
})"; 