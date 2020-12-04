#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;
uniform samplerCube cubeTex;


uniform vec3 cameraPos;

in Vertex {
vec4 colour;
vec2 texCoord;
vec3 normal;
vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void) {

vec4 diffuse = texture (diffuseTex , IN.texCoord );
vec3 viewDir = normalize ( cameraPos - IN.worldPos );

vec3 reflectDir = reflect (-viewDir , normalize (IN.normal ));

vec4 test = texture(bumpTex, IN.texCoord);
vec4 reflectTex = texture(cubeTex , reflectDir ) ;


fragColour = reflectTex + ( diffuse * 0.25f) + vec4(0.25,0.25,0.6,0) *test/5;  //colour of water mixed with reflection 

//mirror effect
//fragColour = reflectTex + ( diffuse * 0.25f) * vec4(0,0,1,0.5);  //if plus vec4 then retains water texture
}