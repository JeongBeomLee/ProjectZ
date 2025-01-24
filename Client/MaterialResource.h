#pragma once
#include "IResource.h"
#include "TextureResource.h"
#include "ShaderResource.h"
#include "MaterialParameter.h"

namespace Resource
{
	class MaterialResource : public IResource {
	public:
		struct PipelineSettings {
			D3D12_RASTERIZER_DESC rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			D3D12_BLEND_DESC blend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			D3D12_DEPTH_STENCIL_DESC depthStencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		};

		MaterialResource() = default;
		~MaterialResource() override;

		bool Load(const std::string& path) override;
		void Unload() override;

		void SetShaders(
			std::shared_ptr<ShaderResource> vertex,
			std::shared_ptr<ShaderResource> pixel);

		void SetTexture(
			const std::string& paramName,
			std::shared_ptr<TextureResource> texture);

		// 접근자 함수들
		ShaderResource* GetVertexShader() const { return m_vertexShader.get(); }
		ShaderResource* GetPixelShader() const { return m_pixelShader.get(); }
		TextureResource* GetTexture(const std::string& paramName) const;

		// 파라미터 정의 및 접근
		void DefineParameter(const std::string& name, MaterialParameterType type);
		bool SetParameterData(const std::string& name, const void* data);
		const MaterialParameterInfo* GetParameterInfo(const std::string& name) const;
		size_t GetTotalParameterSize() const { return m_totalParamSize; }

		// 상수 버퍼 관리
		bool CreateConstantBuffer();
		void UpdateConstantBuffer();
		ID3D12Resource* GetConstantBuffer() const { return m_constantBuffer.Get(); }

		// PSO 관리
		void SetPipelineSettings(const PipelineSettings& settings);
		ID3D12PipelineState* GetPipelineState();

	private:
		// PSO 관련 함수들
		bool CreatePipelineState();
		size_t CalculatePipelineStateHash();

		// JSON 파싱 및 파라미터 설정 헬퍼 함수들
		MaterialParameterType ParseParameterType(const std::string& typeStr);
		void SetParameterFromJson(const std::string& name, const json& value);
		void LoadPipelineSettings(const json& renderState);
		D3D12_BLEND ParseBlendFactor(const json& value);

	private:
		std::shared_ptr<ShaderResource> m_vertexShader;
		std::shared_ptr<ShaderResource> m_pixelShader;
		std::unordered_map<std::string, std::shared_ptr<TextureResource>> m_textures;

		// 파라미터 관리
		std::unordered_map<std::string, MaterialParameterInfo> m_parameters;
		size_t m_totalParamSize = 0;
		ComPtr<ID3D12Resource> m_constantBuffer;
		UINT8* m_mappedData = nullptr;
		bool m_parametersDirty = false;

		// PSO
		PipelineSettings m_pipelineSettings;
		ComPtr<ID3D12PipelineState> m_pipelineState;
	};
}