#version 110

#define ATTENUATION_DISTANCE 100.0

uniform sampler2D texture;
uniform sampler2D noiseField;
uniform sampler2D cracks;

uniform vec3 base_color;

varying vec3 pos;
varying vec3 normal;
varying vec2 texCoords;

uniform bool textured;
uniform bool illum;
uniform bool add;

void main() {
    vec3 cameraPosition = vec3(0.0, 0.0, 1.0);
    vec3 lightPosition = vec3(5.0, 5.0, 7.0);

    vec4 final_color;
    
    if (illum) {
        // Calculate colors
        vec3 ambientColor = 0.1 * base_color;
        vec3 diffuseColor = base_color;
        
        // Camera position
        vec3 V = normalize(pos - cameraPosition);
        vec3 L = normalize(lightPosition - pos);
        vec3 H = normalize(L - V);
        vec3 N = normal;
        
        // Calculate ambient
        vec3 ambient = ambientColor;
        
        // Calculate diffuse
        vec3 diffuse = vec3(0);
        diffuse = clamp(dot(L, N), 0.0, 1.0) * diffuseColor;
        
        // Calculate final color
        final_color = vec4(ambient + diffuse, 1.0);
    }
    else {
        final_color = vec4(0.0, 0.7, 0.9, 1.0);
    }
    if (textured) {
        vec4 textureColor = texture2D(noiseField, texCoords);
        vec4 crackColor = texture2D(cracks, texCoords);
        final_color *= textureColor;
    }
    
    // Attenuation factor
    float distance = length(pos - lightPosition);
    float attenuation = ((ATTENUATION_DISTANCE - distance) / ATTENUATION_DISTANCE);
    gl_FragColor = attenuation * final_color;
    
    /* Height debug
    if (pos.z > 0.9)
        gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else if (pos.z > 0.5)
        gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    else if (pos.z > 0.1)
        gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
    else if (pos.z > -0.5)
        gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0);
    else if (pos.z > -1.0)
        gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);
     */
}
