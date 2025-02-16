#pragma once
#include "Component.h"
#include "ModelResource.h"
#include "MaterialInstance.h"

class MeshRenderer : public Component {
public:
    // 렌더링 리소스들을 담는 구조체 수정
    struct MeshResources {
        // 서브메시별 리소스
        struct SubMeshResources {
            ComPtr<ID3D12Resource> vertexBuffer;
            ComPtr<ID3D12Resource> indexBuffer;
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
            D3D12_INDEX_BUFFER_VIEW indexBufferView;
            UINT indexCount;
            std::shared_ptr<Resource::MaterialInstance> materialInstance;
        };

        std::vector<SubMeshResources> subMeshes;

        // 객체별 상수 버퍼
        ComPtr<ID3D12Resource> constantBuffer;
        D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle;
        UINT8* constantBufferMappedData = nullptr;
    };

    MeshRenderer();
    ~MeshRenderer() override;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Destroy() override;

	// 기존 CreateResources를 Deprecated로 표시하고 SetModel로 대체
    [[deprecated("Use SetModel instead")]]
    bool CreateResources(
        const std::vector<Vertex>& vertices,
        const std::vector<UINT>& indices,
        std::shared_ptr<Resource::MaterialInstance> material);

    // 모델 설정 함수
    bool SetModel(std::shared_ptr<Resource::ModelResource> model);

    // 렌더링 실행
    void Render(ID3D12GraphicsCommandList* commandList);

    // 바운딩 스피어 정보
    const XMFLOAT3& GetBoundingSphereCenter() const { return m_boundingSphereCenter; }
    float GetBoundingSphereRadius() const { return m_boundingSphereRadius; }

private:
    //bool CreateVertexBuffer(const std::vector<Vertex>& vertices);
    //bool CreateIndexBuffer(const std::vector<UINT>& indices);
    bool CreateSubMeshResources(const Resource::ModelResource::SubMesh& subMesh, MeshResources::SubMeshResources& resources);
    bool CreateConstantBuffer();
	bool CreateConstantBufferView(ID3D12Device* device);
    void UpdateConstantBuffer();
    void UpdateBoundingSphere(const std::shared_ptr<Resource::ModelResource>& model);

private:
    std::unique_ptr<MeshResources> m_resources;
    std::shared_ptr<Resource::ModelResource> m_model;
    bool m_isInitialized = false;

    // 전체 메시의 바운딩 스피어 (모든 서브메시 포함)
    XMFLOAT3 m_boundingSphereCenter;
    float m_boundingSphereRadius;
};