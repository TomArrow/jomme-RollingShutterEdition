
#include "VideoMetaHelper.h"
#include <algorithm>

#define BITCOMPRESS(num,phase) (((((((num)<<phase)&37449) * 0x700007U) & 0x3000c00cU) * 0x1010101U) >> 24)
#define BITEXPAND(num,phase) (((((((num)&63ULL)*0x41041041041ULL)&0x804010080201ULL)*0x100010001ULL)>>32ULL)>>phase)
#define CHECKSPACE(a) ((_stride*_height-_currentOffset)>=(a))
#define FORWARD(a) _currentOffset += (a); _bufferPtr += (a);
//#define CHECKRGB(a) ((_stride*_height-_currentOffset)>=(_multiplier*(a)))
#define CHECKRGB(a) (_rgbsLeft >= (a))
#define FORWARDRGB(a) _currentOffset += (a)*_multiplier; _bufferPtr += (a)*_multiplier; _rgbsLeft -= a;


//bool VideoMetaHelper::checkSpaceRGB(size_t count) {
//}

size_t VideoMetaHelper::writeMeta(VideoMeta_t& meta){
	bool success = true;
	success = success && pushMarker(VIDT_marker);
	success = success && pushMarker(meta.VIDT_version);

	success = success && pushMarker(CMRA_marker);
	success = success && pushFloat(meta.camera.pos[0]);
	success = success && pushFloat(meta.camera.pos[1]);
	success = success && pushFloat(meta.camera.pos[2]);
	success = success && pushFloat(meta.camera.ang[0]);
	success = success && pushFloat(meta.camera.ang[1]);
	success = success && pushFloat(meta.camera.ang[2]);
	for (int a = 0; a < 3; a++) {
		success = success && pushFloat(meta.camera.viewAxis[a][0]);
		success = success && pushFloat(meta.camera.viewAxis[a][1]);
		success = success && pushFloat(meta.camera.viewAxis[a][2]);
	}
	success = success && pushShort(meta.camera.blendFrames);
	success = success && pushFloat(meta.camera.fov);
	success = success && pushByte(meta.camera.fisheyeMode);
	success = success && pushFloat(meta.camera.fishEyeNormalBlend);

	success = success && pushMarker(PLIN_marker);
	for (int i = 0; i < 32; i++) {
		success = success && pushFloat(meta.playerMeta[i].light[0]);
		success = success && pushFloat(meta.playerMeta[i].light[1]);
		success = success && pushFloat(meta.playerMeta[i].light[2]);
		success = success && pushFloat(meta.playerMeta[i].pos[0]);
		success = success && pushFloat(meta.playerMeta[i].pos[1]);
		success = success && pushFloat(meta.playerMeta[i].pos[2]);
		success = success && pushFloat(meta.playerMeta[i].ang[0]);
		success = success && pushFloat(meta.playerMeta[i].ang[1]);
		success = success && pushFloat(meta.playerMeta[i].ang[2]);
		success = success && pushFloat(meta.playerMeta[i].vel[0]);
		success = success && pushFloat(meta.playerMeta[i].vel[1]);
		success = success && pushFloat(meta.playerMeta[i].vel[2]);
	}

	if (meta.consoleLines.size()) {
		success = success && pushNewLine();
		success = success && pushMarker(CNSL_marker);
		for (auto it = meta.consoleLines.begin(); it != meta.consoleLines.end(); it++) {
			size_t letterCount = std::clamp(it->letters.size(),(size_t)0, (size_t)255);
			if (!letterCount) continue;
			success = success && pushByte(letterCount);
			success = success && pushShort(std::clamp(it->ageMilliseconds,(int)0,(int)UINT16_MAX));
			for (int i = 0; i < letterCount; i++) {
				success = success && pushRGBA(it->letters[i].color);
				success = success && pushByte(it->letters[i].letter);
			}
		}
		success = success && pushByte(0);
	}

	if (meta.centerPrint.size()) {
		success = success && pushNewLine();
		success = success && pushMarker(CTPR_marker);
		size_t letterCountTotal = meta.centerPrint.size();
		size_t index = 0;
		while (letterCountTotal > 0) {
			size_t letterCount = std::clamp(letterCountTotal, (size_t)0, (size_t)255);
			letterCountTotal -= letterCount;
			success = success && pushByte(letterCount);
			for (int i = 0; i < letterCount; i++, index++) {
				success = success && pushRGBA(meta.centerPrint[index].color);
				success = success && pushRGBA(meta.centerPrint[index].bgColor);
				success = success && pushByte(meta.centerPrint[index].letter);
			}
		}
		success = success && pushByte(0);
	}
	return success ? 1 : 0;
}

VideoMeta_t VideoMetaHelper::parseMeta(){
	VideoMeta_t meta;
	unsigned char readBuf[4];
	while (pullMarker(readBuf)) {
		if (!memcmp(readBuf, VIDT_marker,4)) {
			if (!pullMarker(meta.VIDT_version)) {
				return meta;
			}
		}
		else if (!memcmp(readBuf, CMRA_marker,4)) {
			bool success = pullFloat(&meta.camera.pos[0]);
			success = success && pullFloat(&meta.camera.pos[1]);
			success = success && pullFloat(&meta.camera.pos[2]);
			success = success && pullFloat(&meta.camera.ang[0]);
			success = success && pullFloat(&meta.camera.ang[1]);
			success = success && pullFloat(&meta.camera.ang[2]);
			for (int a = 0; a < 3; a++) {
				success = success && pullFloat(&meta.camera.viewAxis[a][0]);
				success = success && pullFloat(&meta.camera.viewAxis[a][1]);
				success = success && pullFloat(&meta.camera.viewAxis[a][2]);
			}
			success = success && pullShort(&meta.camera.blendFrames);
			success = success && pullFloat(&meta.camera.fov);
			success = success && pullByte(&meta.camera.fisheyeMode);
			success = success && pullFloat(&meta.camera.fishEyeNormalBlend);
			if (!success) {
				return meta;
			}
		}
		else if (!memcmp(readBuf, PLIN_marker,4)) {

			bool success = true;
			for (int i = 0; i < 32; i++) {
				success = success && pullFloat(&meta.playerMeta[i].light[0]);
				success = success && pullFloat(&meta.playerMeta[i].light[1]);
				success = success && pullFloat(&meta.playerMeta[i].light[2]);
				success = success && pullFloat(&meta.playerMeta[i].pos[0]);
				success = success && pullFloat(&meta.playerMeta[i].pos[1]);
				success = success && pullFloat(&meta.playerMeta[i].pos[2]);
				success = success && pullFloat(&meta.playerMeta[i].ang[0]);
				success = success && pullFloat(&meta.playerMeta[i].ang[1]);
				success = success && pullFloat(&meta.playerMeta[i].ang[2]);
				success = success && pullFloat(&meta.playerMeta[i].vel[0]);
				success = success && pullFloat(&meta.playerMeta[i].vel[1]);
				success = success && pullFloat(&meta.playerMeta[i].vel[2]);
			}
			if (!success) {
				return meta;
			}
		}
		else if (!memcmp(readBuf, NWLN_marker,4)) {
			if (!forwardLine()) {
				return meta;
			}
		}
		else if (!memcmp(readBuf, CNSL_marker,4)) {

			bool success = true;
			unsigned char charCount = 0;
			while (pullByte(&charCount) && charCount) {
				ConsoleLine_t newLine;
				uint16_t age = 0;
				success = success && pullShort(&age);
				newLine.ageMilliseconds = age;
				for (int i = 0; i < charCount && success; i++) {
					consoleLetterMeta_t newLetter;
					success = success && pullRGBA(newLetter.color);
					success = success && pullByte((unsigned char*)&newLetter.letter);
					newLine.letters.push_back(std::move(newLetter));
				}
				meta.consoleLines.push_back(std::move(newLine));
			}
			if (!success) {
				return meta;
			}
		}
		else if (!memcmp(readBuf, CTPR_marker,4)) {

			bool success = true;
			unsigned char charCount = 0;
			while (pullByte(&charCount) && charCount) {
				for (int i = 0; i < charCount && success; i++) {
					centerPrintLetterMeta_t newLetter;
					success = success && pullRGBA(newLetter.color);
					success = success && pullRGBA(newLetter.bgColor);
					success = success && pullByte((unsigned char*)&newLetter.letter);
					meta.centerPrint.push_back(std::move(newLetter));
				}
			}
			if (!success) {
				return meta;
			}
		}
		else {
			return meta;
		}
	}
	return meta;


}


void VideoMetaHelper::commitRGB() {
	rearrangeRGB();
	FORWARDRGB(1);

	// see if we are now past the last pixel of the row
	size_t lineOffset = _currentOffset % _stride;
	if (lineOffset >= _bytesPerRow) {
		size_t diff = _stride - lineOffset;
		FORWARD(diff);
	}
}

size_t VideoMetaHelper::pushNewLine() {
	if (pushMarker(NWLN_marker)) {
		return 4+forwardLine();
	}
	return 0;
}

size_t VideoMetaHelper::forwardLine() {
	size_t lineOffset = _currentOffset % _stride;
	size_t diff = _stride - lineOffset;
	if (CHECKSPACE(diff)) {
		FORWARD(diff);
		return diff;
	}
	else {
		return 0;
	}
}

// TODO turn the check into some kind of parity that can actually fix shit?
void VideoMetaHelper::encodenum16fp6(const unsigned short a, unsigned char b[3])
{
	// 0: sign and highest 2 of exponent + lowest 3 of fraction
	// 1: lowest 3 of exponent + 3-4 of fraction + check
	// 2: highest 5 of fraction + check
	b[0] = (BITCOMPRESS(a, 0) & 248) + 3 + (a & 1);
	b[1] = (BITCOMPRESS(a, 1)) + 3 + (a & 1);
	b[2] = (BITCOMPRESS(a, 2)) + 3 + (a & 1);
}

bool VideoMetaHelper::decodenum16fp6(unsigned char bA[3], unsigned short* a)
{
	unsigned short b[3] = { (unsigned short)((bA[0] >> 3) << 1),(unsigned short)((bA[1] >> 3) << 1),(unsigned short)((bA[2] >> 3) << 1) };
	//*a = ((b[0] & 224) << 16) | ((b[1] & 224) << 13) | ((b[2] & 248) << 2) | (b[1] & 24) | ((b[0] & 28) >> 2);
	*a = BITEXPAND(b[0], 0) | BITEXPAND(b[1], 1) | BITEXPAND(b[2], 2) | (1 ^ (1 & ((bA[0] & bA[1]) | (bA[0] & bA[2]) | (bA[1] & bA[2]))));
	return true;
}

void VideoMetaHelper::encodenum(const unsigned char a, unsigned char b[3])
{

	b[0] = ((a & 7) << 5) + 15;
	b[1] = (((a >> 3) & 7) << 5) + 15;
	b[2] = ((((a >> 5) & 6) | (std::bitset<8>(a).count() & 1)) << 5) + 15;
}

bool VideoMetaHelper::decodenum(unsigned char bA[3], unsigned char* a)
{
	unsigned char b[3] = { (unsigned char)(bA[0] >> 5),(unsigned char)(bA[1] >> 5),(unsigned char)(bA[2] >> 5) };
	*a = (b[0] & 7) | ((b[1] & 7) << 3) | ((b[2] & 6) << 5);
	return (std::bitset<8>(*a).count() & 1) == (b[2] & 1); // checksum check. this won't reliably catch errors in one number. but if it fails anywhere in a bunch of numbers, it indicates something is wrong.
}

void VideoMetaHelper::rearrangeRGB(){
	if (_needsRGBRearrange) {
		unsigned char rearr[4];
		memcpy(rearr, _bufferPtr, _multiplier);
		_bufferPtr[_rgboffsets[0]] = rearr[0];
		_bufferPtr[_rgboffsets[1]] = rearr[1];
		_bufferPtr[_rgboffsets[2]] = rearr[2];
	}
}
size_t VideoMetaHelper::pushByte(const unsigned char b) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	encodenum(b,_bufferPtr);
	commitRGB();
	return 1;
}
size_t VideoMetaHelper::pushShort(const unsigned short s) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	encodenum16fp6(s, _bufferPtr);
	commitRGB();
	return 1;
}
size_t VideoMetaHelper::pushRGB(const float* c3) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	_bufferPtr[0] = std::clamp((unsigned char)(c3[0] * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[1] = std::clamp((unsigned char)(c3[1] * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[2] = std::clamp((unsigned char)(c3[2] * 255.0f), (unsigned char)0, (unsigned char)255);
	commitRGB();
	return 1;
}
size_t VideoMetaHelper::pushRGBMult(const float* c3) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	float mult = std::max(std::max(c3[0], c3[1]), c3[2]);
	float multInv = mult;
	_bufferPtr[0] = std::clamp((unsigned char)(c3[0] * multInv * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[1] = std::clamp((unsigned char)(c3[1] * multInv * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[2] = std::clamp((unsigned char)(c3[2] * multInv * 255.0f), (unsigned char)0, (unsigned char)255);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(mult), _bufferPtr);
	commitRGB();
	return 2;
}
size_t VideoMetaHelper::pushRGBA(const float* c4) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	_bufferPtr[0] = std::clamp((unsigned char)(c4[0] * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[1] = std::clamp((unsigned char)(c4[1] * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[2] = std::clamp((unsigned char)(c4[2] * 255.0f), (unsigned char)0, (unsigned char)255);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(c4[3]), _bufferPtr);
	commitRGB();
	return 2;
}
size_t VideoMetaHelper::pushRGBAMult(const float* c4) {
	if (!CHECKRGB(3)) {
		return 0;
	}
	float mult = std::max(std::max(c4[0], c4[1]), c4[2]);
	if (!mult) {
		mult = 1.0f;
	}
	float multInv = 1.0f/mult;
	_bufferPtr[0] = std::clamp((unsigned char)(c4[0] * multInv * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[1] = std::clamp((unsigned char)(c4[1] * multInv * 255.0f), (unsigned char)0, (unsigned char)255);
	_bufferPtr[2] = std::clamp((unsigned char)(c4[2] * multInv * 255.0f), (unsigned char)0, (unsigned char)255);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(mult), _bufferPtr);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(c4[3]), _bufferPtr);
	commitRGB();
	return 3;
}
size_t VideoMetaHelper::pushFloat(const float f) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	encodenum16fp6(fp16_ieee_from_fp32_value(f), _bufferPtr);
	commitRGB();
	return 1;
}

size_t VideoMetaHelper::pushMarker(const unsigned char b[4]) {
	if (!CHECKRGB(4)) {
		return 0;
	}
	encodenum(b[0], _bufferPtr);
	commitRGB();
	encodenum(b[1], _bufferPtr);
	commitRGB();
	encodenum(b[2], _bufferPtr);
	commitRGB();
	encodenum(b[3], _bufferPtr);
	commitRGB();
	return 4;
}



void VideoMetaHelper::getRGB(unsigned char b[3]) {

	if (_needsRGBRearrange) {
		b[0] = _bufferPtr[_rgboffsets[0]];
		b[1] = _bufferPtr[_rgboffsets[1]];
		b[2] = _bufferPtr[_rgboffsets[2]];
	}
	else {
		b[0] = _bufferPtr[0];
		b[1] = _bufferPtr[1];
		b[2] = _bufferPtr[2];
	}
	FORWARDRGB(1);

	// see if we are now past the last pixel of the row
	size_t lineOffset = _currentOffset % _stride;
	if (lineOffset >= _bytesPerRow) {
		size_t diff = _stride - lineOffset;
		FORWARD(diff);
	}
}
size_t VideoMetaHelper::pullByte(unsigned char* b) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	decodenum(rgb, b);
	return 1;
}
size_t VideoMetaHelper::pullShort(unsigned short* s) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	decodenum16fp6(rgb,s);
	return 1;
}
size_t VideoMetaHelper::pullRGB(float* c3) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	c3[0] = (float)rgb[0] * _oneDividedBy255;
	c3[1] = (float)rgb[1] * _oneDividedBy255;
	c3[2] = (float)rgb[2] * _oneDividedBy255;
	return 1;
}
size_t VideoMetaHelper::pullRGBMult(float* c3) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	c3[0] = (float)rgb[0] * _oneDividedBy255;
	c3[1] = (float)rgb[1] * _oneDividedBy255;
	c3[2] = (float)rgb[2] * _oneDividedBy255;
	getRGB(rgb);
	unsigned short us;
	decodenum16fp6(rgb, &us);
	float multiplier = fp16_ieee_to_fp32_value(us);
	c3[0] *= multiplier;
	c3[1] *= multiplier;
	c3[2] *= multiplier;
	return 2;
}
size_t VideoMetaHelper::pullRGBA(float* c4) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	c4[0] = (float)rgb[0] * _oneDividedBy255;
	c4[1] = (float)rgb[1] * _oneDividedBy255;
	c4[2] = (float)rgb[2] * _oneDividedBy255;
	getRGB(rgb);
	unsigned short us;
	decodenum16fp6(rgb, &us);
	float alpha = fp16_ieee_to_fp32_value(us);
	c4[3] = alpha;
	return 2;
}
size_t VideoMetaHelper::pullRGBAMult(float* c4) {
	if (!CHECKRGB(3)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	c4[0] = (float)rgb[0] * _oneDividedBy255;
	c4[1] = (float)rgb[1] * _oneDividedBy255;
	c4[2] = (float)rgb[2] * _oneDividedBy255;

	getRGB(rgb);
	unsigned short us;
	decodenum16fp6(rgb, &us);
	float multiplier = fp16_ieee_to_fp32_value(us);
	c4[0] *= multiplier;
	c4[1] *= multiplier;
	c4[2] *= multiplier;

	getRGB(rgb);
	decodenum16fp6(rgb, &us);
	float alpha = fp16_ieee_to_fp32_value(us);
	c4[3] = alpha;
	return 3;
}
size_t VideoMetaHelper::pullFloat(float* f) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	unsigned short us;
	decodenum16fp6(rgb, &us);
	*f = fp16_ieee_to_fp32_value(us);
	return 1;
}

size_t VideoMetaHelper::pullMarker(unsigned char b[4]) {
	if (!CHECKRGB(4)) {
		return 0;
	}
	unsigned char rgb[3];
	getRGB(rgb);
	decodenum(rgb, &b[0]);
	getRGB(rgb);
	decodenum(rgb, &b[1]);
	getRGB(rgb);
	decodenum(rgb, &b[2]);
	getRGB(rgb);
	decodenum(rgb, &b[3]);
	return 4;
}

