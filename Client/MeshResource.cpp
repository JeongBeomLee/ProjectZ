#include "pch.h"
#include "MeshResource.h"
#include "Engine.h"
#include "Logger.h"

bool MeshResource::Load()
{
    try {
        m_state = ResourceState::Loading;

        // Assimp를 사용한 메시 로딩
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(m_name,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices);

        if (!scene) {
            throw std::runtime_error(importer.GetErrorString());
        }

        // 서브메시 처리
        DirectX::BoundingBox totalBounds;
        bool isFirstBounds = true;

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            Submesh submesh;

            // 정점 데이터 변환
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                Vertex vertex;

                // 위치
                vertex.position = {
                    mesh->mVertices[v].x,
                    mesh->mVertices[v].y,
                    mesh->mVertices[v].z
                };

                // 노말
                if (mesh->HasNormals()) {
                    vertex.normal = {
                        mesh->mNormals[v].x,
                        mesh->mNormals[v].y,
                        mesh->mNormals[v].z
                    };
                }

                // UV 좌표
                if (mesh->HasTextureCoords(0)) {
                    vertex.texCoord = {
                        mesh->mTextureCoords[0][v].x,
                        mesh->mTextureCoords[0][v].y
                    };
                }

                submesh.vertices.push_back(vertex);
            }

            // 인덱스 데이터 변환
            for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                    submesh.indices.push_back(face.mIndices[idx]);
                }
            }

            // 바운딩 박스 계산
            DirectX::BoundingBox meshBounds;
            DirectX::BoundingBox::CreateFromPoints(
                meshBounds,
                submesh.vertices.size(),
                &submesh.vertices[0].position,
                sizeof(Vertex)
            );

            submesh.boundingBox = meshBounds;
            if (isFirstBounds) {
                totalBounds = meshBounds;
                isFirstBounds = false;
            }
            else {
                DirectX::BoundingBox::CreateMerged(totalBounds, totalBounds, meshBounds);
            }

            // 버텍스 버퍼 생성
            auto vertexBufferSize = submesh.vertices.size() * sizeof(Vertex);
            auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

            ComPtr<ID3D12Resource> vertexBuffer;
            ThrowIfFailed(Engine::GetInstance().GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&vertexBuffer)));

            // 인덱스 버퍼 생성
            auto indexBufferSize = submesh.indices.size() * sizeof(uint32_t);
            resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

            ComPtr<ID3D12Resource> indexBuffer;
            ThrowIfFailed(Engine::GetInstance().GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&indexBuffer)));

            // 데이터 업로드
            UpdateBufferResource(Engine::GetInstance().GetDevice(),
                Engine::GetInstance().GetCommandQueue(),
                vertexBuffer.Get(),
                submesh.vertices.data(),
                vertexBufferSize);

            UpdateBufferResource(Engine::GetInstance().GetDevice(),
                Engine::GetInstance().GetCommandQueue(),
                indexBuffer.Get(),
                submesh.indices.data(),
                indexBufferSize);

            m_vertexBuffers.push_back(vertexBuffer);
            m_indexBuffers.push_back(indexBuffer);
            m_submeshes.push_back(std::move(submesh));
        }

        m_boundingBox = totalBounds;
        m_state = ResourceState::Ready;

        LOG_INFO("Mesh loaded: {}, Submeshes: {}", m_name, m_submeshes.size());
        return true;
    }
    catch (const std::exception& e) {
        m_state = ResourceState::Failed;
        m_errorMessage = e.what();
        LOG_ERROR("Failed to load mesh {}: {}", m_name, e.what());
        return false;
    }
}

void MeshResource::Unload()
{
}

size_t MeshResource::GetMemoryUsage() const
{
	return size_t();
}
