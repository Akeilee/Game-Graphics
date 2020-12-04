#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D diffuseLight;
uniform sampler2D specularLight;

in Vertex {
vec2 texCoord;
} IN;

out vec4 fragColour;


void main(void) {
vec4 discardCol0 = texture(diffuseTex,IN.texCoord); //make sure background is not drawn
if(discardCol0.a == 0){
texture(diffuseTex,IN.texCoord);
discard;
}


vec4 discardCol = texture(diffuseLight,IN.texCoord);
if(discardCol.a == 0){
//discard;
}

vec4 discardCol2 = texture(specularLight,IN.texCoord);
if(discardCol2.a == 0){
//discard;
}


vec3 diffuse = texture ( diffuseTex , IN.texCoord ).xyz;
vec3 light = texture ( diffuseLight , IN.texCoord ).xyz;
vec3 specular = texture ( specularLight , IN.texCoord ).xyz;

fragColour.xyz = diffuse * 0.1; // ambient
fragColour.xyz += diffuse * light; // lambert
fragColour.xyz += specular; // Specular
fragColour.a = 1.0;
}