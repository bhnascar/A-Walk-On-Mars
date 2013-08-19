/* Fragment shader for the main scene, before post-processing effects */

/* Specifies GLSL version 1.10 - corresponds to OpenGL 2.0 */
#version 120

#include "filters.frag"

#define ATTENUATION_DISTANCE 20

uniform sampler2D texture;
uniform sampler2D sand;
uniform sampler2D rock;

uniform vec3 baseColor;

/* Illumination model
 * 0 - Flat
 * 1 - Phong
 */
uniform bool illum;

/* Textured? */
uniform bool textured;

/* Bump mapped? */
uniform bool bumpMapped;

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
        
        // Perturb normal
        if (bumpMapped)
        {
            vec3 T = texture2D(sand, texturePosition * 30.0).xyz;
            N = normalize((2 * N + T) / length(2 * N + T));
            
            float angle = abs(acos(dot(N, vec3(0.0, 0.0, 1.0))));
            if (angle > M_PI / 4.0)
            {
                float weight = (angle - M_PI / 4.0) / (M_PI / 4.0);
                
                T = weight * 1.2 * texture2D(rock, texturePosition * 70).xyz;
                N = normalize((2 * N + T) / length(2 * N + T));
            }
        }
        
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
    
    final_color = Desaturate(final_color, 0.4).xyz;
    final_color = ContrastSaturationBrightness(final_color, 1.0, 1.0, 1.6);
    gl_FragColor = vec4(final_color, 1.0);
    
    if (textured) {
        gl_FragColor *= texture2D(texture, texturePosition * 50.0);
    }
    
    if (attenuate) {
        // Attenuation factor
        float distance = length(vertexPosition - lightPosition);
        float attenuation = ((ATTENUATION_DISTANCE - distance) / ATTENUATION_DISTANCE);
        gl_FragColor *= attenuation;
    }
}
