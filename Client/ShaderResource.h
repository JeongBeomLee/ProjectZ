#pragma once
#include "IResource.h"

class ShaderResource : public IResource {
public:
    enum class ShaderType {
        Vertex,
        Pixel,
        Compute,
        Geometry
    };

    ShaderResource(const std::string& id, const std::string& path, ShaderType type);
    ~ShaderResource() override;

    bool Load() override;
    void Unload() override;
    bool IsLoaded() const override;

    // 컴파일된 셰이더 코드 접근자
    ID3DBlob* GetShaderByteCode() const { return m_shaderBlob.Get(); }
    ShaderType GetShaderType() const { return m_shaderType; }

private:
    std::wstring GetShaderTarget() const;
    std::string GetShaderProfile() const;

private:
    ShaderType m_shaderType;
    ComPtr<ID3DBlob> m_shaderBlob;
};