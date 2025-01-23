#pragma once
#include "IResource.h"
namespace Resource
{
    class TextureResource : public IResource {
    public:
        TextureResource() = default;
        ~TextureResource() override;

        bool Load(const std::string& path) override;
        void Unload() override;

        ID3D12Resource* GetTexture() const { return m_texture.Get(); }
		const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDesc() const { return m_srvDesc; }
        const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUSRVHandle() const { return m_srvHandle; }

    private:
        bool CreateTextureFromDDS(const std::wstring& widePath);
        bool CreateShaderResourceView();

    private:
        ComPtr<ID3D12Resource> m_texture;
        ComPtr<ID3D12Resource> m_uploadBuffer;
        D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc = {};
        D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle = {};
    };
}