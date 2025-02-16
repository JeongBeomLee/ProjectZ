#include "pch.h"
#include "ShaderResource.h"
#include "Logger.h"

namespace Resource
{
    ShaderResource::ShaderResource(ShaderType type)
        : m_shaderType(type)
    {
    }

    ShaderResource::~ShaderResource()
    {
        Unload();
    }

    bool ShaderResource::Load(const std::string& path)
    {
        m_path = path;
        SetState(ResourceState::Loading);

        std::wstring widePath(path.begin(), path.end());

        try {
            if (!CompileShader(widePath)) {
                if (m_errorBlob) {
                    SetError(static_cast<const char*>(m_errorBlob->GetBufferPointer()));
                }
                return false;
            }

            SetState(ResourceState::Loaded);
            Logger::Instance().Info("셰이더 로드 성공: {} ({})",
                path, GetShaderTypeString());
            return true;
        }
        catch (const std::exception& e) {
            SetError(std::string("셰이더 로드 실패: ") + e.what());
            return false;
        }
    }

    void ShaderResource::Unload()
    {
        m_shaderBlob.Reset();
        m_errorBlob.Reset();
        SetState(ResourceState::Unloaded);
        Logger::Instance().Debug("셰이더 언로드: {} ({})",
            m_path, GetShaderTypeString());
    }

    bool ShaderResource::CompileShader(const std::wstring& widePath)
    {
        UINT compileFlags = 0;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = D3DCompileFromFile(
            widePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            GetShaderEntryPoint().c_str(), // 엔트리 포인트
            GetShaderTarget().c_str(),     // 셰이더 타겟
            compileFlags,
            0,
            &m_shaderBlob,
            &m_errorBlob);

        return SUCCEEDED(hr);
    }

    std::string ShaderResource::GetShaderEntryPoint() const
    {
		switch (m_shaderType) {
		case ShaderType::Vertex:   return "VSMain";
		case ShaderType::Pixel:    return "PSMain";
		case ShaderType::Compute:  return "CSMain";
		case ShaderType::Geometry: return "GSMain";
        default:
            throw std::runtime_error("Unknown shader type");
        }
    }

    std::string ShaderResource::GetShaderTarget() const
    {
        switch (m_shaderType) {
        case ShaderType::Vertex:   return "vs_5_0";
        case ShaderType::Pixel:    return "ps_5_0";
        case ShaderType::Compute:  return "cs_5_0";
        case ShaderType::Geometry: return "gs_5_0";
        default:
            throw std::runtime_error("Unknown shader type");
        }
    }

    std::string ShaderResource::GetShaderTypeString() const
    {
        switch (m_shaderType) {
        case ShaderType::Vertex:   return "Vertex Shader";
        case ShaderType::Pixel:    return "Pixel Shader";
        case ShaderType::Compute:  return "Compute Shader";
        case ShaderType::Geometry: return "Geometry Shader";
        default:
            return "Unknown Shader";
        }
    }
}