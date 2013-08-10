/*
 ** Copyright (c) 2012, Romain Dura romain@shazbits.com
 **
 ** Permission to use, copy, modify, and/or distribute this software for any
 ** purpose with or without fee is hereby granted, provided that the above
 ** copyright notice and this permission notice appear in all copies.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 ** WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 ** MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 ** SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 ** WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 ** ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 ** IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 ** Photoshop & misc math
 ** Blending modes, RGB/HSL/Contrast/Desaturate, levels control
 **
 ** Romain Dura | Romz
 ** Blog: http://mouaif.wordpress.com
 ** Post: http://mouaif.wordpress.com/?p=94
 */

/* Fragment shader for the main scene, before post-processing effects */

/* Specifies GLSL version 1.10 - corresponds to OpenGL 2.0 */
#version 120

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

#define M_PI 3.14159265358979323846264

vec4 Desaturate(vec3 color, float Desaturation)
{
	vec3 grayXfer = vec3(0.3, 0.59, 0.11);
	float grayf = dot(grayXfer, color);
	vec3 gray = vec3(grayf, grayf, grayf);
	return vec4(mix(color, gray, Desaturation), 1.0);
}

/*
 ** Contrast, saturation, brightness
 ** Code of this function is from TGM's shader pack
 ** http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=21057
 */

// For all settings: 1.0 = 100% 0.5=50% 1.5 = 150%
vec3 ContrastSaturationBrightness(vec3 color, float brt, float sat, float con)
{
	// Increase or decrease theese values to adjust r, g and b color channels seperately
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5;
	const float AvgLumB = 0.5;
	
	const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
	
	vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
	vec3 brtColor = color * brt;
	float intensityf = dot(brtColor, LumCoeff);
	vec3 intensity = vec3(intensityf, intensityf, intensityf);
	vec3 satColor = mix(intensity, brtColor, sat);
	vec3 conColor = mix(AvgLumin, satColor, con);
	return conColor;
}

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
