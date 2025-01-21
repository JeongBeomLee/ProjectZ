#pragma once
#include "IResource.h"
#include "ShaderResource.h"

class MaterialResource : public IResource {
public:
    struct MaterialProperties {
        XMFLOAT4 albedo;           // 기본 색상
        XMFLOAT4 emissive;         // 자체 발광 색상
        float roughness;           // 거칠기
        float metallic;            // 금속성
        float ambientOcclusion;    // 환경 차폐도
        float padding;             // 16바이트 정렬을 위한 패딩
    };

    struct TextureFlags {
        BOOL hasAlbedoTexture;
        BOOL hasNormalTexture;
        BOOL hasMetallicRoughnessTexture;
        BOOL hasEmissiveTexture;
        BOOL hasOcclusionTexture;
        BOOL padding[3];  // 16바이트 정렬
    };

    MaterialResource(const std::string& id, const std::string& path);
    ~MaterialResource() override;

    bool Load() override;
    void Unload() override;
    bool IsLoaded() const override;

    // 셰이더 설정
    void SetVertexShader(std::shared_ptr<ShaderResource> shader);
    void SetPixelShader(std::shared_ptr<ShaderResource> shader);

    // 텍스처 설정
    void SetAlbedoTexture(ID3D12Resource* texture);
    void SetNormalTexture(ID3D12Resource* texture);
    void SetMetallicRoughnessTexture(ID3D12Resource* texture);
    void SetEmissiveTexture(ID3D12Resource* texture);
    void SetOcclusionTexture(ID3D12Resource* texture);

    // 속성 설정
    void SetProperties(const MaterialProperties& properties);

    // 상수 버퍼 업데이트
    void UpdateConstantBuffer();

    // 렌더링에 필요한 접근자들
    ShaderResource* GetVertexShader() const { return m_vertexShader.get(); }
    ShaderResource* GetPixelShader() const { return m_pixelShader.get(); }
    ID3D12Resource* GetMaterialConstantBuffer() const { return m_materialConstantBuffer.Get(); }
    ID3D12Resource* GetTextureFlagsConstantBuffer() const { return m_textureFlagsConstantBuffer.Get(); }
    const D3D12_GPU_VIRTUAL_ADDRESS& GetMaterialConstantBufferView() const { return m_materialConstantBufferGPUAddress; }
    const D3D12_GPU_VIRTUAL_ADDRESS& GetTextureFlagsConstantBufferView() const { return m_textureFlagsConstantBufferGPUAddress; }

private:
    bool CreateConstantBuffer();
    void UpdateTextureFlagsBuffer();

private:
    MaterialProperties m_properties;

    // 상수 버퍼 관련 멤버 변수들
    ComPtr<ID3D12Resource> m_materialConstantBuffer;
    D3D12_GPU_VIRTUAL_ADDRESS m_materialConstantBufferGPUAddress;
    
    ComPtr<ID3D12Resource> m_textureFlagsConstantBuffer;
    D3D12_GPU_VIRTUAL_ADDRESS m_textureFlagsConstantBufferGPUAddress;
    
    TextureFlags m_textureFlags;

    std::shared_ptr<ShaderResource> m_vertexShader;
    std::shared_ptr<ShaderResource> m_pixelShader;

    // 텍스처 리소스들
	// TODO: TextureResource 클래스를 사용하도록 수정
    ComPtr<ID3D12Resource> m_albedoTexture;
    ComPtr<ID3D12Resource> m_normalTexture;
    ComPtr<ID3D12Resource> m_metallicRoughnessTexture;
    ComPtr<ID3D12Resource> m_emissiveTexture;
    ComPtr<ID3D12Resource> m_occlusionTexture;

    bool m_constantBufferDirty;
};