#include "pch.h"
#include "TextureResource.h"
#include "Engine.h"
#include "Logger.h"


bool TextureResource::Load()
{
    try {
        m_state = ResourceState::Loading;

        // DDS 텍스처 로딩
        std::wstring path = std::wstring(m_name.begin(), m_name.end());
        ComPtr<ID3D12Resource> textureResource;

        DirectX::ResourceUploadBatch resourceUpload(Engine::GetInstance().GetDevice());
        resourceUpload.Begin();

        ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
            Engine::GetInstance().GetDevice(),
            resourceUpload,
            path.c_str(),
            textureResource.ReleaseAndGetAddressOf()));

        auto uploadResourcesFinished = resourceUpload.End(Engine::GetInstance().GetCommandQueue());
        uploadResourcesFinished.wait();

        m_texture = textureResource;
        m_desc = m_texture->GetDesc();
        m_state = ResourceState::Ready;

        LOG_INFO("Texture loaded: {}", m_name);
        return true;
    }
    catch (const std::exception& e) {
        m_state = ResourceState::Failed;
        m_errorMessage = e.what();
        LOG_ERROR("Failed to load texture {}: {}", m_name, e.what());
        return false;
    }
}

void TextureResource::Unload()
{
    m_texture.Reset();
    m_state = ResourceState::Unloaded;
}

size_t TextureResource::GetMemoryUsage() const
{
    if (!m_texture) return 0;

    const auto& desc = m_texture->GetDesc();
    size_t size = 0;

    for (UINT i = 0; i < desc.MipLevels; ++i) {
        size += (desc.Width >> i) * (desc.Height >> i) * 4;  // 4 bytes per pixel
    }

    return size;
}
