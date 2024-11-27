#pragma once
#include "Resource.h"
class TextureResource : public Resource {
public:
    explicit TextureResource(const std::string& name)
        : Resource(name) {}

    bool Load() override;
    void Unload() override;
    ResourceType GetType() const override { return ResourceType::Texture; }
    size_t GetMemoryUsage() const override;

    ID3D12Resource* GetTexture() const { return m_texture.Get(); }
    const D3D12_RESOURCE_DESC& GetDesc() const { return m_desc; }

private:
    ComPtr<ID3D12Resource> m_texture;
    D3D12_RESOURCE_DESC m_desc;
};
