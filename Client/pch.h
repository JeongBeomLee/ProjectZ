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
#include "packages/DirectXTK12/include/ResourceUploadBatch.h"
#include "packages/DirectXTK12/include/DDSTextureLoader.h"

// PhysX 관련 헤더
#include "PhysX/include/PxPhysicsAPI.h"

// STL 헤더
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

// COM 스마트 포인터 사용
using Microsoft::WRL::ComPtr;

// 네임스페이스
using namespace DirectX;
using namespace physx;

// 자주 사용하는 상수 정의
constexpr UINT FRAME_BUFFER_COUNT = 2;  // 더블 버퍼링
constexpr UINT MAX_LOADSTRING = 100;
constexpr UINT CACHE_LINE_SIZE = 64;

// 추가 라이브러리
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

// 에러 체크 매크로
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
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
};

// Object 상수 버퍼 구조체
struct ObjectConstants
{
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};

// Light 상수 버퍼 구조체
struct LightConstants {
    XMFLOAT4 lightDirection;  // w는 사용하지 않음
    XMFLOAT4 lightColor;      // w는 강도로 사용
    XMFLOAT4 ambientColor;    // w는 강도로 사용
    XMFLOAT4 eyePosition;     // w는 사용하지 않음
};

// 물리 객체의 타입을 구분하기 위한 열거형 클래스
enum class PhysicsObjectType {
    STATIC,
    DYNAMIC,
    KINEMATIC
};