#version 110

#define ATTENUATION_DISTANCE 50.0

uniform sampler2D texture;
uniform sampler2D noiseField;
uniform sampler2D cracks;
uniform sampler2D normalMap;

uniform vec3 base_color;

/* Defined in world space */
uniform vec3 lightPosition;
varying vec3 vertexPosition;
varying vec3 normal;
varying vec2 texCoords;

uniform bool textured;
uniform bool illum;
uniform bool add;

void main()
{
    vec4 final_color;
    
    // Illumination
    if (illum) {
        // Calculate colors
        vec3 ambientColor = 0.1 * base_color;
        vec3 diffuseColor = base_color;
        vec3 specularColor = vec3(1.0);
        
        // Camera position
        vec3 L = normalize(lightPosition - vertexPosition);
        vec3 N = normalize(normal);
        
        // Calculate ambient
        vec3 ambient = ambientColor;
        
        // Calculate diffuse
        vec3 diffuse = vec3(0);
        diffuse = clamp(dot(L, N), 0.0, 1.0) * diffuseColor;
        
        // Calculate final color
        final_color = vec4(diffuse, 1.0);
    }
    else {
        final_color = vec4(base_color, 1.0);
    }
    
    // Texturing
    if (textured) {
        vec4 textureColor = texture2D(noiseField, texCoords);
        vec4 crackColor = texture2D(cracks, texCoords);
        final_color *= textureColor;
    }
    
    // Attenuation factor
    float distance = length(vertexPosition - lightPosition);
    float attenuation = ((ATTENUATION_DISTANCE - distance) / ATTENUATION_DISTANCE);
    gl_FragColor = attenuation * final_color;
}
