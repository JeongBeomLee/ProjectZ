#include "pch.h"
#include "MeshResource.h"
#include "Engine.h"
#include "Logger.h"

MeshResource::MeshResource(const std::string& id, const std::string& path)
    : IResource(id, path)
{
}

MeshResource::~MeshResource()
{
    Unload();
}

bool MeshResource::Load()
{
    if (m_isLoaded) return true;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(m_path,
        aiProcess_Triangulate |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::Instance().Error("메시 로드 실패: {}, 오류: {}", m_path, importer.GetErrorString());
        return false;
    }

    // 씬의 모든 메시를 처리
    ProcessNode(scene->mRootNode, scene);
    CreateBuffers();

    m_isLoaded = true;
    Logger::Instance().Info("메시 로드 완료: {} (서브메시 {}개)", m_path, m_subMeshes.size());
    return true;
}

void MeshResource::Unload()
{
    if (!m_isLoaded) return;

    m_meshBuffers.clear();
    m_subMeshes.clear();
    m_isLoaded = false;
    Logger::Instance().Info("메시 언로드: {}", m_path);
}

bool MeshResource::IsLoaded() const
{
    return m_isLoaded;
}

const SubMeshData* MeshResource::GetSubMesh(size_t index) const
{
    if (index >= m_subMeshes.size()) return nullptr;
    return &m_subMeshes[index];
}

ID3D12Resource* MeshResource::GetVertexBuffer(size_t index) const
{
    if (index >= m_meshBuffers.size()) return nullptr;
    return m_meshBuffers[index].vertexBuffer.Get();
}

ID3D12Resource* MeshResource::GetIndexBuffer(size_t index) const
{
    if (index >= m_meshBuffers.size()) return nullptr;
    return m_meshBuffers[index].indexBuffer.Get();
}

const D3D12_VERTEX_BUFFER_VIEW& MeshResource::GetVertexBufferView(size_t index) const
{
    static D3D12_VERTEX_BUFFER_VIEW nullView = {};
    if (index >= m_meshBuffers.size()) return nullView;
    return m_meshBuffers[index].vertexBufferView;
}

const D3D12_INDEX_BUFFER_VIEW& MeshResource::GetIndexBufferView(size_t index) const
{
    static D3D12_INDEX_BUFFER_VIEW nullView = {};
    if (index >= m_meshBuffers.size()) return nullView;
    return m_meshBuffers[index].indexBufferView;
}

void MeshResource::ProcessNode(aiNode* node, const aiScene* scene)
{
    // 현재 노드의 모든 메시를 처리
    for (UINT i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene);
    }

    // 자식 노드들을 재귀적으로 처리
    for (UINT i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene);
    }
}

void MeshResource::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    SubMeshData subMesh;
    subMesh.name = mesh->mName.C_Str();
    subMesh.materialIndex = mesh->mMaterialIndex;

    for (UINT i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // 위치 처리
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        // 노말 처리
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }
        else {
            vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        }

        // 탄젠트 처리
        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent.x = mesh->mTangents[i].x;
            vertex.tangent.y = mesh->mTangents[i].y;
            vertex.tangent.z = mesh->mTangents[i].z;
        }
        else {
            // 탄젠트 정보가 없는 경우 기본값 설정
            // 노말이 상향 벡터와 평행하지 않은 경우 노말의 수직 벡터를 탄젠트로 사용
            XMVECTOR normal = XMLoadFloat3(&vertex.normal);
            XMVECTOR defaultUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            XMVECTOR tangent;

            if (abs(XMVectorGetY(normal)) < 0.99f) {
                tangent = XMVector3Normalize(XMVector3Cross(defaultUp, normal));
            }
            else {
                // 노말이 상향 벡터와 평행한 경우 우측 벡터를 탄젠트로 사용
                tangent = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
            }
            XMStoreFloat3(&vertex.tangent, tangent);
        }

        // 텍스처 좌표 처리
        if (mesh->HasTextureCoords(0)) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.texCoord = XMFLOAT2(0.0f, 0.0f);
        }

        vertex.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        subMesh.vertices.push_back(vertex);

    // 인덱스 데이터 변환
    for (UINT i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++) {
            subMesh.indices.push_back(face.mIndices[j]);
        }
    }

    // 경계 구체 계산
    DirectX::BoundingSphere::CreateFromPoints(
        subMesh.boundingSphere,
        subMesh.vertices.size(),
        &subMesh.vertices[0].position,
        sizeof(Vertex)
    );

    m_subMeshes.push_back(std::move(subMesh));
}

void MeshResource::CreateBuffers()
{
    auto& engine = Engine::Instance();
    auto device = engine.GetDevice();

    m_meshBuffers.resize(m_subMeshes.size());

    for (size_t i = 0; i < m_subMeshes.size(); i++) {
        const auto& subMesh = m_subMeshes[i];
        auto& buffers = m_meshBuffers[i];

        // 버텍스 버퍼 생성
        const UINT vertexBufferSize = static_cast<UINT>(subMesh.vertices.size() * sizeof(Vertex));
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&buffers.vertexBuffer)));

        // 버텍스 데이터 복사
        UINT8* vertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(buffers.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
        memcpy(vertexDataBegin, subMesh.vertices.data(), vertexBufferSize);
        buffers.vertexBuffer->Unmap(0, nullptr);

        // 버텍스 버퍼 뷰 설정
        buffers.vertexBufferView.BufferLocation = buffers.vertexBuffer->GetGPUVirtualAddress();
        buffers.vertexBufferView.StrideInBytes = sizeof(Vertex);
        buffers.vertexBufferView.SizeInBytes = vertexBufferSize;

        // 인덱스 버퍼 생성
        const UINT indexBufferSize = static_cast<UINT>(subMesh.indices.size() * sizeof(uint32_t));
        resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&buffers.indexBuffer)));

        // 인덱스 데이터 복사
        UINT8* indexDataBegin;
        ThrowIfFailed(buffers.indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&indexDataBegin)));
        memcpy(indexDataBegin, subMesh.indices.data(), indexBufferSize);
        buffers.indexBuffer->Unmap(0, nullptr);

        // 인덱스 버퍼 뷰 설정
        buffers.indexBufferView.BufferLocation = buffers.indexBuffer->GetGPUVirtualAddress();
        buffers.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        buffers.indexBufferView.SizeInBytes = indexBufferSize;
    }
}