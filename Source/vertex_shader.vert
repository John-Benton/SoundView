#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

float base_change_denom = 3.321928; //change base 2 to base 10

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
	
	TexCoord = aTexCoord;
		
}