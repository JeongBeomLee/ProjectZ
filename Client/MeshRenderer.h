#pragma once
#include "Component.h"
#include "MaterialInstance.h"

// 렌더링에 필요한 리소스들을 담는 구조체
struct MeshResources {
	// 버텍스 버퍼와 인덱스 버퍼
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    UINT indexCount;

	// 오브젝트 상수 버퍼
    ComPtr<ID3D12Resource> constantBuffer;
    D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle;
    UINT8* constantBufferMappedData = nullptr;
};

class MeshRenderer : public Component {
public:
    MeshRenderer();
    ~MeshRenderer() override;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Destroy() override;

    // 렌더링 리소스 생성 메서드
    bool CreateResources(
        const std::vector<Vertex>& vertices,
        const std::vector<UINT>& indices,
        std::shared_ptr<Resource::MaterialInstance> material);

    // 렌더링 실행
    void Render(ID3D12GraphicsCommandList* commandList);

    // 상수 버퍼 업데이트
    void UpdateConstantBuffer();

    // 바운딩 스피어 정보
    const XMFLOAT3& GetBoundingSphereCenter() const { return m_boundingSphereCenter; }
    float GetBoundingSphereRadius() const { return m_boundingSphereRadius; }

private:
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices);
    bool CreateIndexBuffer(const std::vector<UINT>& indices);
    bool CreateConstantBuffer();
	bool CreateConstantBufferView(ID3D12Device* device);
    void UpdateBoundingSphere(const std::vector<Vertex>& vertices);

private:
    std::unique_ptr<MeshResources> m_resources;
    std::shared_ptr<Resource::MaterialInstance> m_materialInstance;
    bool m_isInitialized = false;

    XMFLOAT3 m_boundingSphereCenter;
    float m_boundingSphereRadius;
};