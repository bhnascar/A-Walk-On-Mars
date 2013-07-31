#version 110

uniform sampler2D texture;
uniform vec3 base_color;

varying vec3 pos;
varying vec3 normal;
varying vec2 texCoords;

uniform bool textured;
uniform bool illum;

void main() {
    vec3 cameraPosition = vec3(0.0, 0.0, 1.0);
    vec3 lightPosition = vec3(0.0, 1.0, 1.0);

    vec4 final_color;
    
    if (textured) {
        vec4 blah = texture2D(texture, texCoords);
        float illum = (0.3 * blah.x + 0.59 * blah.y + 0.1 * blah.z);
        final_color = vec4(1.0, 0.0, 0.0, illum);
    }
    else if (illum) {
        // Calculate colors
        vec3 ambientColor = 0.3 * base_color;
        vec3 diffuseColor = base_color;
        
        // Camera position
        vec3 V = normalize(pos - cameraPosition);
        vec3 L = normalize(lightPosition - pos);
        vec3 H = normalize(L - V);
        vec3 N = normalize(-cross(dFdx(pos), dFdy(pos)));
        
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
    
    gl_FragColor = final_color;
}
