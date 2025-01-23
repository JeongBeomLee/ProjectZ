#pragma once
#include "Component.h"
#include "TextureResource.h"

// 렌더링에 필요한 리소스들을 담는 구조체
struct MeshResources {
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    ComPtr<ID3D12Resource> constantBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle;
    UINT indexCount;
    UINT8* constantBufferMappedData;
};

class MeshRenderer : public Component {
public:
    MeshRenderer();
    ~MeshRenderer() override;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Destroy() override;

    // 렌더링 리소스 생성 메서드
    bool CreateResources(const std::vector<Vertex>& vertices,
        const std::vector<UINT>& indices,
        const std::string& texturePath);

    // 렌더링 실행
    void Render(ID3D12GraphicsCommandList* commandList);

    // 상수 버퍼 업데이트
    void UpdateConstantBuffer();

    // 바운딩 스피어 정보
    const XMFLOAT3& GetBoundingSphereCenter() const { return m_boundingSphereCenter; }
    float GetBoundingSphereRadius() const { return m_boundingSphereRadius; }
    void UpdateBoundingSphere(const std::vector<Vertex>& vertices);

private:
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices);
    bool CreateIndexBuffer(const std::vector<UINT>& indices);
    bool CreateConstantBuffer();
	bool CreateConstantBufferView(ID3D12Device* device);

    std::unique_ptr<MeshResources> m_resources;
    std::shared_ptr<Resource::TextureResource> m_textureResource;
    bool m_isInitialized = false;

    XMFLOAT3 m_boundingSphereCenter;
    float m_boundingSphereRadius;
};