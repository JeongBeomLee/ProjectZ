#include "pch.h"
#include "Engine.h"
#include "MaterialResource.h"
#include "ResourceManager.h"
#include "Logger.h"

namespace Resource
{
	MaterialResource::~MaterialResource()
	{
		Unload();
	}

	bool MaterialResource::Load(const std::string& path)
	{
		m_path = path;
		SetState(ResourceState::Loading);

		try {
			// 기본 상수 버퍼 생성
			if (!CreateConstantBuffer() || !CreateConstantBufferView()) {
				SetError("상수 버퍼 생성 실패");
				return false;
			}

			SetState(ResourceState::Loaded);
			Logger::Instance().Info("머티리얼 로드 성공: {}", path);
			return true;
		}
		catch (const std::exception& e) {
			SetError(std::string("머티리얼 로드 실패: ") + e.what());
			return false;
		}
	}

	void MaterialResource::Unload()
	{
		if (m_mappedConstantBufferData) {
			m_constantBuffer->Unmap(0, nullptr);
			m_mappedConstantBufferData = nullptr;
		}

		m_constantBuffer.Reset();
		m_pipelineState.Reset();
		m_vertexShader.reset();
		m_pixelShader.reset();

		for (auto& slot : m_textureSlots) {
			slot.texture.reset();
		}

		SetState(ResourceState::Unloaded);
		Logger::Instance().Debug("머티리얼 언로드: {}", m_path);
	}

	void MaterialResource::SetShaders(std::shared_ptr<ShaderResource> vertex, std::shared_ptr<ShaderResource> pixel)
	{
		m_vertexShader = vertex;
		m_pixelShader = pixel;
		m_pipelineState.Reset(); // 파이프라인 상태 재생성 필요
	}

	void MaterialResource::SetBaseColorTexture(std::shared_ptr<TextureResource> texture)
	{
		m_textureSlots[0].texture = texture;
		if (texture) {
			m_textureSlots[0].handle = texture->GetGPUSRVHandle();
			m_materialConstants.useNormalMap = 1;
		}
	}

	void MaterialResource::SetNormalTexture(std::shared_ptr<TextureResource> texture)
	{
		m_textureSlots[1].texture = texture;
		if (texture) {
			m_textureSlots[1].handle = texture->GetGPUSRVHandle();
			m_materialConstants.useNormalMap = 1;
		}
	}

	void MaterialResource::SetMetallicRoughnessTexture(std::shared_ptr<TextureResource> texture)
	{
		m_textureSlots[2].texture = texture;
		if (texture) {
			m_textureSlots[2].handle = texture->GetGPUSRVHandle();
			m_materialConstants.useNormalMap = 1;
		}
	}

	void MaterialResource::SetPipelineSettings(
		const D3D12_RASTERIZER_DESC& rasterizer, 
		const D3D12_BLEND_DESC& blend, 
		const D3D12_DEPTH_STENCIL_DESC& depthStencil)
	{
		m_rasterizerDesc = rasterizer;
		m_blendDesc = blend;
		m_depthStencilDesc = depthStencil;
		m_pipelineState.Reset();
	}

	ID3D12PipelineState* MaterialResource::GetPipelineState()
	{
		if (!m_pipelineState && !CreatePipelineState()) {
			Logger::Instance().Error("파이프라인 상태 객체를 가져올 수 없음");
			return nullptr;
		}
		return m_pipelineState.Get();
	}

	void MaterialResource::UpdateMaterialConstants()
	{
		if (m_constantsDirty && m_mappedConstantBufferData) {
			memcpy(m_mappedConstantBufferData, &m_materialConstants, sizeof(MaterialConstants));
			m_constantsDirty = false;
		}
	}

	void MaterialResource::SetBaseColor(const XMFLOAT4& color)
	{
		m_materialConstants.baseColor = color;
		m_constantsDirty = true;
	}

	void MaterialResource::SetRoughness(float roughness)
	{
		m_materialConstants.materialParams.y = roughness;
		m_constantsDirty = true;
	}

	void MaterialResource::SetMetallic(float metallic)
	{
		m_materialConstants.materialParams.x = metallic;
		m_constantsDirty = true;
	}

	void MaterialResource::SetAO(float ao)
	{
		m_materialConstants.materialParams.z = ao;
		m_constantsDirty = true;
	}

	void MaterialResource::SetEmissive(const XMFLOAT4& emissive)
	{
		m_materialConstants.emissiveColor = emissive;
		m_constantsDirty = true;
	}

	bool MaterialResource::CreateConstantBuffer()
	{
		// 상수 버퍼는 256바이트 정렬 필요
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
			Logger::Instance().Error("머티리얼 상수 버퍼 생성 실패");
			return false;
		}

		// CPU에서 상수 버퍼 매핑
		CD3DX12_RANGE readRange(0, 0);  // CPU에서는 읽지 않음
		hr = m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedConstantBufferData));
		if (FAILED(hr)) {
			Logger::Instance().Error("상수 버퍼 매핑 실패");
			return false;
		}

		// 초기 데이터 복사
		memcpy(m_mappedConstantBufferData, &m_materialConstants, sizeof(MaterialConstants));

		return true;
	}

	bool MaterialResource::CreateConstantBufferView()
	{
		auto device = Engine::Instance().GetDevice();
		auto descHeap = Engine::Instance().GetDescriptorHeap();
		UINT descriptorIndex = Engine::Instance().GetMaterialCbvDescriptorIndex();
		UINT descriptorSize = Engine::Instance().GetDescriptorIncrementSize();

		if (!device || !descHeap) return false;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(descHeap->GetCPUDescriptorHandleForHeapStart());
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

	bool MaterialResource::CreatePipelineState()
	{
		if (!m_vertexShader || !m_pixelShader) {
			Logger::Instance().Error("PSO 생성 실패: 셰이더가 설정되지 않음");
			return false;
		}

		auto device = Engine::Instance().GetDevice();
		auto rootSignature = Engine::Instance().GetRootSignature();
		if (!device || !rootSignature) return false;

		// 입력 레이아웃 정의
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 40,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 52,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature;
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_vertexShader->GetShaderBlob());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pixelShader->GetShaderBlob());

		psoDesc.RasterizerState = m_rasterizerDesc;
		// Test //
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		//////////

		psoDesc.BlendState = m_blendDesc;
		psoDesc.DepthStencilState = m_depthStencilDesc;
		// Test //
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		//////////

		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(&m_pipelineState));

		if (FAILED(hr)) {
			Logger::Instance().Error("PSO 생성 실패");
			return false;
		}

		return true;
	}
}