#include "pch.h"
#include "MeshResource.h"
#include "Engine.h"
#include "Logger.h"

bool MeshResource::Load()
{
    try {
        m_state = ResourceState::Loading;

        // Assimp�� ����� �޽� �ε�
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(m_name,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices);

        if (!scene) {
            throw std::runtime_error(importer.GetErrorString());
        }

        // ����޽� ó��
        DirectX::BoundingBox totalBounds;
        bool isFirstBounds = true;

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            Submesh submesh;

            // ���� ������ ��ȯ
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                Vertex vertex;

                // ��ġ
                vertex.position = {
                    mesh->mVertices[v].x,
                    mesh->mVertices[v].y,
                    mesh->mVertices[v].z
                };

                // �븻
                if (mesh->HasNormals()) {
                    vertex.normal = {
                        mesh->mNormals[v].x,
                        mesh->mNormals[v].y,
                        mesh->mNormals[v].z
                    };
                }

                // UV ��ǥ
                if (mesh->HasTextureCoords(0)) {
                    vertex.texCoord = {
                        mesh->mTextureCoords[0][v].x,
                        mesh->mTextureCoords[0][v].y
                    };
                }

                submesh.vertices.push_back(vertex);
            }

            // �ε��� ������ ��ȯ
            for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                    submesh.indices.push_back(face.mIndices[idx]);
                }
            }

            // �ٿ�� �ڽ� ���
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

            // ���ؽ� ���� ����
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

            // �ε��� ���� ����
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

            // ������ ���ε�
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
