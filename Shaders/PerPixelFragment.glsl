#version 330 core

uniform sampler2D diffuseTex;
uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

in Vertex {
vec3 colour;
vec2 texCoord; 
vec3 normal;
vec3 worldPos;
} IN;

out vec4 fragColour;


void main(void) {

//making vector size of 1
vec3 incident = normalize ( lightPos - IN.worldPos );
vec3 viewDir = normalize ( cameraPos - IN.worldPos );
vec3 halfDir = normalize ( incident + viewDir );

vec4 diffuse = texture (diffuseTex , IN.texCoord); // texture

float lambert = max(dot(incident , IN.normal ), 0.0f); //dot = 1 then max brightness. normal and incident pointing at same direction
float distance = length ( lightPos - IN.worldPos );  //distance from light source to vector
float attenuation = 1.0 - clamp( distance / lightRadius , 0.0, 1.0); //reduce in light beam with respect to distance

float specFactor = clamp (dot(halfDir , IN.normal ) ,0.0 ,1.0); ///////////// how bright spec light reflects into camera
specFactor = pow(specFactor , 60.0 ); //lower number more shiny


vec3 surface = ( diffuse.rgb * lightColour.rgb ); // diffuse.  (light x tex colour)

fragColour.rgb = surface * lambert * attenuation ; 
fragColour.rgb += ( lightColour.rgb * specFactor )* attenuation *0.33;  //0.33 lowers brighness of specular componenet
fragColour.rgb += surface * 0.1f; //ambient !
fragColour.a = diffuse.a; //alpha
}


//vec3 surface = ( diffuse.rgb * lightColour.rgb * vec3(1,0,1) ); //diffuse  (vec3(1,0,0) makes texture red
//fragColour.rgb += ( lightColour.rgb * specFactor * vec3(1,0,1))* attenuation *0.33;  //times by vec3 makes spec light turn pink