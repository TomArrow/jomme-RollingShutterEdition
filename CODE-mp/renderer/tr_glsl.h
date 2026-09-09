#pragma once
#include "tr_local.h"

#define MAX_SHADER_ERROR_LOG_LENGTH 32768

typedef enum glslShaderType_s {
	GLSLSHAD_PERLIN = (1<<0),
	GLSLSHAD_ZPREPASS = (1<<1),
	GLSLSHAD_MAX = (1<<2),
}glslShaderType_t;

class R_GLSL
{
public:
	GLuint ShaderId(bool perlinFuckery, bool zPrepass) {
		int shaderbits = (perlinFuckery ? GLSLSHAD_PERLIN : 0) | (zPrepass ? GLSLSHAD_ZPREPASS : 0);
		return shaderId[shaderbits];
	};
	GLuint ShaderIdByBits(int shaderbits) {
		if (shaderbits < 0 || shaderbits >= GLSLSHAD_MAX) {
			Com_Error(ERR_FATAL,"ShaderIdByBits: invalid bits %d\n",shaderbits);
			return 0;
		}
		return shaderId[shaderbits];
	};
	bool IsWorking() {
		return isWorking;
	};
	R_GLSL(char* filenameVertexShader, char* filenameTessellationControlShader, char* filenameTessellationEvaluationShader, char* filenameGeometryShader, char* filenameFragmentShader, qboolean noFragment);
	~R_GLSL(){
		for (int i = 0; i < GLSLSHAD_MAX; i++) {
			if (shaderId[i]) {
				qglDeleteProgram(shaderId[i]);
			}
		}
	}
private:
	GLuint shaderId[GLSLSHAD_MAX];
	//GLuint shaderIdPerlinFuckery;
	bool isWorking = false;
	bool hasErrored(GLuint glId,char* filename,bool isProgram); //true if has errored
};

