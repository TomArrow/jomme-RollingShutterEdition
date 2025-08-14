#version 400 compatibility

#define TEXTURE_COUNT 14

uniform sampler2D text_in;
varying vec4 vertColor;

#define RENDERFLAG_SIMPLELIGHTING 1
#define RENDERFLAG_NOLIGHTING 2 // skyboxes and such
uniform int alphaFuncUniform; 
uniform float alphaFuncValueUniform;
uniform int renderFlagsUniform;

varying vec2 my_TexCoord[TEXTURE_COUNT];

/* AlphaFunction */
#define ALPHA_NEVER                          0x0200
#define ALPHA_LESS                           0x0201
#define ALPHA_EQUAL                          0x0202
#define ALPHA_LEQUAL                         0x0203
#define ALPHA_GREATER                        0x0204
#define ALPHA_NOTEQUAL                       0x0205
#define ALPHA_GEQUAL                         0x0206
#define ALPHA_ALWAYS                         0x0207

void main(void)
{	
	vec2 uvCoords = my_TexCoord[0].st;
	vec4 color;
	color = texture2D(text_in, uvCoords);
	//if(color.w <= 0.1f){
	//	discard;
	//}

	float effectiveAlpha = color.w*vertColor.w;

	if(alphaFuncUniform > 0){
		if(
		alphaFuncUniform == ALPHA_GREATER && effectiveAlpha <= alphaFuncValueUniform
		|| alphaFuncUniform == ALPHA_LESS && effectiveAlpha >= alphaFuncValueUniform
		|| alphaFuncUniform == ALPHA_GEQUAL && effectiveAlpha < alphaFuncValueUniform
		){
			discard; // ok? why do light calc for shit that isnt even visible
		}
	}
	gl_FragColor = color; 
	gl_FragColor.xyz *= vertColor.xyz; 
	//gl_FragColor.xyz = vec3(gl_FragCoord.z+0.5f);
}



