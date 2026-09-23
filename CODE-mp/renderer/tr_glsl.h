#pragma once
#include "tr_local.h"
#include <atomic>

#define MAX_SHADER_ERROR_LOG_LENGTH 32768

extern int64_t boundShaderUniqueId;

typedef enum glslShaderType_s {
	GLSLSHAD_PERLIN = (1<<0),
	GLSLSHAD_ZPREPASS = (1<<1),
	GLSLSHAD_MAX = (1<<2),
}glslShaderType_t;

class R_GLSL
{
public:
	inline static void UseProgram(GLint index, int64_t uniqueShaderId) {
		boundShaderUniqueId = uniqueShaderId;
		qglUseProgram(index);
	}
	inline void UseThisProgram(int shaderBits) {
		UseProgram(ShaderIdByBits(shaderBits),getUniqueId(shaderBits));
	}
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
	inline static std::atomic<int64_t> uniqueIdCounter{ 0 };
	//int64_t uniqueId = uniqueIdCounter.fetch_add(1,std::memory_order_relaxed);
	int64_t uniqueShaderId[GLSLSHAD_MAX] = { 0 };
public:
	inline int64_t getUniqueId(int shaderbits) {
		if (shaderbits < 0 || shaderbits >= GLSLSHAD_MAX) {
			Com_Error(ERR_FATAL, "getUniqueId: invalid bits %d\n", shaderbits);
			return 0;
		}
		return uniqueShaderId[shaderbits];
	}
};

typedef enum uniformType_s {
	UNIFORM_TYPE_INVALID,
	UNIFORM_TYPE_1F,
	UNIFORM_TYPE_1I,
	UNIFORM_TYPE_1UI,
	UNIFORM_TYPE_3FV,
	UNIFORM_TYPE_4FV,
	UNIFORM_TYPE_MATRIX4FV,
} uniformType_t;
typedef struct uniformHistoryEntry_s {
	int64_t			lastUniqueShaderId;
	qboolean		everSet;
	GLboolean		transpose;
	uniformType_t	type;
	size_t			lastArraySize;
	union {
		GLint		intValue;
		GLuint		uIntValue;
		GLfloat		floatValue;
		GLfloat		floatArray[16];
	} val;
} uniformHistoryEntry_t;

#define UNIFORM_HISTORY_ENTRIES 10
class R_GLSL_Uniform {
	uniformHistoryEntry_t	history[UNIFORM_HISTORY_ENTRIES] = { {0} };
	size_t					historyCount = 0;
	uniformHistoryEntry_t*	_getHistoryEntry(int64_t uniqueShaderId, uniformType_t type) {
		uniformHistoryEntry_t* entry = NULL;
		size_t lowestEntry = historyCount <= UNIFORM_HISTORY_ENTRIES ? 0 : historyCount - UNIFORM_HISTORY_ENTRIES;
		for (size_t histEntry = historyCount - 1; histEntry >= lowestEntry; histEntry--) {
			entry = &history[histEntry % UNIFORM_HISTORY_ENTRIES];
			if (entry->lastUniqueShaderId == uniqueShaderId) {
				assert(entry->type == type);
				if (entry->type != type) {
					// type changed. shouldnt rly happen. invalidate history data.
					memset(entry, 0, sizeof(*entry));
					entry->lastUniqueShaderId = uniqueShaderId;
					entry->type = type;
				}
				return entry;
			}
			if (histEntry) break; // meh i made this very awkward with my choice of size_t
		}
		// couldn't find a match. create a new one (oldest is ditched, oh well)
		entry = &history[historyCount % UNIFORM_HISTORY_ENTRIES];
		memset(entry, 0, sizeof(*entry));
		entry->lastUniqueShaderId = uniqueShaderId;
		entry->type = type;
		historyCount++;
		return entry;
	}

public:
	GLint					location=-2;

	inline	void	getUniformLocation(GLuint programId, const GLchar* name) {
		location = qglGetUniformLocation(programId,name);
	}
	inline	void	set1f(GLfloat val) {
		uniformHistoryEntry_t* hist = _getHistoryEntry(boundShaderUniqueId, UNIFORM_TYPE_1F);
		if (hist->everSet && hist->val.floatValue == val) {
			return;
		}
		qglUniform1f(location,val);
		hist->val.floatValue = val;
		hist->everSet = qtrue;
	}
	inline	void	set1i(GLint val) {
		uniformHistoryEntry_t* hist = _getHistoryEntry(boundShaderUniqueId, UNIFORM_TYPE_1I);
		if (hist->everSet && hist->val.intValue == val) {
			return;
		}
		qglUniform1i(location,val);
		hist->val.intValue = val;
		hist->everSet = qtrue;
	}
	inline	void	set1ui(GLuint val) {
		uniformHistoryEntry_t* hist = _getHistoryEntry(boundShaderUniqueId, UNIFORM_TYPE_1UI);
		if (hist->everSet && hist->val.uIntValue == val) {
			return;
		}
		qglUniform1ui(location,val);
		hist->val.uIntValue = val;
		hist->everSet = qtrue;
	}
	inline	void	set3fv(GLsizei count, const GLfloat* value) {
		uniformHistoryEntry_t* hist = _getHistoryEntry(boundShaderUniqueId, UNIFORM_TYPE_3FV);
		size_t byteCount = count * 3 * sizeof(float);
		assert(byteCount <= sizeof(hist->val.floatArray)); // not critical, but let's make sure it's the case if we can for now.
		if (hist->everSet && byteCount <= sizeof(hist->val.floatArray) && hist->lastArraySize == byteCount && !memcmp(hist->val.floatArray,value, byteCount)) {
			return;
		}
		qglUniform3fv(location, count, value);
		if (byteCount > sizeof(hist->val.floatArray)) {
			// we can't save the history of this.
			hist->everSet = qfalse;
			return;
		}
		memcpy(hist->val.floatArray, value, byteCount);
		hist->lastArraySize = byteCount;
		hist->everSet = qtrue;
	}
	inline	void	set4fv(GLsizei count, const GLfloat* value) {
		uniformHistoryEntry_t* hist = _getHistoryEntry(boundShaderUniqueId, UNIFORM_TYPE_4FV);
		size_t byteCount = count * 4 * sizeof(float);
		assert(byteCount <= sizeof(hist->val.floatArray)); // not critical, but let's make sure it's the case if we can for now.
		if (hist->everSet && byteCount <= sizeof(hist->val.floatArray) && hist->lastArraySize == byteCount && !memcmp(hist->val.floatArray,value, byteCount)) {
			return;
		}
		qglUniform4fv(location, count, value);
		if (byteCount > sizeof(hist->val.floatArray)) {
			// we can't save the history of this.
			hist->everSet = qfalse;
			return;
		}
		memcpy(hist->val.floatArray, value, byteCount);
		hist->lastArraySize = byteCount;
		hist->everSet = qtrue;
	}
	inline	void	setMatrix4fv(GLsizei count, GLboolean transpose, const GLfloat* value) {
		uniformHistoryEntry_t* hist = _getHistoryEntry(boundShaderUniqueId, UNIFORM_TYPE_MATRIX4FV);
		size_t byteCount = count * 16 * sizeof(float);
		assert(byteCount <= sizeof(hist->val.floatArray)); // not critical, but let's make sure it's the case if we can for now.
		if (hist->everSet && byteCount <= sizeof(hist->val.floatArray) && hist->transpose == transpose && hist->lastArraySize == byteCount && !memcmp(hist->val.floatArray,value, byteCount)) {
			return;
		}
		qglUniformMatrix4fv(location, count, transpose, value);
		if (byteCount > sizeof(hist->val.floatArray)) {
			// we can't save the history of this.
			hist->everSet = qfalse;
			return;
		}
		memcpy(hist->val.floatArray, value, byteCount);
		hist->lastArraySize = byteCount;
		hist->transpose = transpose;
		hist->everSet = qtrue;
	}
};

