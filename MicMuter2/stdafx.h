#pragma once
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1
#include <new>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include <objbase.h>
#pragma warning(push)
#pragma warning(disable : 4201)
#include <mmdeviceapi.h>
#include <audiopolicy.h>       
#include <Mmsystem.h>  
#include <functiondiscoverykeys.h>
#include <EndpointVolume.h>
#pragma warning(pop)

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}
