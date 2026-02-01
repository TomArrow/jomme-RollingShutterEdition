// dllmain.cpp : Defines the entry point for the DLL application.
#include "VideoMetaHelper.h"
#include <string>
#include <sstream>
#include <Windows.h>
#include <limits>
#include <iomanip>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void pushJSONChar(char c, std::stringstream& ss) {
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

// unsigned char* buf, size_t width, size_t height, size_t totalHeight, size_t stride, size_t multiplier, size_t rgboffsets[4]
extern "C" __declspec(dllexport) char* __cdecl  parseVideoMetaToString(unsigned char* buf, int width, int height, int totalHeight, int stride, int multiplier, size_t* rgboffsets3Array) {
    VideoMetaHelper helper(buf, width, height, totalHeight, stride, multiplier, rgboffsets3Array);

    VideoMeta_t meta =  helper.parseMeta();

    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<float>::max_digits10);
    ss << "{\n";

    // debug
    ss << "\"inValues\":{\n";
    ss << "\"width\":"<<width<<",\n";
    ss << "\"height\":"<<height<<",\n";
    ss << "\"totalHeight\":"<<height<<",\n";
    ss << "\"stride\":"<<stride<<",\n";
    ss << "\"multiplier\":"<<multiplier<<",\n";
    ss << "\"rgbOffsets\":[" << rgboffsets3Array[0] << "," << rgboffsets3Array[1] << "," << rgboffsets3Array[2] << "]\n";
    ss << "},\n";

    ss << "\"version\":[" << (int)meta.VIDT_version[0] << "," << (int)meta.VIDT_version[1] << "," << (int)meta.VIDT_version[2]  << "," << (int)meta.VIDT_version[3] << "],\n";

    // camera
    ss << "\"camera\":{\n";
    ss << "\"pos\":["<< meta.camera.pos[0] << "," << meta.camera.pos[1] << "," << meta.camera.pos[2] << "],\n";
    ss << "\"ang\":["<< meta.camera.ang[0] << "," << meta.camera.ang[1] << "," << meta.camera.ang[2] << "],\n";
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
            pushJSONChar(letter.letter,ss);
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
        if (i == meta.consoleLines.size()-1) {
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

    std::string json = ss.str();
    char* test = new char[json.size()+1];
    for (int i = 0; i < json.size(); i++) {
        test[i] = json[i];
    }
    test[json.size()] = 0;
    return test;
}

extern "C" __declspec(dllexport) char* __cdecl  testString() {
    char* test = new char[5]{"t3st"};
    return test;
}

extern "C" __declspec(dllexport) void __cdecl  freeVideoMetaString(char* metaString) {
    delete[] metaString;
}