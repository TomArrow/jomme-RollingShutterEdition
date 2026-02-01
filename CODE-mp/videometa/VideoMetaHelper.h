#pragma once

#ifndef VIDEOMETAHELPER_H
#define VIDEOMETAHELPER_H


#include <vector>
#include <bitset>
#include "fp16/fp16/fp16.h"
#define VIDEOMETAHELPERSHARED_WITHCPP
#include "VideoMetaHelperShared.h"





class VideoMeta_t {
public:
	// VIDT
	unsigned char				VIDT_version[4] = { 0,0,0,1 };

	struct {
		float					pos[3] = { 0,0,0 };
		float					ang[3] = { 0,0,0 };
		float					viewAxis[3][3] = { 0 };
		unsigned short			blendFrames = 0;
		float					fov = 0;
		unsigned char			fisheyeMode = 0;
		float					fishEyeNormalBlend = 0;
	} camera;

	unsigned char				psClientNum=0;
	playerMeta_t				playerMeta[32] = { 0 };
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
	
	// Versioning
	bool			_versionHasFullFloat = false;
public:

	static void srgbLinearToHDRPQ(const float in[3], float out[3]);
	static void hdrPQtoSRGBLinear(const float in[3], float out[3]);

	// TODO turn the check into some kind of parity that can actually fix shit?
	static void encodenum16fp6(const unsigned short a, unsigned char b[3]);
	static bool decodenum16fp6(unsigned char bA[3], unsigned short* a);
	static void encodenum32fp(const unsigned int a, unsigned char b[6]);
	static bool decodenum32fp(unsigned char bA[6], unsigned int* a);
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
	size_t pushRGBMultOver1(const float* c3);
	size_t pushRGBA(const float* c4);
	size_t pushRGBAMult(const float* c4);
	size_t pushFloat(const float f, bool fp32=false);
	size_t pushFloat32(const float f);
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
	size_t pullFloat(float* f, bool fp32 = false);
	size_t pullFloat32(float* f);
	size_t pullMarker(unsigned char b[4]);

	// write or read
	VideoMetaHelper(unsigned char* buf, size_t width, size_t height, size_t stride, unsigned char multiplier, size_t rgboffsets[3], bool write=true) {
		_bufferPtr = _buffer = buf;
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
		if (_multiplier < 3)
		{
			// we interpret the grey/2-color stuff as rgb, fuck it. what else can we do? 2-byte might still get fkd by rgbrearrange. do 2-byte formats even exist?
			_bytesPerRow = _width * _multiplier / 3 * 3; // make it align to triplets
			_rgbsLeft = _width * _multiplier / 3 * _height; // bytecount divided by 3 (rounded down cuz trailing ones gonna be useless due to stride possible > _bytesPerRow), multiplied with count of lines
			_multiplier = 3;
		}
		if (_rgboffsets[0] != 0 || _rgboffsets[1] != 1 || _rgboffsets[2] != 2) {
			_needsRGBRearrange = true;
		}
		_totalBytes = height * stride;
	}
	// use this constructor for automatically finding the start to begin reading. read only.
	VideoMetaHelper(unsigned char* buf, size_t width, size_t height, size_t totalHeight, size_t stride, size_t multiplier, size_t rgboffsets[3]) {
		// TODO seek.
		_bufferPtr = _buffer = buf;
		_write = false;
		_width = width;
		_stride = stride;
		_height = height;
		_multiplier = multiplier;
		_bytesPerRow = _width * _multiplier;
		_rgboffsets[0] = rgboffsets[0];
		_rgboffsets[1] = rgboffsets[1];
		_rgboffsets[2] = rgboffsets[2];
		_rgbsLeft = _width * _height;
		if (_multiplier < 3)
		{
			// we interpret the grey/2-color stuff as rgb, fuck it. what else can we do? 2-byte might still get fkd by rgbrearrange. do 2-byte formats even exist?
			_bytesPerRow = _width * _multiplier / 3 * 3; // make it align to triplets
			_rgbsLeft = _width * _multiplier / 3 * _height; // bytecount divided by 3 (rounded down cuz trailing ones gonna be useless due to stride possible > _bytesPerRow), multiplied with count of lines
			_multiplier = 3;
		}
		if (_rgboffsets[0] != 0 || _rgboffsets[1] != 1 || _rgboffsets[2] != 2) {
			_needsRGBRearrange = true;
		}
		_totalBytes = height * stride;
	}

	size_t writeMeta(VideoMeta_t& meta);
	VideoMeta_t parseMeta();

	bool pullPlayerInfo(playerMeta_t& playerMeta);
	bool pushPlayerInfo(playerMeta_t& playerMeta);

};

#endif