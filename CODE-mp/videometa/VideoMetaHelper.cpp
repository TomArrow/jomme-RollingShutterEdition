
#include "VideoMetaHelper.h"
#include <algorithm>
#include <iomanip>

#define BITCOMPRESS(num,phase) (((((((num)<<phase)&37449) * 0x700007U) & 0x3000c00cU) * 0x1010101U) >> 24)
#define BITEXPAND(num,phase) (((((((num)&63ULL)*0x41041041041ULL)&0x804010080201ULL)*0x100010001ULL)>>32ULL)>>phase)

#define BITCOMPRESS32(num,phase) ((((((((num)<<phase)&0x82082082ULL) >> 1ULL) * 0x3f00000003fULL) & 0x4000000080201804ULL) * 0x101010101010101ULL) >> 56ULL)
#define BITEXPAND32(num,phase) (((((((num)&63ULL)*0x401004000200802ULL)&0x8008008002002002ULL)*0x100000001ULL)>>32ULL)>>phase)


#define CHECKSPACE(a) ((_stride*_height-_currentOffset)>=(a))
#define FORWARD(a) _currentOffset += (a); _bufferPtr += (a);
//#define CHECKRGB(a) ((_stride*_height-_currentOffset)>=(_multiplier*(a)))
#define CHECKRGB(a) (_rgbsLeft >= (a))
#define FORWARDRGB(a) _currentOffset += (a)*_multiplier; _bufferPtr += (a)*_multiplier; _rgbsLeft -= a;


#define COMPAREINDEX(v,a,i,n) ((a) < (v)[(i)] || ((a) == (v)[(i)] && (n)))
#define VERSIONATLEAST(v,a,b,c,d) (COMPAREINDEX((v),(a), 0, COMPAREINDEX((v),(b), 1, COMPAREINDEX((v),(c), 2, ((d) <= (v)[3])))))


//bool VideoMetaHelper::checkSpaceRGB(size_t count) {
//}

bool VideoMetaHelper::pushPlayerInfo(playerMeta_t& playerMeta) {
	bool success = true;
	success = success && pushRGBMultOver1(playerMeta.light);
	success = success && pushRGBMultOver1(playerMeta.lightDirect);
	success = success && pushFloat(playerMeta.lightDir[0]);
	success = success && pushFloat(playerMeta.lightDir[1]);
	success = success && pushFloat(playerMeta.lightDir[2]);
	success = success && pushFloat(playerMeta.pos[0], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.pos[1], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.pos[2], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.headPos[0], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.headPos[1], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.headPos[2], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.ang[0], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.ang[1], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.ang[2], _versionHasFullFloat);
	success = success && pushFloat(playerMeta.vel[0]);
	success = success && pushFloat(playerMeta.vel[1]);
	success = success && pushFloat(playerMeta.vel[2]);
	return success;
}


void VideoMetaHelper::pushJSONChar(char c, std::ostream& ss) {
	switch (c) {
	case '\0':
		ss << "\\0";
		break;
	case '\n':
		ss << "\\n";
		break;
	case '\r':
		ss << "\\r";
		break;
	case '\t':
		ss << "\\t";
		break;
	case '\b':
		ss << "\\b";
		break;
	case '\f':
		ss << "\\f";
		break;
	case '\\':
		ss << "\\\\";
		break;
	case '"':
		ss << "\\\"";
		break;
	default:
		if (c >= '\x00' && c <= '\x1f') {
			ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << (int)c << std::dec << std::setfill(' ');
		}
		else {
			ss << c;
		}
		break;
	}
}

void VideoMetaHelper::printMetaToStream(VideoMeta_t& meta, std::ostream& ss)
{



	ss << std::setprecision(std::numeric_limits<float>::max_digits10);
	ss << "{\n";

	// debug
	ss << "\"inValues\":{\n";
	ss << "\"width\":" << _width << ",\n";
	ss << "\"height\":" << _height << ",\n";
	ss << "\"totalHeight\":" << _height << ",\n";
	ss << "\"stride\":" << _stride << ",\n";
	ss << "\"multiplier\":" << (int)_multiplier << ",\n";
	ss << "\"rgbOffsets\":[" << (int)_rgboffsets[0] << "," << (int)_rgboffsets[1] << "," << (int)_rgboffsets[2] << "]\n";
	ss << "},\n";

	ss << "\"version\":[" << (int)meta.VIDT_version[0] << "," << (int)meta.VIDT_version[1] << "," << (int)meta.VIDT_version[2] << "," << (int)meta.VIDT_version[3] << "],\n";

	// camera
	ss << "\"camera\":{\n";
	ss << "\"pos\":[" << meta.camera.pos[0] << "," << meta.camera.pos[1] << "," << meta.camera.pos[2] << "],\n";
	ss << "\"ang\":[" << meta.camera.ang[0] << "," << meta.camera.ang[1] << "," << meta.camera.ang[2] << "],\n";
	ss << "\"viewAxes\":[\n";
	ss << "[" << meta.camera.viewAxis[0][0] << "," << meta.camera.viewAxis[0][1] << "," << meta.camera.viewAxis[0][2] << "],\n";
	ss << "[" << meta.camera.viewAxis[1][0] << "," << meta.camera.viewAxis[1][1] << "," << meta.camera.viewAxis[1][2] << "],\n";
	ss << "[" << meta.camera.viewAxis[2][0] << "," << meta.camera.viewAxis[2][1] << "," << meta.camera.viewAxis[2][2] << "]\n";
	ss << "],\n";
	ss << "\"blendFrames\":" << meta.camera.blendFrames << ",\n";
	ss << "\"fov\":" << meta.camera.fov << ",\n";
	ss << "\"fisheyeMode\":" << (int)meta.camera.fisheyeMode << ",\n";
	ss << "\"fishEyeNormalBlend\":" << meta.camera.fishEyeNormalBlend << "\n";
	ss << "},\n";


	// players
	ss << "\"psClientNum\":" << (int)meta.psClientNum << ",\n";
	ss << "\"playerMeta\":[\n";
	for (int i = -1; i < 32; i++) {
		int c = i == -1 ? (meta.psClientNum >= 0 && meta.psClientNum < 32 ? meta.psClientNum : 0) : i;
		playerMeta_t& pM = meta.playerMeta[c];
		ss << "{\n";
		ss << "\"light\":[" << pM.light[0] << "," << pM.light[1] << "," << pM.light[2] << "],\n";
		ss << "\"lightDirect\":[" << pM.lightDirect[0] << "," << pM.lightDirect[1] << "," << pM.lightDirect[2] << "],\n";
		ss << "\"lightDir\":[" << pM.lightDir[0] << "," << pM.lightDir[1] << "," << pM.lightDir[2] << "],\n";
		ss << "\"pos\":[" << pM.pos[0] << "," << pM.pos[1] << "," << pM.pos[2] << "],\n";
		ss << "\"headPos\":[" << pM.headPos[0] << "," << pM.headPos[1] << "," << pM.headPos[2] << "],\n";
		ss << "\"vel\":[" << pM.vel[0] << "," << pM.vel[1] << "," << pM.vel[2] << "],\n";
		ss << "\"ang\":[" << pM.ang[0] << "," << pM.ang[1] << "," << pM.ang[2] << "]\n";
		if (i == 31) {
			ss << "}\n";
		}
		else {
			ss << "},\n";
		}
	}
	ss << "],\n";

	// console lines
	ss << "\"consoleLines\":[\n";
	for (int i = 0; i < meta.consoleLines.size(); i++) {
		ConsoleLine_t& line = meta.consoleLines[i];
		ss << "{\n";
		ss << "\"ageMilliseconds\":" << line.ageMilliseconds << ",\n";
		ss << "\"plaintext\":\"";
		for (int j = 0; j < line.letters.size(); j++) {
			consoleLetterMeta_t& letter = line.letters[j];
			pushJSONChar(letter.letter, ss);
		}
		ss << "\",\n";
		ss << "\"letters\":[\n";
		for (int j = 0; j < line.letters.size(); j++) {
			consoleLetterMeta_t& letter = line.letters[j];
			ss << "{";
			ss << "\"letter\":\"";
			pushJSONChar(letter.letter, ss);
			ss << "\",";
			ss << "\"color\":[" << letter.color[0] << "," << letter.color[1] << "," << letter.color[2] << "," << letter.color[3] << "]";
			if (j == line.letters.size() - 1) {
				ss << "}\n";
			}
			else {
				ss << "},\n";
			}
		}
		ss << "]\n";
		if (i == meta.consoleLines.size() - 1) {
			ss << "}\n";
		}
		else {
			ss << "},\n";
		}
	}
	ss << "],\n";

	// centerprint
	ss << "\"centerPrint\":{\n";
	ss << "\"plaintext\":\"";
	for (int j = 0; j < meta.centerPrint.size(); j++) {
		centerPrintLetterMeta_t& letter = meta.centerPrint[j];
		pushJSONChar(letter.letter, ss);
	}
	ss << "\",\n";
	ss << "\"letters\":[\n";
	for (int j = 0; j < meta.centerPrint.size(); j++) {
		centerPrintLetterMeta_t& letter = meta.centerPrint[j];
		ss << "{";
		ss << "\"letter\":\"";
		pushJSONChar(letter.letter, ss);
		ss << "\",";
		ss << "\"color\":[" << letter.color[0] << "," << letter.color[1] << "," << letter.color[2] << "," << letter.color[3] << "],";
		ss << "\"bgColor\":[" << letter.bgColor[0] << "," << letter.bgColor[1] << "," << letter.bgColor[2] << "," << letter.bgColor[3] << "]";
		if (j == meta.centerPrint.size() - 1) {
			ss << "}\n";
		}
		else {
			ss << "},\n";
		}
	}
	ss << "]\n";
	ss << "}\n";


	// end
	ss << "}\n";
}
size_t VideoMetaHelper::writeMeta(VideoMeta_t& meta){
	_versionHasFullFloat = VERSIONATLEAST(meta.VIDT_version, 0, 0, 0, 2);
	bool success = true;
	success = success && pushMarker(VIDT_marker);
	success = success && pushMarker(meta.VIDT_version);

	success = success && pushMarker(CMRA_marker);
	success = success && pushFloat(meta.camera.pos[0], _versionHasFullFloat);
	success = success && pushFloat(meta.camera.pos[1], _versionHasFullFloat);
	success = success && pushFloat(meta.camera.pos[2], _versionHasFullFloat);
	success = success && pushFloat(meta.camera.ang[0], _versionHasFullFloat);
	success = success && pushFloat(meta.camera.ang[1], _versionHasFullFloat);
	success = success && pushFloat(meta.camera.ang[2], _versionHasFullFloat);
	for (int a = 0; a < 3; a++) {
		success = success && pushFloat(meta.camera.viewAxis[a][0], _versionHasFullFloat);
		success = success && pushFloat(meta.camera.viewAxis[a][1], _versionHasFullFloat);
		success = success && pushFloat(meta.camera.viewAxis[a][2], _versionHasFullFloat);
	}
	success = success && pushShort(meta.camera.blendFrames);
	success = success && pushFloat(meta.camera.fov, _versionHasFullFloat);
	success = success && pushByte(meta.camera.fisheyeMode);
	success = success && pushFloat(meta.camera.fishEyeNormalBlend, _versionHasFullFloat);

	success = success && pushMarker(PLIN_marker);
	success = success && pushByte(meta.psClientNum);
	success = success && pushPlayerInfo(meta.playerMeta[meta.psClientNum]); // push the main ps one first for easier access in video editing. its gonna be doubled, yes
	for (int i = 0; i < 32; i++) {
		success = success && pushPlayerInfo(meta.playerMeta[i]);
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


bool VideoMetaHelper::pullPlayerInfo(playerMeta_t& playerMeta) {
	bool success = true;
	success = success && pullRGBMult(playerMeta.light);
	success = success && pullRGBMult(playerMeta.lightDirect);
	success = success && pullFloat(&playerMeta.lightDir[0]);
	success = success && pullFloat(&playerMeta.lightDir[1]);
	success = success && pullFloat(&playerMeta.lightDir[2]);
	success = success && pullFloat(&playerMeta.pos[0], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.pos[1], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.pos[2], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.headPos[0], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.headPos[1], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.headPos[2], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.ang[0], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.ang[1], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.ang[2], _versionHasFullFloat);
	success = success && pullFloat(&playerMeta.vel[0]);
	success = success && pullFloat(&playerMeta.vel[1]);
	success = success && pullFloat(&playerMeta.vel[2]);
	return success;
}

VideoMeta_t VideoMetaHelper::parseMeta(){
	VideoMeta_t meta;
	unsigned char readBuf[4];
	while (pullMarker(readBuf)) {
		if (!memcmp(readBuf, VIDT_marker,4)) {
			if (!pullMarker(meta.VIDT_version)) {
				return meta;
			}
			_versionHasFullFloat = VERSIONATLEAST(meta.VIDT_version, 0, 0, 0, 2);
		}
		else if (!memcmp(readBuf, CMRA_marker,4)) {
			bool success = pullFloat(&meta.camera.pos[0], _versionHasFullFloat);
			success = success && pullFloat(&meta.camera.pos[1], _versionHasFullFloat);
			success = success && pullFloat(&meta.camera.pos[2], _versionHasFullFloat);
			success = success && pullFloat(&meta.camera.ang[0], _versionHasFullFloat);
			success = success && pullFloat(&meta.camera.ang[1], _versionHasFullFloat);
			success = success && pullFloat(&meta.camera.ang[2], _versionHasFullFloat);
			for (int a = 0; a < 3; a++) {
				success = success && pullFloat(&meta.camera.viewAxis[a][0], _versionHasFullFloat);
				success = success && pullFloat(&meta.camera.viewAxis[a][1], _versionHasFullFloat);
				success = success && pullFloat(&meta.camera.viewAxis[a][2], _versionHasFullFloat);
			}
			success = success && pullShort(&meta.camera.blendFrames);
			success = success && pullFloat(&meta.camera.fov, _versionHasFullFloat);
			success = success && pullByte(&meta.camera.fisheyeMode);
			success = success && pullFloat(&meta.camera.fishEyeNormalBlend, _versionHasFullFloat);
			if (!success) {
				return meta;
			}
		}
		else if (!memcmp(readBuf, PLIN_marker,4)) {

			bool success = true;
			success = success && pullByte(&meta.psClientNum); // one is double. cuz its the main playerstate one. just put it in a random one who cares
			success = success && pullPlayerInfo(meta.playerMeta[meta.psClientNum >= 0 && meta.psClientNum < 32 ? meta.psClientNum : 0]); // one is double. cuz its the main playerstate one. just put it in a random one who cares
			for (int i = 0; i < 32; i++) {
				success = success && pullPlayerInfo(meta.playerMeta[i]);
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



const float m1 = 1305.0f / 8192.0f;
const float m1inv = 8192.0f / 1305.0f;
const float m2 = 2523.0f / 32.0f;
const float m2inv = 32.0f / 2523.0f;
const float c1 = 107.0f / 128.0f;
const float c2 = 2413.0f / 128.0f;
const float c3 = 2392.0f / 128.0f;

void VideoMetaHelper::srgbLinearToHDRPQ(const float in[3], float out[3]) {
	out[0] = in[0] * 0.627441372057979 + in[1] * 0.329297459521910 + in[2] * 0.043351458394495;
	out[1] = in[0] * 0.069027617147078 + in[1] * 0.919580666887028 + in[2] * 0.011361422575401;
	out[2] = in[0] * 0.016364235071681 + in[1] * 0.088017162471727 + in[2] * 0.895564972725983;

	// 1.0f in the source would mean 10,000 nits. 
	// Let's assume 400 nits for a typical gaming monitor (so the target for 1.0f from source buffer)
	// 400/10000 = 0.04f	
	out[0] = 0.04f * out[0];
	out[1] = 0.04f * out[1];
	out[2] = 0.04f * out[2];

	out[0] = powf((c1 + c2 * powf(out[0], m1)) / (1.0f + c3 * powf(out[0], m1)), m2);
	out[1] = powf((c1 + c2 * powf(out[1], m1)) / (1.0f + c3 * powf(out[1], m1)), m2);
	out[2] = powf((c1 + c2 * powf(out[2], m1)) / (1.0f + c3 * powf(out[2], m1)), m2);
}
void VideoMetaHelper::hdrPQtoSRGBLinear(const float in[3], float out[3]) {

	// the check for 0 is important, otherwise we end up with a negative exponent for some reason...
	// TODO technically i should check against the smallest value that produces NaN but lazy. And we only input scaled 8 bit numbers anyway
	out[0] = in[0] == 0 ? 0 : powf((powf(in[0], m2inv) - c1) / (c2 - c3 * powf(in[0], m2inv)), m1inv);
	out[1] = in[1] == 0 ? 0 : powf((powf(in[1], m2inv) - c1) / (c2 - c3 * powf(in[1], m2inv)), m1inv);
	out[2] = in[2] == 0 ? 0 : powf((powf(in[2], m2inv) - c1) / (c2 - c3 * powf(in[2], m2inv)), m1inv);

	// 1.0f in the source would mean 10,000 nits. 
	// Let's assume 400 nits for a typical gaming monitor (so the target for 1.0f from source buffer)
	// 400/10000 = 0.04f	
	// and inverse: 10000/400 = 25.0f
	out[0] = 25.0f * out[0];
	out[1] = 25.0f * out[1];
	out[2] = 25.0f * out[2]; 

	out[0] = out[0] * 1.6603176191042 + out[1] * -0.58757266606618 + out[2] * -0.072916573137668;
	out[1] = out[0] * -0.12440670211719 + out[1] * 1.1328007408693 + out[2] * -0.0083489374502385;
	out[2] = out[0] * -0.018111363657382 + out[1] * -0.100596531096745 + out[2] * 1.1187664817637;
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

void VideoMetaHelper::encodenum32fp(const unsigned int a, unsigned char b[6])
{
	b[0] = (BITCOMPRESS32(a, 0) & 248) + 3 + (a & 1);
	b[1] = (BITCOMPRESS32(a, 2)) + 3 + (a & 1);
	b[2] = (BITCOMPRESS32(a, 4)) + 3 + (a & 1);

	b[3] = (BITCOMPRESS32(a, 1) & 248) + 3 + ((a & 2) >> 1);
	b[4] = (BITCOMPRESS32(a, 3)) + 3 + ((a & 2) >> 1);
	b[5] = (BITCOMPRESS32(a, 5)) + 3 + ((a & 2) >> 1);
}

bool VideoMetaHelper::decodenum32fp(unsigned char bA[6], unsigned int* a)
{
	unsigned int b[6] = {
		(unsigned int)((bA[0] >> 3) << 1),(unsigned int)((bA[1] >> 3) << 1),(unsigned int)((bA[2] >> 3) << 1),
		(unsigned int)((bA[3] >> 3) << 1),(unsigned int)((bA[4] >> 3) << 1),(unsigned int)((bA[5] >> 3) << 1)
	};
	*a = BITEXPAND32(b[0], 0) | BITEXPAND32(b[1], 2) | BITEXPAND32(b[2], 4)
		| BITEXPAND32(b[3], 1) | BITEXPAND32(b[4], 3) | BITEXPAND32(b[5], 5)
		| (1 ^ (1 & ((bA[0] & bA[1]) | (bA[0] & bA[2]) | (bA[1] & bA[2]))))
		| (((1 ^ (1 & ((bA[3] & bA[4]) | (bA[3] & bA[5]) | (bA[4] & bA[5]))))) << 1);
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
// TODO use sRGB for colors. wait. or just hdr?
size_t VideoMetaHelper::pushRGB(const float* c3in) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	float c3[3];
	srgbLinearToHDRPQ(c3in, c3);
	_bufferPtr[0] = std::clamp((int)(c3[0] * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[1] = std::clamp((int)(c3[1] * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[2] = std::clamp((int)(c3[2] * 255.0f + 0.5f), (int)0, (int)255);
	commitRGB();
	return 1;
}
size_t VideoMetaHelper::pushRGBMult(const float* c3in) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	float c3[3];
	srgbLinearToHDRPQ(c3in, c3);
	float mult = std::max(std::max(c3[0], c3[1]), c3[2]);
	float multInv = mult;
	_bufferPtr[0] = std::clamp((int)(c3[0] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[1] = std::clamp((int)(c3[1] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[2] = std::clamp((int)(c3[2] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(mult), _bufferPtr);
	commitRGB();
	return 2;
}
// only applies the multiplier if the original value is over 1. so we caan more easily read sub-1 values
size_t VideoMetaHelper::pushRGBMultOver1(const float* c3in) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	float c3[3];
	srgbLinearToHDRPQ(c3in, c3);
	float mult = std::max(1.0f,std::max(std::max(c3[0], c3[1]), c3[2]));
	float multInv = mult;
	_bufferPtr[0] = std::clamp((int)(c3[0] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[1] = std::clamp((int)(c3[1] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[2] = std::clamp((int)(c3[2] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(mult), _bufferPtr);
	commitRGB();
	return 2;
}
size_t VideoMetaHelper::pushRGBA(const float* c4in) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	float c4[4];
	srgbLinearToHDRPQ(c4in, c4);
	c4[3] = c4in[3];
	_bufferPtr[0] = std::clamp((int)(c4[0] * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[1] = std::clamp((int)(c4[1] * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[2] = std::clamp((int)(c4[2] * 255.0f + 0.5f), (int)0, (int)255);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(c4[3]), _bufferPtr);
	commitRGB();
	return 2;
}
size_t VideoMetaHelper::pushRGBAMult(const float* c4in) {
	if (!CHECKRGB(3)) {
		return 0;
	}
	float c4[4];
	srgbLinearToHDRPQ(c4in, c4);
	c4[3] = c4in[3];
	float mult = std::max(std::max(c4[0], c4[1]), c4[2]);
	if (!mult) {
		mult = 1.0f;
	}
	float multInv = 1.0f/mult;
	_bufferPtr[0] = std::clamp((int)(c4[0] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[1] = std::clamp((int)(c4[1] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	_bufferPtr[2] = std::clamp((int)(c4[2] * multInv * 255.0f + 0.5f), (int)0, (int)255);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(mult), _bufferPtr);
	commitRGB();
	encodenum16fp6(fp16_ieee_from_fp32_value(c4[3]), _bufferPtr);
	commitRGB();
	return 3;
}
size_t VideoMetaHelper::pushFloat(const float f, bool fp32) {
	if (fp32) {
		return pushFloat32(f);
	}
	if (!CHECKRGB(1)) {
		return 0;
	}
	encodenum16fp6(fp16_ieee_from_fp32_value(f), _bufferPtr);
	commitRGB();
	return 1;
}
size_t VideoMetaHelper::pushFloat32(const float f) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	unsigned char tmp[6];
	encodenum32fp(*(unsigned int*)&f, tmp);
	_bufferPtr[0] = tmp[0];
	_bufferPtr[1] = tmp[1];
	_bufferPtr[2] = tmp[2];
	commitRGB();
	_bufferPtr[0] = tmp[3];
	_bufferPtr[1] = tmp[4];
	_bufferPtr[2] = tmp[5];
	commitRGB();
	return 2;
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
size_t VideoMetaHelper::pullRGB(float* c3out) {
	if (!CHECKRGB(1)) {
		return 0;
	}
	float c3[3];
	unsigned char rgb[3];
	getRGB(rgb);
	c3[0] = (float)rgb[0] * _oneDividedBy255;
	c3[1] = (float)rgb[1] * _oneDividedBy255;
	c3[2] = (float)rgb[2] * _oneDividedBy255;
	hdrPQtoSRGBLinear(c3, c3out);
	return 1;
}
size_t VideoMetaHelper::pullRGBMult(float* c3out) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	float c3[3];
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
	hdrPQtoSRGBLinear(c3, c3out);
	return 2;
}
size_t VideoMetaHelper::pullRGBA(float* c4out) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	float c4[4];
	unsigned char rgb[3];
	getRGB(rgb);
	c4[0] = (float)rgb[0] * _oneDividedBy255;
	c4[1] = (float)rgb[1] * _oneDividedBy255;
	c4[2] = (float)rgb[2] * _oneDividedBy255;
	getRGB(rgb);
	unsigned short us;
	decodenum16fp6(rgb, &us);
	float alpha = fp16_ieee_to_fp32_value(us);
	c4out[3] = alpha;
	hdrPQtoSRGBLinear(c4, c4out);
	return 2;
}
size_t VideoMetaHelper::pullRGBAMult(float* c4out) {
	if (!CHECKRGB(3)) {
		return 0;
	}
	float c4[4];
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
	c4out[3] = alpha;
	hdrPQtoSRGBLinear(c4, c4out);
	return 3;
}
size_t VideoMetaHelper::pullFloat(float* f, bool fp32) {
	if (fp32) {
		return pullFloat32(f);
	}
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
size_t VideoMetaHelper::pullFloat32(float* f) {
	if (!CHECKRGB(2)) {
		return 0;
	}
	unsigned char rgb[6];
	getRGB(rgb);
	getRGB(rgb+3);
	unsigned int ui;
	decodenum32fp(rgb, &ui);
	*f = *(float*)&ui;
	return 2;
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

