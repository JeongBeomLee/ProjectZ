#include "pch.h"
#include "ModelResource.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "MaterialResource.h"
#include "Logger.h"

namespace Resource
{
	ModelResource::~ModelResource()
	{
		Unload();
	}

	bool ModelResource::Load(const std::string& path)
	{
        m_path = path;
        SetState(ResourceState::Loading);

        try {
            // Assimp 임포터 생성
            Assimp::Importer importer;

            // 씬 로드 옵션 설정
            unsigned int flags = 
                aiProcess_Triangulate |           // 삼각형으로 변환
                aiProcess_GenNormals |            // 노말 생성
                aiProcess_CalcTangentSpace |      // 탄젠트/바이탄젠트 계산
                aiProcess_JoinIdenticalVertices | // 중복 버텍스 제거
                aiProcess_ImproveCacheLocality |  // 캐시 최적화
                aiProcess_LimitBoneWeights |      // 본 가중치 제한
                aiProcess_ConvertToLeftHanded;    // 왼손 좌표계로 변환

#ifdef _DEBUG
            flags |= aiProcess_ValidateDataStructure;
#endif

            // 파일 로드
            const aiScene* scene = importer.ReadFile(path, flags);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                SetError(importer.GetErrorString());
                return false;
            }

            // 루트 노드부터 재귀적으로 처리
            if (!ProcessNode(scene->mRootNode, scene)) {
                return false;
            }

            SetState(ResourceState::Loaded);
            Logger::Instance().Info("모델 로드 성공: {}", path);
            return true;
        }
        catch (const std::exception& e) {
            SetError(std::string("모델 로드 실패: ") + e.what());
            return false;
        }
	}

    void ModelResource::Unload()
    {
        m_subMeshes.clear();
        SetState(ResourceState::Unloaded);
    }

    bool ModelResource::ProcessNode(aiNode* node, const aiScene* scene)
    {
        // 현재 노드의 모든 메시 처리
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            if (!ProcessMesh(mesh, scene)) {
                return false;
            }
        }

        // 자식 노드들 재귀적으로 처리
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            if (!ProcessNode(node->mChildren[i], scene)) {
                return false;
            }
        }

        return true;
    }

    bool ModelResource::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        SubMesh subMesh;

        // 버텍스 데이터 추출
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            // 위치
            vertex.position = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            };

            // 노말
            if (mesh->mNormals) {
                vertex.normal = {
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                };
            }

            // 탄젠트
            if (mesh->mTangents) {
                vertex.tangent = {
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                };
            }

            // 텍스처 좌표
            if (mesh->mTextureCoords[0]) {
                vertex.texCoord = {
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                };
            }

            // 기본 버텍스 컬러
            vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            if (mesh->mColors[0]) {
                vertex.color = {
                    mesh->mColors[0][i].r,
                    mesh->mColors[0][i].g,
                    mesh->mColors[0][i].b,
                    mesh->mColors[0][i].a
                };
            }

            // TODO: 본 가중치와 인덱스는 스켈레탈 애니메이션 구현 시 추가

            subMesh.vertices.push_back(vertex);
        }

        // 인덱스 데이터 추출
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                subMesh.indices.push_back(face.mIndices[j]);
            }
        }

        // 바운딩 볼륨 계산
        CalculateBoundingVolumes(subMesh);

        // 서브메시를 벡터에 추가
        m_subMeshes.push_back(std::move(subMesh));

        // 머테리얼 처리
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            if (!ProcessMaterial(material)) {
                return false;
            }
        }

        return true;
    }

    bool ModelResource::ProcessMaterial(aiMaterial* aiMat)
    {
        auto& resourceManager = ResourceManager::Instance();

        // 기본 PBR 머테리얼 리소스 생성
        std::string materialName = m_path + "_material_" + std::to_string(m_subMeshes.size());
        auto materialResource = resourceManager.LoadMaterial(materialName);

        // 셰이더 설정
        auto vertexShader = resourceManager.LoadShader("shaders.hlsl", ShaderType::Vertex);
        auto pixelShader = resourceManager.LoadShader("shaders.hlsl", ShaderType::Pixel);
        materialResource->SetShaders(vertexShader, pixelShader);

        // 베이스 컬러
        aiColor4D baseColor(1.0f);
        if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor)) {
            materialResource->SetBaseColor(XMFLOAT4(baseColor.r, baseColor.g, baseColor.b, baseColor.a));
        }

        // 금속성/거칠기
        float metallic = 0.0f;
        aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
        materialResource->SetMetallic(metallic);

        float roughness = 0.5f;
        aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
        materialResource->SetRoughness(roughness);

        // 텍스처 로드
        aiString texPath;
        std::filesystem::path modelPath(m_path);
        auto modelDir = modelPath.parent_path();

        // 베이스 컬러 텍스처
        if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)) {
            auto fullPath = modelDir / texPath.C_Str();
            auto texture = resourceManager.LoadTexture(fullPath.string());
            materialResource->SetBaseColorTexture(texture);
        }

        // 노말 맵
        if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath)) {
            auto fullPath = modelDir / texPath.C_Str();
            auto texture = resourceManager.LoadTexture(fullPath.string());
            materialResource->SetNormalTexture(texture);
        }

        // 금속성-거칠기 텍스처
        if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_METALNESS, 0, &texPath)) {
            auto fullPath = modelDir / texPath.C_Str();
            auto texture = resourceManager.LoadTexture(fullPath.string());
            materialResource->SetMetallicRoughnessTexture(texture);
        }

        m_subMeshes.back().material = std::make_shared<MaterialInstance>(materialResource);
        return true;
    }

    void ModelResource::CalculateBoundingVolumes(SubMesh& subMesh)
    {
        if (subMesh.vertices.empty()) {
            subMesh.boundingSphereCenter = XMFLOAT3(0, 0, 0);
            subMesh.boundingSphereRadius = 0.0f;
            subMesh.bounds = PxBounds3::empty();
            return;
        }

        // AABB 계산
        PxVec3 min(FLT_MAX);
        PxVec3 max(-FLT_MAX);

        for (const auto& vertex : subMesh.vertices) {
            // AABB 갱신
            min.x = std::min(min.x, vertex.position.x);
            min.y = std::min(min.y, vertex.position.y);
            min.z = std::min(min.z, vertex.position.z);

            max.x = std::max(max.x, vertex.position.x);
            max.y = std::max(max.y, vertex.position.y);
            max.z = std::max(max.z, vertex.position.z);
        }

        subMesh.bounds = PxBounds3(min, max);

        // 바운딩 스피어 계산
        // 중심점은 AABB의 중심으로 설정
        PxVec3 center = (min + max) * 0.5f;
        subMesh.boundingSphereCenter = XMFLOAT3(center.x, center.y, center.z);

        // 반지름 계산
        float maxRadiusSq = 0.0f;
        for (const auto& vertex : subMesh.vertices) {
            float dx = vertex.position.x - center.x;
            float dy = vertex.position.y - center.y;
            float dz = vertex.position.z - center.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            maxRadiusSq = std::max(maxRadiusSq, distSq);
        }

        subMesh.boundingSphereRadius = std::sqrt(maxRadiusSq);
    }
}