#pragma once
#include "IResource.h"

struct SubMeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    DirectX::BoundingSphere boundingSphere;
    uint32_t materialIndex;  // 해당 서브메시가 사용하는 재질 인덱스
};

class MeshResource : public IResource {
public:
    MeshResource(const std::string& id, const std::string& path);
    ~MeshResource() override;

    bool Load() override;
    void Unload() override;
    bool IsLoaded() const override;

    // 서브메시 접근자
    size_t GetSubMeshCount() const { return m_subMeshes.size(); }
    const SubMeshData* GetSubMesh(size_t index) const;
    ID3D12Resource* GetVertexBuffer(size_t index) const;
    ID3D12Resource* GetIndexBuffer(size_t index) const;
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView(size_t index) const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView(size_t index) const;

private:
    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void CreateBuffers();

private:
    struct MeshBuffers {
        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;
    };

    std::vector<SubMeshData> m_subMeshes;
    std::vector<MeshBuffers> m_meshBuffers;
};