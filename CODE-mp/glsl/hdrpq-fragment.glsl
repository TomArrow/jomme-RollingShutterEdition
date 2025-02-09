#version 400 compatibility

uniform sampler2D text_in;

void main(void)
{
	// [[ 0.627441372057979  0.329297459521910  0.043351458394495]
	// [ 0.069027617147078  0.919580666887028  0.011361422575401]
	// [ 0.016364235071681  0.088017162471727  0.895564972725983]]
	const mat3 srgbToHDR = mat3(0.627441372057979,  0.329297459521910,  0.043351458394495,0.069027617147078,  0.919580666887028 , 0.011361422575401,0.016364235071681 , 0.088017162471727,  0.895564972725983);
	
	const float m1 = 1305.0f / 8192.0f;
	const float m2 = 2523.0f / 32.0f;
	const float c1 = 107.0f / 128.0f;
	const float c2 = 2413.0f / 128.0f;
	const float c3 = 2392.0f / 128.0f;
	const vec3 m1vec = vec3(m1);
	const vec3 m2vec = vec3(m2);
	const vec3 c1vec = vec3(c1);
	const vec3 c2vec = vec3(c2);
	const vec3 c3vec = vec3(c3);
	const vec3 charMult = vec3(255.0f,255.0f,255.0f);
	const vec3 castAdd = vec3(0.5f,0.5f,0.5f);

	// 1.0f in the source would mean 10,000 nits. 
	// Let's assume 400 nits for a typical gaming monitor (so the target for 1.0f from source buffer)
	// 400/10000 = 0.04f				
	vec3 inputColorTmp = 0.04f*texture2D(text_in, gl_TexCoord[0].st).xyz;
	inputColorTmp *= srgbToHDR;
	inputColorTmp = pow((c1vec + c2vec * pow(inputColorTmp, m1vec)) / (1.0f + c3vec * pow(inputColorTmp, m1vec)), m2vec);
	inputColorTmp *= 255.0f; // Prepare for conversion to byte
	inputColorTmp += 0.5f; // Make it so we don't have to use round() in C++
	inputColorTmp = clamp(inputColorTmp,0.5f,255.5f);
	gl_FragColor = vec4(inputColorTmp,1.0f); 
}