#include "pch.h"
#include "ShaderResource.h"
#include "Logger.h"

namespace Resource
{
    bool Resource::ShaderResource::Load()
    {
        if (IsReady()) return true;

        SetState(State::Loading);

        UINT compileFlags = 0;
        IFDEBUG(compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);

        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = D3DCompileFromFile(
            std::wstring(GetPath().begin(), GetPath().end()).c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            m_entryPoint.c_str(),
            GetShaderTarget().c_str(),
            compileFlags,
            0,
            &m_shaderBlob,
            &errorBlob);

        if (FAILED(hr)) {
            SetState(State::Failed);
            std::string errorMsg = errorBlob ?
                static_cast<const char*>(errorBlob->GetBufferPointer()) :
                "Unknown error";
            Logger::Instance().Error("셰이더 컴파일 실패: {} - {}", GetPath(), errorMsg);
            return false;
        }

        SetState(State::Ready);
        SetSize(m_shaderBlob->GetBufferSize());

        Logger::Instance().Info("셰이더 로드 완료: {} ({}bytes)", GetName(), GetSize());
        return true;
    }

    void ShaderResource::Unload()
    {
        if (m_shaderBlob) {
            m_shaderBlob.Reset();
            SetState(State::Unloaded);
            Logger::Instance().Debug("셰이더 언로드: {}", GetName());
        }
    }
}
