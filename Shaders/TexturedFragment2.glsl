#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D testTex;

in Vertex {
vec2 texCoord;

///////////
//vec4 colour;
} IN;


out vec4 fragColour;


void main(void) {



fragColour = texture(diffuseTex, IN.texCoord ).bgga;

//fragColour = texture(diffuseTex, IN.texCoord) + texture(testTex, IN.texCoord)*IN.colour;
}


//fragColour = texture (diffuseTex , IN.texCoord ).bgra; // Swizzling
//fragColour = texture (diffuseTex , IN.texCoord ).xxxw; // Swizzling

//////////////////////////
//+ IN.colour is brighter
//* IN.colour is original colour blended