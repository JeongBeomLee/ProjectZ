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
			// 기본 머티리얼의 상수 값을 복사하여 초기화
			m_materialConstants = m_baseMaterial->GetMaterialConstants();

			if (CreateConstantBuffer() && CreateConstantBufferView()) {
				Logger::Instance().Debug("머티리얼 인스턴스 생성됨");
			}
			else {
				Logger::Instance().Error("머티리얼 인스턴스 생성 실패");
			}
		}
	}

	MaterialInstance::~MaterialInstance()
	{
		if (m_mappedConstantBufferData) {
			m_constantBuffer->Unmap(0, nullptr);
			m_mappedConstantBufferData = nullptr;
		}
	}

	void MaterialInstance::SetBaseColor(const XMFLOAT4& color)
	{
		m_materialConstants.baseColor = color;
		m_constantsDirty = true;
	}

	void MaterialInstance::SetRoughness(float roughness)
	{
		m_materialConstants.materialParams.y = roughness;
		m_constantsDirty = true;
	}

	void MaterialInstance::SetMetallic(float metallic)
	{
		m_materialConstants.materialParams.x = metallic;
		m_constantsDirty = true;
	}

	void MaterialInstance::SetAO(float ao)
	{
		m_materialConstants.materialParams.z = ao;
		m_constantsDirty = true;
	}

	void MaterialInstance::SetEmissive(const XMFLOAT4& emissive)
	{
		m_materialConstants.emissiveColor = emissive;
		m_constantsDirty = true;
	}

	void MaterialInstance::UpdateMaterialConstants()
	{
		if (m_constantsDirty && m_mappedConstantBufferData) {
			memcpy(m_mappedConstantBufferData, &m_materialConstants, sizeof(MaterialConstants));
			m_constantsDirty = false;
		}
	}

	ID3D12PipelineState* MaterialInstance::GetPipelineState() const
	{
		return m_baseMaterial ? m_baseMaterial->GetPipelineState() : nullptr;
	}

	const MaterialResource::TextureSlot& MaterialInstance::GetTextureSlot(UINT index) const
	{
		return m_baseMaterial->GetTextureSlot(index);
	}

	bool MaterialInstance::CreateConstantBuffer()
	{
		const UINT alignedSize = (sizeof(MaterialConstants) + 255) & ~255;

		auto device = Engine::Instance().GetDevice();
		if (!device) return false;

		D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer));

		if (FAILED(hr)) {
			Logger::Instance().Error("인스턴스 상수 버퍼 생성 실패");
			return false;
		}

		// 상수 버퍼 매핑
		CD3DX12_RANGE readRange(0, 0);
		hr = m_constantBuffer->Map(0, &readRange,
			reinterpret_cast<void**>(&m_mappedConstantBufferData));

		if (FAILED(hr)) {
			Logger::Instance().Error("인스턴스 상수 버퍼 매핑 실패");
			return false;
		}

		// 초기 데이터 복사
		memcpy(m_mappedConstantBufferData, &m_materialConstants, sizeof(MaterialConstants));

		return true;
	}

	bool MaterialInstance::CreateConstantBufferView()
	{
		auto device = Engine::Instance().GetDevice();
		auto descHeap = Engine::Instance().GetDescriptorHeap();
		UINT descriptorIndex = Engine::Instance().GetMaterialCbvDescriptorIndex();
		UINT descriptorSize = Engine::Instance().GetDescriptorIncrementSize();

		if (!device || !descHeap) return false;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(
			descHeap->GetCPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(descriptorIndex, descriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(MaterialConstants) + 255) & ~255;

		device->CreateConstantBufferView(&cbvDesc, cbvHandle);

		// GPU 디스크립터 핸들 저장
		m_cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
			descHeap->GetGPUDescriptorHandleForHeapStart(),
			descriptorIndex,
			descriptorSize);

		return true;
	}
}