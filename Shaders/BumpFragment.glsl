#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D bumpTex; // New!
uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

uniform sampler2D shadowTex; ////

in Vertex {
vec3 colour;
vec2 texCoord;
vec3 normal;
vec3 tangent; // New!
vec3 binormal; // New!
vec3 worldPos;

vec4 shadowProj; ////


} IN;

out vec4 fragColour;


void main(void) {
vec3 incident = normalize ( lightPos - IN.worldPos );
vec3 viewDir = normalize ( cameraPos - IN.worldPos );
vec3 halfDir = normalize ( incident + viewDir );

mat3 TBN = mat3( normalize (IN.tangent ), normalize (IN.binormal ), normalize (IN.normal ));  /// new

vec4 diffuse = texture (diffuseTex , IN.texCoord );
vec3 bumpNormal = texture (bumpTex , IN.texCoord ).rgb;  /// new
bumpNormal = normalize (TBN * bumpNormal * 2.0 - 1.0);  ////////////////// new

float lambert = max(dot(incident , bumpNormal ), 0.0f);
float distance = length(lightPos - IN.worldPos);
float attenuation = 1.0 - clamp( distance / lightRadius , 0.0, 1.0);

float specFactor = clamp (dot(halfDir , bumpNormal ) ,0.0 ,1.0);
specFactor = pow(specFactor , 60.0 );

vec3 surface = ( diffuse.rgb * lightColour.rgb );
fragColour.rgb = surface * lambert * attenuation ;
fragColour.rgb += ( lightColour.rgb * specFactor )* attenuation *0.33;
fragColour.rgb += surface * 0.1f;
fragColour.a = diffuse.a;


}