#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D BuildingShadowTex;

in Vertex {
vec2 texCoord;

///////////
vec4 colour;
} IN;


out vec4 fragColour;


void main(void) {

vec4 discardCol = texture(diffuseTex,IN.texCoord);
//if(discardCol.rgb == vec3(1,0,0)){
//discard;}

if(discardCol.a == 0){
discard;}

//fragColour = texture (diffuseTex , IN.texCoord );

//fragColour = texture(diffuseTex, IN.texCoord) + texture(BuildingShadowTex, IN.texCoord);
fragColour = texture(diffuseTex, IN.texCoord) * texture(BuildingShadowTex, IN.texCoord).r*IN.colour;
}


//fragColour = texture (diffuseTex , IN.texCoord ).bgra; // Swizzling
//fragColour = texture (diffuseTex , IN.texCoord ).xxxw; // Swizzling

//////////////////////////
//+ IN.colour is brighter
//* IN.colour is original colour blended