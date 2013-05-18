#version 110

uniform sampler2D heightField;
uniform mat4 MVP;

varying vec3 normal;
varying vec3 pos;
varying vec3 base_color;

void main() {
    vec4 height = texture2D(heightField, gl_MultiTexCoord0.xy);
    float displacement = 0.3 * height.x;

    vec3 position = gl_Vertex.xyz;
    position += displacement * gl_Normal;
    
    if (abs(gl_MultiTexCoord0.x - 0.5) < 0.1)
        position -= displacement * 3.0 * gl_Normal;

    gl_Position = MVP * vec4(position, 1);
    pos = position;
    normal = gl_NormalMatrix * normal;
}
