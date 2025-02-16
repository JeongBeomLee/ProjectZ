#pragma once
#include "IResource.h"
#include "MaterialInstance.h"

namespace Resource {
    class ModelResource : public IResource {
    public:
        // 서브메시 데이터를 저장하는 구조체
        struct SubMesh {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            std::shared_ptr<MaterialInstance> material;
            // 바운딩 볼륨
            XMFLOAT3 boundingSphereCenter;
            float boundingSphereRadius;
            // 물리 시스템용 바운딩 박스
            PxBounds3 bounds;
        };

        ModelResource() = default;
        ~ModelResource() override;

        bool Load(const std::string& path) override;
        void Unload() override;

        const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }

    private:
        bool ProcessNode(aiNode* node, const aiScene* scene);
        bool ProcessMesh(aiMesh* mesh, const aiScene* scene);
        bool ProcessMaterial(aiMaterial* aiMat);
        void CalculateBoundingVolumes(SubMesh& subMesh);

    private:
        std::vector<SubMesh> m_subMeshes;
    };
}