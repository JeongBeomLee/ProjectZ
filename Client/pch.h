#pragma once

// Windows 헤더
#include <windows.h>
#include <windowsx.h>
#include <wrl.h>

// DirectX 관련 헤더
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include "Util/d3dx12.h"

// STL 헤더
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <format>

// COM 스마트 포인터 사용
using Microsoft::WRL::ComPtr;

// DirectX 네임스페이스
using namespace DirectX;

// 자주 사용하는 상수 정의
constexpr UINT FRAME_BUFFER_COUNT = 2;  // 더블 버퍼링
constexpr UINT MAX_LOADSTRING = 100;

// 추가 라이브러리
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// 에러 체크 매크로
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                               \
{                                                                      \
    HRESULT hr__ = (x);                                               \
    if(FAILED(hr__)) {                                                \
        std::string error_msg = std::format("{}({}) failed with {:x}", \
            #x, __LINE__, static_cast<unsigned int>(hr__));           \
        throw std::runtime_error(error_msg);                          \
    }                                                                 \
}
#endif

// 디버그 빌드용 매크로
#if defined(DEBUG) | defined(_DEBUG)
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif

// 메모리 정렬 헬퍼
template<typename T>
constexpr UINT CalculateConstantBufferByteSize(UINT byteSize) 
{
    return (byteSize + (sizeof(T) - 1)) & ~(sizeof(T) - 1);
}

// Release COM 객체 헬퍼
template<typename T>
void SafeRelease(T& ptr)  
{
    if (ptr != nullptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

// 정점 구조체
struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};