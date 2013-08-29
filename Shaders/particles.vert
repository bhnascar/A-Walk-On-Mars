#version 110

attribute vec3 vertexCoordinates;
attribute vec2 textureCoordinates;
attribute vec3 particleAge;

uniform mat4 MVP;

varying vec3 pos;
varying vec2 texCoords;
varying float age;

void main()
{
    vec3 position = vertexCoordinates.xyz;

    gl_Position = MVP * vec4(position, 1);
    pos = position;
    texCoords = textureCoordinates;
    age = particleAge.x + particleAge.y + particleAge.z;
}
