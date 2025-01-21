#include "pch.h"
#include "MeshRenderer.h"
#include "MaterialResource.h"
#include "GameObject.h"
#include "Transform.h"
#include "Engine.h"
#include "Logger.h"

UINT MeshRenderer::s_meshCount = 0;

MeshRenderer::MeshRenderer()
    : m_resources(std::make_unique<MeshResources>())
    , m_meshIndex(s_meshCount++)
    , m_isInitialized(false)
{
    m_updatePriority = UpdatePriority::Renderer;
	Logger::Instance().Debug("MeshRenderer 컴포넌트 생성됨");
}

MeshRenderer::~MeshRenderer() 
{
    Destroy();
}

void MeshRenderer::Initialize() 
{
    Logger::Instance().Debug("MeshRenderer 초기화됨");
}

void MeshRenderer::Update(float deltaTime) 
{
    if (!m_isInitialized || !IsEnabled()) return;
    UpdateConstantBuffer();
}

void MeshRenderer::Destroy() 
{
    if (m_resources) {
        if (m_resources->transformConstantBufferMappedData) {
            m_resources->transformConstantBuffer->Unmap(0, nullptr);
        }
        m_resources.reset();
    }
    m_isInitialized = false;
    Logger::Instance().Debug("MeshRenderer 리소스 해제됨");
}

bool MeshRenderer::CreateResources(const std::vector<Vertex>& vertices,
                                   const std::vector<UINT>& indices,
                                   const std::wstring& texturePath) 
{
    if (!CreateVertexBuffer(vertices)) return false;
    if (!CreateIndexBuffer(indices)) return false;
    if (!CreateConstantBuffer()) return false;

    m_isInitialized = true;
    return true;
}

void MeshRenderer::Render(ID3D12GraphicsCommandList* commandList) 
{
    if (!m_isInitialized || !m_material) return;

    // Transform CBV 바인딩 (slot 0)
    commandList->SetGraphicsRootDescriptorTable(0, m_resources->transformCbvHandle);

    // Material CBV 바인딩 (slot 1)
    m_material->BindMaterialBuffer(commandList, 1);

    // TextureFlags CBV 바인딩 (slot 3)
    m_material->BindTextureFlagsBuffer(commandList, 3);

    // 텍스처 바인딩 (slot 4)
    m_material->BindTextures(commandList, 4);

    // 버텍스/인덱스 버퍼 설정 및 드로우 콜
    commandList->IASetVertexBuffers(0, 1, &m_resources->vertexBufferView);
    commandList->IASetIndexBuffer(&m_resources->indexBufferView);
    commandList->DrawIndexedInstanced(m_resources->indexCount, 1, 0, 0, 0);
}

void MeshRenderer::UpdateConstantBuffer() 
{
    if (!m_resources->transformConstantBufferMappedData) {
		Logger::Instance().Error("상수 버퍼 매핑 실패");
		return;
    }

    auto transform = GetGameObject()->GetTransform();

    // 상수 버퍼 데이터 업데이트
    ObjectConstants constants;
    constants.worldMatrix = XMMatrixTranspose(transform->GetWorldMatrix());
    constants.viewMatrix = XMMatrixTranspose(Engine::Instance().GetViewMatrix());
    constants.projectionMatrix = XMMatrixTranspose(Engine::Instance().GetProjectionMatrix());

    memcpy(m_resources->transformConstantBufferMappedData, &constants, sizeof(ObjectConstants));
}

//void MeshRenderer::UpdateBoundingSphere(const std::vector<Vertex>& vertices)
//{
//    if (vertices.empty()) {
//        m_boundingSphereCenter = XMFLOAT3(0, 0, 0);
//        m_boundingSphereRadius = 1.0f;
//        return;
//    }
//
//    // 중심점 계산
//    XMFLOAT3 center(0, 0, 0);
//    for (const auto& vertex : vertices) {
//        center.x += vertex.position.x;
//        center.y += vertex.position.y;
//        center.z += vertex.position.z;
//    }
//
//    float invCount = 1.0f / vertices.size();
//    center.x *= invCount;
//    center.y *= invCount;
//    center.z *= invCount;
//
//    // 반지름 계산
//    float maxRadiusSq = 0.0f;
//    for (const auto& vertex : vertices) {
//        float dx = vertex.position.x - center.x;
//        float dy = vertex.position.y - center.y;
//        float dz = vertex.position.z - center.z;
//        float distSq = dx * dx + dy * dy + dz * dz;
//        maxRadiusSq = std::max(maxRadiusSq, distSq);
//    }
//
//    m_boundingSphereCenter = center;
//    m_boundingSphereRadius = std::sqrt(maxRadiusSq);
//}

bool MeshRenderer::CreateVertexBuffer(const std::vector<Vertex>& vertices)
{
    auto device = Engine::Instance().GetDevice();

    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resources->vertexBuffer));

    if (FAILED(hr)) {
        Logger::Instance().Error("정점 버퍼 생성 실패");
        return false;
    }

    // 데이터 복사
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    hr = m_resources->vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(hr)) return false;

    memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
    m_resources->vertexBuffer->Unmap(0, nullptr);

    // 버퍼 뷰 생성
    m_resources->vertexBufferView.BufferLocation = m_resources->vertexBuffer->GetGPUVirtualAddress();
    m_resources->vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_resources->vertexBufferView.SizeInBytes = vertexBufferSize;

    return true;
}

bool MeshRenderer::CreateIndexBuffer(const std::vector<UINT>& indices) 
{
    auto device = Engine::Instance().GetDevice();

    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(UINT));
    m_resources->indexCount = static_cast<UINT>(indices.size());

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resources->indexBuffer));

    if (FAILED(hr)) {
        Logger::Instance().Error("인덱스 버퍼 생성 실패");
        return false;
    }

    // 데이터 복사
    UINT8* pIndexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    hr = m_resources->indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
    if (FAILED(hr)) return false;

    memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
    m_resources->indexBuffer->Unmap(0, nullptr);

    // 버퍼 뷰 생성
    m_resources->indexBufferView.BufferLocation = m_resources->indexBuffer->GetGPUVirtualAddress();
    m_resources->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_resources->indexBufferView.SizeInBytes = indexBufferSize;

    return true;
}

bool MeshRenderer::CreateConstantBuffer() 
{
    auto device = Engine::Instance().GetDevice();
    const UINT constantBufferSize = (sizeof(ObjectConstants) + 255) & ~255;

    // 상수 버퍼 생성
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resources->transformConstantBuffer)));

    // 상수 버퍼 매핑
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(m_resources->transformConstantBuffer->Map(0, &readRange,
        reinterpret_cast<void**>(&m_resources->transformConstantBufferMappedData)));

    // CBV 생성
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_resources->transformConstantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = constantBufferSize;

    auto& engine = Engine::Instance();
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(
        engine.GetDescHeap()->GetCPUDescriptorHandleForHeapStart(),
        engine.GetTransformDescriptorOffset(m_meshIndex),
        engine.GetDescriptorIncrementSize());

    device->CreateConstantBufferView(&cbvDesc, cbvHandle);

    // GPU 디스크립터 핸들 저장
    m_resources->transformCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        engine.GetDescHeap()->GetGPUDescriptorHandleForHeapStart(),
        engine.GetTransformDescriptorOffset(m_meshIndex),
        engine.GetDescriptorIncrementSize());

    return true;
}

//bool MeshRenderer::CreateTextureResource(const std::wstring& texturePath)
//{
//	auto device = Engine::Instance().GetDevice();
//	auto commandQueue = Engine::Instance().GetCommandQueue();
//	if (!device || !commandQueue) return false;
//
//    // 리소스 업로드 배치 생성
//    DirectX::ResourceUploadBatch resourceUpload(device);
//    resourceUpload.Begin();
//
//    // DDS 텍스처 로드
//    if (FAILED(DirectX::CreateDDSTextureFromFile(
//        device,
//        resourceUpload,
//        texturePath.c_str(),
//        m_resources->texture.ReleaseAndGetAddressOf()))) {
//        return false;
//    }
//
//    // 리소스 업로드 실행
//    auto uploadResourcesFinished = resourceUpload.End(commandQueue);
//    uploadResourcesFinished.wait();
//
//    return true;
//}

//bool MeshRenderer::CreateShaderResourceView(ID3D12Device* device)
//{
//    // SRV 생성
//    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//    srvDesc.Format = m_resources->texture->GetDesc().Format;
//    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//    srvDesc.Texture2D.MipLevels = m_resources->texture->GetDesc().MipLevels;
//
//    // 디스크립터 할당 및 생성
//    auto descHeap = Engine::Instance().GetDescriptorHeap();
//	if (!descHeap) return false;
//    UINT descriptorIndex = Engine::Instance().GetSrvDescriptorIndex();
//	UINT descriptorSize = Engine::Instance().GetDescriptorIncrementSize();
//
//    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(descHeap->GetCPUDescriptorHandleForHeapStart());
//    srvHandle.Offset(descriptorIndex, descriptorSize);
//
//    device->CreateShaderResourceView(m_resources->texture.Get(), &srvDesc, srvHandle);
//
//    // GPU 핸들 저장
//    m_resources->srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
//        descHeap->GetGPUDescriptorHandleForHeapStart(),
//        descriptorIndex,
//        descriptorSize);
//
//	return true;
//}
