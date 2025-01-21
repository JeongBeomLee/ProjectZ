#include "pch.h"
#include "ShaderResource.h"
#include "Logger.h"

ShaderResource::ShaderResource(const std::string& id, const std::string& path, ShaderType type)
    : IResource(id, path)
    , m_shaderType(type)
{
}

ShaderResource::~ShaderResource()
{
    Unload();
}

bool ShaderResource::Load()
{
    if (m_isLoaded) return true;

    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errorBlob = nullptr;
    std::wstring wPath(m_path.begin(), m_path.end());

    HRESULT hr = D3DCompileFromFile(
        wPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        GetEntryPoint().c_str(),    // 엔트리 포인트
        GetShaderProfile().c_str(), // 셰이더 타겟
        compileFlags,
        0,
        &m_shaderBlob,
        &errorBlob);

    if (FAILED(hr)) {
        std::string errorMessage = "셰이더 컴파일 실패: ";
        if (errorBlob) {
            errorMessage += static_cast<const char*>(errorBlob->GetBufferPointer());
        }
        Logger::Instance().Error(errorMessage);
        return false;
    }

    m_isLoaded = true;
    Logger::Instance().Info("셰이더 로드 완료: {} (타입: {})", m_path, GetShaderProfile());
    return true;
}

void ShaderResource::Unload()
{
    if (!m_isLoaded) return;

    m_shaderBlob.Reset();
    m_isLoaded = false;
    Logger::Instance().Info("셰이더 언로드: {}", m_path);
}

bool ShaderResource::IsLoaded() const
{
    return m_isLoaded;
}

std::string ShaderResource::GetEntryPoint() const
{
	switch (m_shaderType) {
	case ShaderType::Vertex:
		return "VSMain";
	case ShaderType::Pixel:
		return "PSMain";
	case ShaderType::Compute:
		return "CSMain";
	case ShaderType::Geometry:
		return "GSMain";
	default:
		return "";
	}
}

std::string ShaderResource::GetShaderProfile() const
{
    switch (m_shaderType) {
    case ShaderType::Vertex:
        return "vs_5_0";
    case ShaderType::Pixel:
        return "ps_5_0";
    case ShaderType::Compute:
        return "cs_5_0";
    case ShaderType::Geometry:
        return "gs_5_0";
    default:
        return "";
    }
}