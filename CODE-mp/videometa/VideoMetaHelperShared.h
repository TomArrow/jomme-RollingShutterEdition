

#ifndef VIDEOMETAHELPERSHARED_H
#define VIDEOMETAHELPERSHARED_H

//typedef struct {
//	float		color[4];
//	char		letter;
//} consoleLetterMeta_t;


typedef struct {
	float		color[4];
	float		bgColor[4];
	char		letter;
} centerPrintLetterMeta_t;
#define consoleLetterMeta_t centerPrintLetterMeta_t


typedef struct playerMeta_s {
	float					light[3];
	float					lightDirect[3];
	float					lightDir[3];
	float					pos[3];
	float					headPos[3];
	float					vel[3];
	float					ang[3];
} playerMeta_t;


#endif

#ifdef VIDEOMETAHELPERSHARED_WITHCPP
#ifndef VIDEOMETAHELPERSHARED_H_CPP
#define VIDEOMETAHELPERSHARED_H_CPP
#include <vector>
#include <sstream>
class ConsoleLine_t {
public:
	int									ageMilliseconds;
	std::vector<consoleLetterMeta_t>	letters;

	std::string as_string() {
		std::stringstream ss;
		ss << "age " << ageMilliseconds << ": ";
		for (int i = 0; i < letters.size(); i++) {
			ss << letters[i].letter;
		}
		return ss.str();
	}
};
#endif
#endif
