#pragma once
#include <string>
#include <Windows.h>

inline char* WCharToChar(const wchar_t* str)
{
    int strLength = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    char* buffer = new char[strLength];
    WideCharToMultiByte(CP_ACP, 0, str, -1, buffer, strLength, NULL, NULL);
    return buffer;
}

inline wchar_t* CharToWChar(const char* str)
{
    int strLength = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    wchar_t* buffer = new wchar_t[strLength];
    MultiByteToWideChar(CP_ACP, 0, str, -1, buffer, strLength);
    return buffer;
}