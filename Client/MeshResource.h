#pragma once
#include "Resource.h"
struct Submesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string materialName;
    DirectX::BoundingBox boundingBox;
};

class MeshResource : public Resource {
public:
    explicit MeshResource(const std::string& name)
        : Resource(name) {}

    bool Load() override;
    void Unload() override;
    ResourceType GetType() const override { return ResourceType::Mesh; }
    size_t GetMemoryUsage() const override;

    const std::vector<Submesh>& GetSubmeshes() const { return m_submeshes; }
    const DirectX::BoundingBox& GetBoundingBox() const { return m_boundingBox; }

private:
    std::vector<Submesh> m_submeshes;
    DirectX::BoundingBox m_boundingBox;
    std::vector<ComPtr<ID3D12Resource>> m_vertexBuffers;
    std::vector<ComPtr<ID3D12Resource>> m_indexBuffers;
};