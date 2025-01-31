#pragma once
#include "IResource.h"
#include "ShaderResource.h"
#include "TextureResource.h"

namespace Resource
{
    class MaterialResource : public IResource {
    public:
        struct TextureSlot {
            std::shared_ptr<TextureResource> texture;
            D3D12_GPU_DESCRIPTOR_HANDLE handle;
        };

        MaterialResource() = default;
        ~MaterialResource() override;

        bool Load(const std::string& path) override;
        void Unload() override;

        // 리소스 설정
        void SetShaders(std::shared_ptr<ShaderResource> vertex,
            std::shared_ptr<ShaderResource> pixel);
        void SetBaseColorTexture(std::shared_ptr<TextureResource> texture);
        void SetNormalTexture(std::shared_ptr<TextureResource> texture);
        void SetMetallicRoughnessTexture(std::shared_ptr<TextureResource> texture);

        // 파이프라인 상태 관리
        void SetPipelineSettings(const D3D12_RASTERIZER_DESC& rasterizer,
            const D3D12_BLEND_DESC& blend,
            const D3D12_DEPTH_STENCIL_DESC& depthStencil);
        ID3D12PipelineState* GetPipelineState();
        
        // Material Constants 관리
        MaterialConstants& GetMaterialConstants() { return m_materialConstants; }
        void SetBaseColor(const XMFLOAT4& color);
		void SetRoughness(float roughness);
		void SetMetallic(float metallic);
		void SetAO(float ao);
		void SetEmissive(const XMFLOAT4& emissive);
        void UpdateMaterialConstants();

        // GPU Descriptor Handles
        const D3D12_GPU_DESCRIPTOR_HANDLE& GetMaterialCBVHandle() const { return m_cbvHandle; }
        const TextureSlot& GetTextureSlot(UINT index) const { return m_textureSlots[index]; }

    private:
        bool CreateConstantBuffer();
        bool CreateConstantBufferView();
        bool CreatePipelineState();

    private:
        std::shared_ptr<ShaderResource> m_vertexShader;
        std::shared_ptr<ShaderResource> m_pixelShader;

		// [slot 1] Base Color
		// [slot 2] Normal
		// [slot 3] Metallic-Roughness
        std::array<TextureSlot, 3> m_textureSlots;

        ComPtr<ID3D12Resource> m_constantBuffer;
        D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle;
        UINT8* m_mappedConstantBufferData = nullptr;

        MaterialConstants m_materialConstants;
        bool m_constantsDirty = false;

        D3D12_RASTERIZER_DESC m_rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_BLEND_DESC m_blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC m_depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        ComPtr<ID3D12PipelineState> m_pipelineState;
    };
}