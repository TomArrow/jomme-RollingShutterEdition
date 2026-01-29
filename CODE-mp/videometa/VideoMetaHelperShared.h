#pragma once

#ifndef VIDEOMETAHELPERSHARED_H
#define VIDEOMETAHELPERSHARED_H


typedef struct {
	float		color[4];
	char		letter;
} consoleLetterMeta_t;

typedef struct {
	float		color[4];
	float		bgColor[4];
	char		letter;
} centerPrintLetterMeta_t;

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