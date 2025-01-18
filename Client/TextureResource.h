#pragma once
#include "IResource.h"

class TextureResource : public IResource {
public:
    TextureResource(const std::string& id, const std::string& path);
    ~TextureResource() override;

    // IResource 인터페이스 구현
    bool Load() override;
    void Unload() override;
    bool IsLoaded() const override { return m_isLoaded; }

    // 텍스처 리소스 접근자
    ID3D12Resource* GetTexture() const { return m_texture.Get(); }

private:
    ComPtr<ID3D12Resource> m_texture;
};