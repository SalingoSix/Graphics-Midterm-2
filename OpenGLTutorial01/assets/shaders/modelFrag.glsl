#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_static;
uniform float staticTime;
uniform float randOffsetX;
uniform float randOffsetY;

void main()
{    
	float realStaticTime = staticTime;
	if (realStaticTime > 0.65)
	{
		realStaticTime = 0.65;
	}
	//The TV screen model is turned the wrong way, so we'll have to just flip the texture back upright
	//TV screen is also kinda lopsided, with some of it hidden from sight, so move the texture to the left a bit
	vec2 adjustedTexCoords;
	adjustedTexCoords.x = TexCoords.x + 0.16;
	adjustedTexCoords.y = 1.0 - TexCoords.y;
	
	//Random offset every frame to make the static lively and dynamic
	vec2 randomTexCoords;
	randomTexCoords.x = TexCoords.x + randOffsetX;
	randomTexCoords.y = TexCoords.y + randOffsetY;
	
	FragColor = mix(texture(texture_diffuse1, adjustedTexCoords), texture(texture_static, randomTexCoords), realStaticTime);
}