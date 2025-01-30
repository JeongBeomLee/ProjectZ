#pragma once
#include "MaterialResource.h"

namespace Resource
{
    class MaterialInstance {
    public:
        MaterialInstance(std::shared_ptr<MaterialResource> baseMaterial);
        ~MaterialInstance();

        // 머티리얼 파라미터 업데이트
        MaterialConstants& GetMaterialConstants() { return m_materialConstants; }
        void SetBaseColor(const XMFLOAT4& color);
        void SetRoughness(float roughness);
        void SetMetallic(float metallic);
        void SetAO(float ao);
        void SetEmissive(const XMFLOAT4& emissive);
        void UpdateMaterialConstants();

        // 렌더링에 필요한 리소스 접근
        ID3D12PipelineState* GetPipelineState() const;
        const D3D12_GPU_DESCRIPTOR_HANDLE& GetMaterialCBVHandle() const { return m_cbvHandle; }
        const MaterialResource::TextureSlot& GetTextureSlot(UINT index) const;
        MaterialResource* GetBaseMaterial() const { return m_baseMaterial.get(); }

    private:
        bool CreateConstantBuffer();
        bool CreateConstantBufferView();

    private:
        std::shared_ptr<MaterialResource> m_baseMaterial;
        ComPtr<ID3D12Resource> m_constantBuffer;
        D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle;
        UINT8* m_mappedConstantBufferData = nullptr;
        MaterialConstants m_materialConstants;
        bool m_constantsDirty = true;
    };
}