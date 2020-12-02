#version 330 core

in Vertex {
vec4 colour;
} IN;

out vec4 fragColour;

void main(void) {

//if(gl_FragCoord.x > 650)
       // fragColour = vec4(1.0, 0.0, 0.0, 1.0);
   // else
		fragColour = IN.colour;
		fragColour.a = 0.5;
}