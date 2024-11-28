#include "pch.h"
#include "ShaderResource.h"
#include "Logger.h"

bool ShaderResource::Load()
{
    try {
        m_state = ResourceState::Loading;

        UINT compileFlags = 0;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        std::wstring path = std::wstring(m_name.begin(), m_name.end());
        ComPtr<ID3DBlob> errorBlob;

        // 버텍스 셰이더 컴파일
        HRESULT hr = D3DCompileFromFile(
            path.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            &m_vertexShader,
            &errorBlob);

        if (FAILED(hr)) {
            throw std::runtime_error(static_cast<const char*>(
                errorBlob->GetBufferPointer()));
        }

        // 픽셀 셰이더 컴파일
        hr = D3DCompileFromFile(
            path.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            &m_pixelShader,
            &errorBlob);

        if (FAILED(hr)) {
            throw std::runtime_error(static_cast<const char*>(
                errorBlob->GetBufferPointer()));
        }

        m_state = ResourceState::Ready;
        LOG_INFO("Shader loaded: {}", m_name);
        return true;
    }
    catch (const std::exception& e) {
        m_state = ResourceState::Failed;
        m_errorMessage = e.what();
        LOG_ERROR("Failed to load shader {}: {}", m_name, e.what());
        return false;
    }
}

void ShaderResource::Unload()
{
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_state = ResourceState::Unloaded;
}

size_t ShaderResource::GetMemoryUsage() const
{
    size_t size = 0;
    if (m_vertexShader) size += m_vertexShader->GetBufferSize();
    if (m_pixelShader) size += m_pixelShader->GetBufferSize();
    return size;
}
