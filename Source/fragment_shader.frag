#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;

float saturation = 1.0;
float lightness;

float highest_ampltidue_color_deg = 0.0;
float lowest_amplitude_color_deg = 240.0;

float lower_amplitude_limit = 0.01;
float upper_amplitde_limit = 1.0;

float R;
float G;
float B;

float R1;
float G1;
float B1;

void main()
{
	   	
	vec4 texture_value = texture(ourTexture, TexCoord);
	
	float texture_red_channel = texture_value.x;
	
	float hue_proportion;
	
	if(texture_red_channel < lower_amplitude_limit)
	{
		hue_proportion = 0.0;
		lightness = 0.5 * (texture_red_channel / lower_amplitude_limit);
	}
	
	if(texture_red_channel > upper_amplitde_limit)
	{
		hue_proportion = 1.0;
		lightness = 0.5;
	}
	
	if(texture_red_channel > lower_amplitude_limit && texture_red_channel < upper_amplitde_limit)
	{
		hue_proportion = texture_red_channel;
		lightness = 0.5;
	}

	float hue_deg = lowest_amplitude_color_deg - (hue_proportion * lowest_amplitude_color_deg);
	
	//==========//
	
	//conversion from HSL to RGB from https://en.wikipedia.org/wiki/HSL_and_HSV
	
	float C = (1.0 - abs((2.0*lightness-1.0))) * saturation;
		
	float Hprime = hue_deg / 60.0;
	
	float X = C * (1.0-abs((mod(Hprime,2.0)-1.0)));

	if(Hprime >= 0 && Hprime <= 1){
		R1 = C;
		G1 = X;
		B1 = 0;
	}
	
	if(Hprime >= 1 && Hprime <= 2){
		R1 = X;
		G1 = C;
		B1 = 0;
	}
	
	if(Hprime >= 2 && Hprime <= 3){
		R1 = 0;
		G1 = C;
		B1 = X;
	}
	
	if(Hprime >= 3 && Hprime <= 4){
		R1 = 0;
		G1 = X;
		B1 = C;
	}
	
	if(Hprime >= 4 && Hprime <= 5){
		R1 = X;
		G1 = 0;
		B1 = C;
	}
	
	if(Hprime >= 5 && Hprime <= 6){
		R1 = C;
		G1 = 0;
		B1 = X;
	}
	
	float m = lightness - (C/2.0);
	
	R = R1 + m;
	G = G1 + m;
	B = B1 + m;
	
	FragColor = vec4(R,G,B,1.0);	
	
}