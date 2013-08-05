#version 110

uniform sampler2D heightField;
uniform sampler2D normalMap;
uniform mat4 MVP;
uniform mat4 M;

uniform bool displace;

/* Defined in world space */
varying vec3 normal;
varying vec3 vertexPosition;
varying vec2 texCoords;

varying float height;

/* Fetches the texture height at the given position */
float texHeight(float u, float v)
{
    vec2 texPos = vec2(mod(u, 1.0), mod(v, 1.0));
    vec4 color = texture2D(heightField, texPos);
    float height = (color.x + color.y + color.z) / (3.0);
    return 0.05 * height;
}

/* Performs vertex displacement based on the given texture height map 
 * H(x, y) = (x, y, f(x, y))
 * dH/dx = (1, 0, df/dx)
 * dH/dy = (0, 1, df/dy)
 */
void main()
{
    vec3 position = gl_Vertex.xyz;
    if (displace) {
        float x = gl_MultiTexCoord0.x;
        float y = gl_MultiTexCoord0.y;
        
        height = texHeight(x, y);
        position += height * gl_Normal;
        
        // Fix normal using normal map
        normal = texture2D(normalMap, gl_MultiTexCoord0.xy).xyz;
    }
    else {
        normal = (M * vec4(gl_Normal, 1.0)).xyz;
    }

    gl_Position = MVP * vec4(position, 1.0);
    texCoords = gl_MultiTexCoord0.xy;
    vertexPosition = position;
}
