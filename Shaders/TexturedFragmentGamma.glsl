#version 330 core
uniform sampler2D diffuseTex;

in Vertex {
vec2 texCoord;
} IN;


out vec4 fragColour;


void main(void) {

float gamma = 2.2;

vec4 diffuse = texture(diffuseTex, IN.texCoord );
fragColour.rgb = pow(diffuse.rgb, vec3(1.0/gamma));
fragColour.a = diffuse.a;
}

