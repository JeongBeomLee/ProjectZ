#pragma once
#include "IResource.h"

namespace Resource
{
    enum class ShaderType {
        Vertex,
        Pixel,
        Compute,
        Geometry
    };

    class ShaderResource : public IResource {
    public:
        ShaderResource(ShaderType type);
        ~ShaderResource() override;

        bool Load(const std::string& path) override;
        void Unload() override;

        ID3DBlob* GetShaderBlob() const { return m_shaderBlob.Get(); }
        ShaderType GetShaderType() const { return m_shaderType; }

    private:
        bool CompileShader(const std::wstring& widePath);
		std::string GetShaderEntryPoint() const;
        std::string GetShaderTarget() const;
        std::string GetShaderTypeString() const;

    private:
        ShaderType m_shaderType;
        ComPtr<ID3DBlob> m_shaderBlob;
        ComPtr<ID3DBlob> m_errorBlob;
    };
}