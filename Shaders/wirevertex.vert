#version 110

uniform sampler2D heightField;
uniform mat4 MVP;

uniform bool displace;

varying vec3 normal;
varying vec3 pos;
varying vec2 texCoords;

varying float height;

float delta = 0.001;

/* Fetches the texture height at the given position */
float texHeight(float u, float v)
{
    vec2 texPos = vec2(mod(u, 1.0), mod(v, 1.0));
    vec4 color = texture2D(heightField, texPos);
    float height = (color.x + color.y + color.z) / (3.0);
    return 0.05 * height;
}

/* Performs vertex displacement based on the given texture height map */
void main()
{
    vec3 position = gl_Vertex.xyz;
    if (displace) {
        float x = gl_MultiTexCoord0.x;
        float y = gl_MultiTexCoord0.y;
        height = texHeight(gl_MultiTexCoord0.x, gl_MultiTexCoord0.y);
        position -= height * gl_Normal;
        
        float dh_dx = (texHeight(x + delta, y) - texHeight(x, y)) / delta;
        float dh_dy = (texHeight(x, y + delta) - texHeight(x, y)) / delta;
        
        vec3 tan1 = normalize(vec3(1, dh_dx, 0));
        vec3 tan2 = normalize(vec3(0, -dh_dy, -1));
        normal = cross(tan1, tan2);
    }
    else {
        normal = gl_NormalMatrix * normal;
    }

    gl_Position = MVP * vec4(position, 1);
    texCoords = gl_MultiTexCoord0.xy;
    pos = position;
}
