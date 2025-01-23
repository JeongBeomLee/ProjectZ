#include "pch.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include "Transform.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "Logger.h"

MeshRenderer::MeshRenderer()
    : m_resources(std::make_unique<MeshResources>()) 
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
    m_resources.reset();
    m_isInitialized = false;
}

bool MeshRenderer::CreateResources(const std::vector<Vertex>& vertices,
    const std::vector<UINT>& indices,
    const std::string& texturePath) 
{
    auto device = Engine::Instance().GetDevice();
    if (!device) return false;

    if (!CreateVertexBuffer(vertices)) return false;
    if (!CreateIndexBuffer(indices)) return false;

    if (!CreateConstantBuffer()) return false;
	if (!CreateConstantBufferView(device)) return false;

    m_textureResource = 
        Resource::ResourceManager::Instance().Load<Resource::TextureResource>(texturePath);
    if (!m_textureResource) {
        Logger::Instance().Error("텍스처 리소스 로드 실패: {}", texturePath);
        return false;
    }

    UpdateBoundingSphere(vertices);

    m_isInitialized = true;
    return true;
}

void MeshRenderer::Render(ID3D12GraphicsCommandList* commandList) 
{
    if (!m_isInitialized || !IsEnabled()) return;

    // 상수 버퍼 뷰 설정
    commandList->SetGraphicsRootDescriptorTable(0, m_resources->cbvHandle);

    // 텍스처 SRV 설정
    commandList->SetGraphicsRootDescriptorTable(2, m_textureResource->GetGPUSRVHandle());

    // 정점 버퍼 설정
    commandList->IASetVertexBuffers(0, 1, &m_resources->vertexBufferView);
    commandList->IASetIndexBuffer(&m_resources->indexBufferView);

    // 드로우 콜
    commandList->DrawIndexedInstanced(m_resources->indexCount, 1, 0, 0, 0);
}

void MeshRenderer::UpdateConstantBuffer() 
{
    if (!m_resources->constantBufferMappedData) {
		Logger::Instance().Error("상수 버퍼 매핑 실패");
		return;
    }

    auto transform = GetGameObject()->GetTransform();

    // 상수 버퍼 데이터 업데이트
    ObjectConstants constants;
    constants.worldMatrix = XMMatrixTranspose(transform->GetWorldMatrix());
    constants.viewMatrix = XMMatrixTranspose(Engine::Instance().GetViewMatrix());
    constants.projectionMatrix = XMMatrixTranspose(Engine::Instance().GetProjectionMatrix());

    memcpy(m_resources->constantBufferMappedData, &constants, sizeof(ObjectConstants));
}

void MeshRenderer::UpdateBoundingSphere(const std::vector<Vertex>& vertices)
{
    if (vertices.empty()) {
        m_boundingSphereCenter = XMFLOAT3(0, 0, 0);
        m_boundingSphereRadius = 1.0f;
        return;
    }

    // 중심점 계산
    XMFLOAT3 center(0, 0, 0);
    for (const auto& vertex : vertices) {
        center.x += vertex.position.x;
        center.y += vertex.position.y;
        center.z += vertex.position.z;
    }

    float invCount = 1.0f / vertices.size();
    center.x *= invCount;
    center.y *= invCount;
    center.z *= invCount;

    // 반지름 계산
    float maxRadiusSq = 0.0f;
    for (const auto& vertex : vertices) {
        float dx = vertex.position.x - center.x;
        float dy = vertex.position.y - center.y;
        float dz = vertex.position.z - center.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        maxRadiusSq = std::max(maxRadiusSq, distSq);
    }

    m_boundingSphereCenter = center;
    m_boundingSphereRadius = std::sqrt(maxRadiusSq);
}

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

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resources->constantBuffer));

    if (FAILED(hr)) {
        Logger::Instance().Error("상수 버퍼 생성 실패");
        return false;
    }

    // 상수 버퍼 매핑
    CD3DX12_RANGE readRange(0, 0);
    hr = m_resources->constantBuffer->Map(0, &readRange,
        reinterpret_cast<void**>(&m_resources->constantBufferMappedData));
    if (FAILED(hr)) return false;

    return true;
}

bool MeshRenderer::CreateConstantBufferView(ID3D12Device* device)
{
	UINT descriptorIndex = Engine::Instance().GetCbvDescriptorIndex();
    auto descHeap = Engine::Instance().GetDescriptorHeap();
	UINT descriptorSize = Engine::Instance().GetDescriptorIncrementSize();
	if (!descHeap) return false;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(descHeap->GetCPUDescriptorHandleForHeapStart());
    cbvHandle.Offset(descriptorIndex, descriptorSize);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_resources->constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = (sizeof(ObjectConstants) + 255) & ~255;
    device->CreateConstantBufferView(&cbvDesc, cbvHandle);

    // GPU 디스크립터 핸들 저장
    m_resources->cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        descHeap->GetGPUDescriptorHandleForHeapStart(),
        descriptorIndex,
        descriptorSize);

	return true;
}