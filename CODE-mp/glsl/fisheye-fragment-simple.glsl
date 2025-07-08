#version 400 compatibility

uniform sampler2D text_in;
varying vec4 vertColor;

void main(void)
{	
	vec2 uvCoords = gl_TexCoord[0].st;
	vec4 color;
	color = texture2D(text_in, uvCoords);

	gl_FragColor = color*vertColor; 

}



