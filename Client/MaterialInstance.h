#pragma once
#include "MaterialResource.h"

namespace Resource
{
    class MaterialInstance {
    public:
        MaterialInstance(std::shared_ptr<MaterialResource> baseMaterial);
        ~MaterialInstance();

        // 파라미터 설정 및 업데이트
        bool SetParameterData(const std::string& name, const void* data);
        void UpdateParameters();

        // 렌더링에 필요한 리소스 접근자
        ID3D12PipelineState* GetPipelineState() const;
        ID3D12Resource* GetConstantBuffer() const { return m_parameterBuffer.Get(); }
        MaterialResource* GetBaseMaterial() const { return m_baseMaterial.get(); }

    private:
        bool CreateConstantBuffer();

    private:
        std::shared_ptr<MaterialResource> m_baseMaterial;
        ComPtr<ID3D12Resource> m_parameterBuffer;
        UINT8* m_mappedData = nullptr;
        bool m_parametersDirty = false;
    };
}