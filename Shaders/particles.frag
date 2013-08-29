#version 110

uniform sampler2D texture;
uniform vec3 base_color;
uniform bool textured;
uniform float max_age;

varying float age;
varying vec3 pos;
varying vec2 texCoords;

void main() {
    vec4 final_color;
    if (textured) {
        vec4 blah = texture2D(texture, texCoords);
        float illum = (0.3 * blah.x + 0.59 * blah.y + 0.1 * blah.z);
        final_color = vec4(base_color, illum);
    }
    else {
        final_color = vec4(1.0);
    }
    final_color *= (max_age - age) / max_age;
    final_color = clamp(final_color, 0.0, 0.1);
    
    gl_FragColor = vec4(1.0);
}
