#pragma once

// Windows ���
#include <windows.h>
#include <windowsx.h>
#include <wrl.h>

// DirectX ���� ���
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include "Util/d3dx12.h"

// STL ���
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

// COM ����Ʈ ������ ���
using Microsoft::WRL::ComPtr;

// DirectX ���ӽ����̽�
using namespace DirectX;

// ���� ����ϴ� ��� ����
constexpr UINT FRAME_BUFFER_COUNT = 2;  // ���� ���۸�
constexpr UINT MAX_LOADSTRING = 100;

// �߰� ���̺귯��
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ���� üũ ��ũ��
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

// ����� ����� ��ũ��
#if defined(DEBUG) | defined(_DEBUG)
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif

// �޸� ���� ����
template<typename T>
constexpr UINT CalculateConstantBufferByteSize(UINT byteSize) 
{
    return (byteSize + (sizeof(T) - 1)) & ~(sizeof(T) - 1);
}

// Release COM ��ü ����
template<typename T>
void SafeRelease(T& ptr)  
{
    if (ptr != nullptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

// ���� ����ü
struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};