#pragma once
#include "Resource.h"
class ShaderResource : public Resource {
public:
    explicit ShaderResource(const std::string& name)
        : Resource(name) {}

    bool Load() override;
    void Unload() override;
    ResourceType GetType() const override { return ResourceType::Shader; }
    size_t GetMemoryUsage() const override;

    ID3DBlob* GetVertexShader() const { return m_vertexShader.Get(); }
    ID3DBlob* GetPixelShader() const { return m_pixelShader.Get(); }

private:
    ComPtr<ID3DBlob> m_vertexShader;
    ComPtr<ID3DBlob> m_pixelShader;
};