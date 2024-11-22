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
#include "packages/DirectXTK12/include/ResourceUploadBatch.h"
#include "packages/DirectXTK12/include/DDSTextureLoader.h"

// PhysX ���� ���
#include "PhysX/include/PxPhysicsAPI.h"

// STL ���
#include <iostream>
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
#include <thread>
#include <format>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <chrono>

#include <stdexcept>
#include <cassert>
#include <future>
#include <sstream>
#include <functional>
#include <source_location>
#include <type_traits>
#include <iomanip>

// COM ����Ʈ ������ ���
using Microsoft::WRL::ComPtr;

// ���ӽ����̽�
using namespace DirectX;
using namespace physx;

// ���� ����ϴ� ��� ����
constexpr UINT FRAME_BUFFER_COUNT = 2;  // ���� ���۸�
constexpr UINT MAX_LOADSTRING = 100;
constexpr UINT CACHE_LINE_SIZE = 64;

// �߰� ���̺귯��
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#ifdef _DEBUG
    #pragma comment(lib, "PhysX_64.lib")
    #pragma comment(lib, "PhysXCommon_64.lib")
    #pragma comment(lib, "PhysXTask_static_64")
    #pragma comment(lib, "PhysXCooking_64.lib")
    #pragma comment(lib, "PhysXVehicle_static_64")
    #pragma comment(lib, "PhysXFoundation_64.lib")
    #pragma comment(lib, "PhysXVehicle2_static_64")
    #pragma comment(lib, "PhysXPvdSDK_static_64.lib")
    #pragma comment(lib, "PhysXExtensions_static_64.lib")
    #pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")

    #pragma comment(lib, "PVDRuntime_64")
    #pragma comment(lib, "LowLevel_static_64")
    #pragma comment(lib, "SceneQuery_static_64")
    #pragma comment(lib, "LowLevelAABB_static_64")
    #pragma comment(lib, "LowLevelDynamics_static_64")
    #pragma comment(lib, "SimulationController_static_64")
#else
    #pragma comment(lib, "PhysX_64.lib")
    #pragma comment(lib, "PhysXCommon_64.lib")
    #pragma comment(lib, "PhysXTask_static_64")
    #pragma comment(lib, "PhysXCooking_64.lib")
    #pragma comment(lib, "PhysXVehicle_static_64")
    #pragma comment(lib, "PhysXFoundation_64.lib")
    #pragma comment(lib, "PhysXVehicle2_static_64")
    #pragma comment(lib, "PhysXPvdSDK_static_64.lib")
    #pragma comment(lib, "PhysXExtensions_static_64.lib")
    #pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")

    #pragma comment(lib, "PVDRuntime_64")
    #pragma comment(lib, "LowLevel_static_64")
    #pragma comment(lib, "SceneQuery_static_64")
    #pragma comment(lib, "LowLevelAABB_static_64")
    #pragma comment(lib, "LowLevelDynamics_static_64")
    #pragma comment(lib, "SimulationController_static_64")
#endif

// ���� üũ ��ũ��
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                               \
{                                                                      \
    HRESULT hr__ = (x);                                                \
    if (FAILED(hr__)) {                                                \
        std::string error_msg = std::format("{}({}) failed with {:x}", \
            #x, __LINE__, static_cast<unsigned int>(hr__));            \
        MessageBox(nullptr, std::wstring(error_msg.begin(),            \
                                         error_msg.end()).c_str(), 	   \
                                         L"Error", MB_OK);             \
		ExitProcess(EXIT_FAILURE);                                     \
    }                                                                  \
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
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
};

// Object ��� ���� ����ü
struct ObjectConstants
{
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};

// Light ��� ���� ����ü
struct LightConstants {
    XMFLOAT4 lightDirection;  // w�� ������� ����
    XMFLOAT4 lightColor;      // w�� ������ ���
    XMFLOAT4 ambientColor;    // w�� ������ ���
    XMFLOAT4 eyePosition;     // w�� ������� ����
};

// ���� ��ü�� Ÿ���� �����ϱ� ���� ������ Ŭ����
enum class PhysicsObjectType {
    STATIC,
    DYNAMIC,
    KINEMATIC
};