#version 110

uniform sampler2D heightField;
uniform mat4 MVP;

uniform bool displace;

varying vec3 normal;
varying vec3 pos;
varying vec2 texCoords;

void main() {

    vec3 position = gl_Vertex.xyz;
    if (displace) {
        vec4 height = texture2D(heightField, gl_MultiTexCoord0.xy);
        float displacement = 0.3 * height.x;
        position += displacement * gl_Normal;
    }
    else {
        texCoords = gl_MultiTexCoord0.xy;
    }

    gl_Position = MVP * vec4(position, 1);
    pos = position;
    normal = gl_NormalMatrix * normal;
}
