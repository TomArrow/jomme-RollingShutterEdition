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


// unsigned char* buf, size_t width, size_t height, size_t totalHeight, size_t stride, size_t multiplier, size_t rgboffsets[4]
extern "C" __declspec(dllexport) char* __cdecl  parseVideoMetaToString(unsigned char* buf, int width, int height, int totalHeight, int stride, int multiplier, size_t* rgboffsets3Array) {
    VideoMetaHelper helper(buf, width, height, totalHeight, stride, multiplier, rgboffsets3Array);

    VideoMeta_t meta =  helper.parseMeta();

    std::stringstream ss;

    helper.printMetaToStream(meta,ss);

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