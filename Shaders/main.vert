/* Vertex shader for the main scene, before post-processing effects */

/* Specifies GLSL version 1.10 - corresponds to OpenGL 2.0 */
#version 120

/* Defined in model space */
attribute vec3 vertexCoordinates;
attribute vec3 normalCoordinates;
attribute vec2 textureCoordinates;

/* MVP information */
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 MVP;

/* Interpolated normal, vertex, texture coordinates */
varying vec3 vertexPosition;
varying vec3 normalPosition;
varying vec2 texturePosition;

/* Displacement information */
uniform sampler2D heightMap;
uniform sampler2D normalMap;
uniform bool displace;

/* Fetches the texture height at the given position */
float texHeight(float u, float v)
{
    vec2 texPos = vec2(mod(u, 1.0), mod(v, 1.0));
    vec4 color = texture2D(heightMap, texPos);
    float height = (color.x + color.y + color.z) / (3.0);
    return 0.1 * height;
}

void main()
{
    vec3 position = vertexCoordinates;
    
    if (displace) {
        float x = textureCoordinates.x;
        float y = textureCoordinates.y;
        
        float height = texHeight(x, y);
        position += height * normalCoordinates;
        
        // Fix normal using normal map
        normalPosition = (model * texture2D(normalMap, textureCoordinates)).xyz;
    }
    else {
        normalPosition = (model * vec4(normalCoordinates, 1)).xyz;
    }
    
    // Pass interpolated vertex position and normals to shader
    vertexPosition = (vec4(position, 1)).xyz;
    
    // Pass interpolated texture position along
    texturePosition = textureCoordinates;
    
    // Transform vertex coordinates by MVP
    gl_Position = MVP * vec4(position, 1);
}
