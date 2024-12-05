#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform float coef;

out vec3 ourColor;
out vec2 TexCoord;
uniform mat4 model;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord *  vec2(coef, 1.0);
}
