#pragma once

#include <vector>
#include <bitset>
#include "fp16/fp16/fp16.h"

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
	float					pos[3];
	float					vel[3];
	float					ang[3];
} playerMeta_t;


class ConsoleLine_t {
public:
	int									ageMilliseconds;
	std::vector<consoleLetterMeta_t>	letters;
};

class VideoMeta_t {
public:
	// VIDT
	unsigned char				VIDT_version[4] = { 0,0,0,1 };

	struct {
		float					pos[3] = { 0,0,0 };
		float					ang[3] = { 0,0,0 };
		unsigned short			blendFrames = 0;
		float					fov = 0;
		unsigned char			fisheyeMode = 0;
		float					fishEyeNormalBlend = 0;
	} camera;

	playerMeta_t				playerMeta[32];
	std::vector<ConsoleLine_t>	consoleLines;
	std::vector<centerPrintLetterMeta_t>	centerPrint;
};

class VideoMetaHelper {
	const unsigned char	VIDT_marker[4] = {'V','I','D','T'}; // metadata start
	const unsigned char	NWLN_marker[4] = {'N','W','L','N'}; // newline
	const unsigned char	CMRA_marker[4] = {'C','M','R','A'}; // camera info
	const unsigned char PLIN_marker[4] = {'P','L','I','N'}; // playerinfo (light, position, speed, angles)
	const unsigned char CNSL_marker[4] = {'C','N','S','L'}; // console
	const unsigned char CTPR_marker[4] = {'C','T','P','R'}; // centerprint

	bool			_write = false;
	unsigned char*	_buffer = nullptr;
	unsigned char*	_bufferPtr = nullptr;
	size_t			_currentOffset = 0;
	unsigned char	_rgboffsets[3];
	unsigned char	_multiplier;
	size_t			_width = 0, _height = 0;
	size_t			_stride = 0;
	size_t			_bytesPerRow = 0;
	size_t			_totalBytes = 0;
	size_t			_rgbsLeft = 0;
	bool			_needsRGBRearrange = false;
	const float		_oneDividedBy255 = 1.0f / 255.0f;
public:


	// TODO turn the check into some kind of parity that can actually fix shit?
	static void encodenum16fp6(const unsigned short a, unsigned char b[3]);
	static bool decodenum16fp6(unsigned char bA[3], unsigned short* a);
	static void encodenum(const unsigned char a, unsigned char b[3]);
	static bool decodenum(unsigned char bA[3], unsigned char* a);

	// writing
	void rearrangeRGB();
	//bool checkSpaceRGB(size_t count);
	void commitRGB();
	size_t pushByte(const unsigned char b);
	size_t pushShort(const unsigned short s);
	size_t pushRGB(const float* c3);
	size_t pushRGBMult(const float* c3);
	size_t pushRGBA(const float* c4);
	size_t pushRGBAMult(const float* c4);
	size_t pushFloat(const float f);
	size_t pushMarker(const unsigned char b[4]);
	size_t pushNewLine();
	size_t forwardLine();

	// reading
	void getRGB(unsigned char b[3]);
	size_t pullByte(unsigned char* b);
	size_t pullShort(unsigned short* s);
	size_t pullRGB(float* c3);
	size_t pullRGBMult(float* c3);
	size_t pullRGBA(float* c4);
	size_t pullRGBAMult(float* c4);
	size_t pullFloat(float* f);
	size_t pullMarker(unsigned char b[4]);

	// write or read
	VideoMetaHelper(char* buf, size_t width, size_t height, size_t stride, unsigned char multiplier, unsigned char rgboffsets[3], bool write=true) {
		_bufferPtr = _buffer = (unsigned char*)buf;
		_write = write;
		_width = width;
		_stride = stride;
		_height = height;
		_multiplier = multiplier;
		_bytesPerRow = _width * _multiplier;
		_rgboffsets[0] = rgboffsets[0];
		_rgboffsets[1] = rgboffsets[1];
		_rgboffsets[2] = rgboffsets[2];
		_rgbsLeft = _width * _height;
		if (_rgboffsets[0] != 0 || _rgboffsets[1] != 1 || _rgboffsets[2] != 2) {
			_needsRGBRearrange = true;
		}
		_totalBytes = height * stride;
	}
	// use this constructor for automatically finding the start to begin reading. read only.
	VideoMetaHelper(char* buf, size_t width, size_t height, size_t totalHeight, size_t stride, size_t multiplier, size_t rgboffsets[4]) {
		// TODO seek.
		_bufferPtr = _buffer = (unsigned char*)buf;
		_write = false;
		_width = width;
		_stride = stride;
		_height = height;
		_multiplier = multiplier;
		_rgboffsets[0] = rgboffsets[0];
		_rgboffsets[1] = rgboffsets[1];
		_rgboffsets[2] = rgboffsets[2];
		_rgbsLeft = _width * _height;
		if (_rgboffsets[0] != 0 || _rgboffsets[1] != 1 || _rgboffsets[2] != 2) {
			_needsRGBRearrange = true;
		}
		_totalBytes = height * stride;
	}

	size_t writeMeta(VideoMeta_t& meta);
	VideoMeta_t parseMeta();

};