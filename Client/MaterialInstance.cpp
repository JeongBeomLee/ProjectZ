#include "pch.h"
#include "MaterialInstance.h"
#include "Engine.h"
#include "Logger.h"

namespace Resource
{
    MaterialInstance::MaterialInstance(std::shared_ptr<MaterialResource> baseMaterial)
        : m_baseMaterial(baseMaterial)
    {
        if (m_baseMaterial) {
            CreateConstantBuffer();
            Logger::Instance().Debug("머티리얼 인스턴스 생성됨: {}", m_baseMaterial->GetPath());
        }
    }

    MaterialInstance::~MaterialInstance()
    {
        if (m_mappedData) {
            m_parameterBuffer->Unmap(0, nullptr);
            m_mappedData = nullptr;
        }
    }

    bool MaterialInstance::SetParameterData(const std::string& name, const void* data)
    {
        if (!m_baseMaterial || !m_mappedData) {
            return false;
        }

        const MaterialParameterInfo* paramInfo = m_baseMaterial->GetParameterInfo(name);
        if (!paramInfo) {
            Logger::Instance().Error("파라미터를 찾을 수 없음: {}", name);
            return false;
        }

        memcpy(m_mappedData + paramInfo->offset, data, paramInfo->size);
        m_parametersDirty = true;
        return true;
    }

    void MaterialInstance::UpdateParameters()
    {
        if (!m_parametersDirty) {
            return;
        }

        // GPU가 현재 프레임의 상수 버퍼를 사용하지 않는다고 가정
        // 더 정교한 동기화가 필요할 수 있음
        m_parametersDirty = false;
    }

    ID3D12PipelineState* MaterialInstance::GetPipelineState() const
    {
        return m_baseMaterial ? m_baseMaterial->GetPipelineState() : nullptr;
    }

    bool MaterialInstance::CreateConstantBuffer()
    {
        if (!m_baseMaterial) {
            Logger::Instance().Error("베이스 머티리얼이 설정되지 않음");
            return false;
        }

        // 베이스 머티리얼로부터 전체 파라미터 크기를 계산
        const size_t totalParamSize = (m_baseMaterial->GetTotalParameterSize() + 255) & ~255;
        if (totalParamSize == 0) {
            Logger::Instance().Debug("파라미터가 없는 머티리얼 인스턴스");
            return true;
        }

        auto device = Engine::Instance().GetDevice();
        if (!device) return false;

        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalParamSize);

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_parameterBuffer));

        if (FAILED(hr)) {
            Logger::Instance().Error("인스턴스 상수 버퍼 생성 실패");
            return false;
        }

        CD3DX12_RANGE readRange(0, 0);
        hr = m_parameterBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedData));
        if (FAILED(hr)) {
            Logger::Instance().Error("인스턴스 상수 버퍼 매핑 실패");
            return false;
        }

        Logger::Instance().Debug("인스턴스 상수 버퍼 생성됨");
        return true;
    }
}