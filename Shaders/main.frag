/* Fragment shader for the main scene, before post-processing effects */

/* Specifies GLSL version 1.10 - corresponds to OpenGL 2.0 */
#version 120

#define ATTENUATION_DISTANCE 20

uniform sampler2D texture;

uniform vec3 baseColor;

/* Illumination model
 * 0 - Flat
 * 1 - Phong
 */
uniform bool illum;

/* Textured? */
uniform bool textured;

/* Attenuate color based on distance? */
uniform bool attenuate;

/* Light position in camera space */
uniform vec3 lightPosition;

/* Interpolated vertex position from vertex shader */
varying vec3 vertexPosition;
varying vec3 normalPosition;
varying vec2 texturePosition;

void main()
{
    vec3 final_color;
    if (illum) {
        vec3 color = baseColor;
        
        // Calculate colors
        vec3 ambientColor = 0.1f * color;
        vec3 diffuseColor = color;
        
        // Camera position
        vec3 L = normalize(lightPosition - vertexPosition);
        vec3 N = normalize(normalPosition);
        
        // Calculate ambient
        vec3 ambient = ambientColor;
        
        // Calculate diffuse
        vec3 diffuse = vec3(0);
        diffuse = clamp(dot(L, N), 0.0, 1.0) * diffuseColor;
        
        // Calculate final color
        final_color = ambient + diffuse;
    }
    else {
        final_color = baseColor;
    }
    
    gl_FragColor = vec4(final_color, 1.0);
    
    if (textured) {
        gl_FragColor *= texture2D(texture, texturePosition);
    }
    
    if (attenuate) {
        // Attenuation factor
        float distance = length(vertexPosition - lightPosition);
        float attenuation = ((ATTENUATION_DISTANCE - distance) / ATTENUATION_DISTANCE);
        gl_FragColor *= attenuation;
    }
}
