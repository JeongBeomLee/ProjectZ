#pragma once
#include "Component.h"

class MaterialResource;
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
                         const std::wstring& texturePath);

    // 렌더링 실행
    void Render(ID3D12GraphicsCommandList* commandList);

    // 상수 버퍼 업데이트
    void UpdateConstantBuffer();

    void SetMaterial(std::shared_ptr<MaterialResource> material) { m_material = material; }
    MaterialResource* GetMaterial() const { return m_material.get(); }

private:
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices);
    bool CreateIndexBuffer(const std::vector<UINT>& indices);
    bool CreateConstantBuffer();
    //bool CreateTextureResource(const std::wstring& texturePath);
    //bool CreateShaderResourceView(ID3D12Device* device);

private:
    // 렌더링에 필요한 리소스들을 담는 구조체
    struct MeshResources {
        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        ComPtr<ID3D12Resource> transformConstantBuffer;
        D3D12_GPU_DESCRIPTOR_HANDLE transformCbvHandle;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;
        UINT indexCount;
        UINT8* transformConstantBufferMappedData;
    };
    
    std::unique_ptr<MeshResources> m_resources;
    std::shared_ptr<MaterialResource> m_material;
    static UINT s_meshCount;  // Transform CBV 할당을 위한 카운터
    UINT m_meshIndex;        // 이 메시의 Transform CBV 인덱스
    bool m_isInitialized;
};