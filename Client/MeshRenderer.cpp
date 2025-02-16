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
    if (!m_isInitialized || !IsEnabled() || !m_resources) return;

    // 객체의 변환 행렬 업데이트
    UpdateConstantBuffer();

    // 각 서브메시의 머테리얼 인스턴스 업데이트
    for (const auto& subMesh : m_resources->subMeshes) {
        if (subMesh.materialInstance) {
            subMesh.materialInstance->UpdateMaterialConstants();
        }
    }

    // 모델이 있고 Transform이 변경되었다면 바운딩 볼륨 업데이트
    auto transform = GetGameObject()->GetTransform();
    if (m_model && transform->IsDirty()) {
        // 추후 스켈레탈 애니메이션 구현 시 여기에서 본 행렬도 업데이트
        UpdateBoundingSphere(m_model);
    }
}

void MeshRenderer::Destroy() 
{
    if (m_resources) {
        if (m_resources->constantBufferMappedData) {
            m_resources->constantBuffer->Unmap(0, nullptr);
            m_resources->constantBufferMappedData = nullptr;
        }
    }

    m_resources.reset();
    m_model.reset();
    m_isInitialized = false;
}

bool MeshRenderer::CreateResources(
    const std::vector<Vertex>& vertices,
    const std::vector<UINT>& indices,
    std::shared_ptr<Resource::MaterialInstance> material)
{
    auto device = Engine::Instance().GetDevice();
    if (!device) return false;

    //if (!CreateVertexBuffer(vertices)) return false;
    //if (!CreateIndexBuffer(indices)) return false;

    if (!CreateConstantBuffer()) return false;
	if (!CreateConstantBufferView(device)) return false;

    //m_materialInstance = material;
    //UpdateBoundingSphere(vertices);
    m_isInitialized = true;
    return true;
}

bool MeshRenderer::SetModel(std::shared_ptr<Resource::ModelResource> model)
{
    if (!model || model->GetState() != Resource::ResourceState::Loaded) {
        return false;
    }

    Destroy();
    m_model = model;
    m_resources = std::make_unique<MeshResources>();

    // 각 서브메시에 대한 리소스 생성
    const auto& subMeshes = model->GetSubMeshes();
    m_resources->subMeshes.reserve(subMeshes.size());

    for (const auto& subMesh : subMeshes) {
        MeshResources::SubMeshResources resources;
        if (!CreateSubMeshResources(subMesh, resources)) {
            return false;
        }
        resources.materialInstance = subMesh.material;
        m_resources->subMeshes.push_back(std::move(resources));
    }

    // 상수 버퍼 생성
    if (!CreateConstantBuffer() || !CreateConstantBufferView(Engine::Instance().GetDevice())) {
        return false;
    }

    UpdateBoundingSphere(model);
    m_isInitialized = true;
    return true;
}

void MeshRenderer::Render(ID3D12GraphicsCommandList* commandList) 
{
    if (!m_isInitialized || !IsEnabled() || !m_resources) return;

    // 객체 상수 버퍼 설정 (변환 행렬)
    commandList->SetGraphicsRootDescriptorTable(0, m_resources->cbvHandle);

    // 각 서브메시 렌더링
    for (const auto& subMesh : m_resources->subMeshes) {
        // 머테리얼이 없다면 스킵
        if (!subMesh.materialInstance) continue;

        // 파이프라인 스테이트 설정
        ID3D12PipelineState* pso = subMesh.materialInstance->GetPipelineState();
        if (!pso) continue;
        commandList->SetPipelineState(pso);

        // 머테리얼 상수 버퍼 설정
        commandList->SetGraphicsRootDescriptorTable(2,
            subMesh.materialInstance->GetMaterialCBVHandle());
        auto materialConstants = subMesh.materialInstance->GetMaterialConstants();

        // 텍스처가 존재할 때만 바인딩
        const auto& baseColorSlot = subMesh.materialInstance->GetTextureSlot(0);
        const auto& normalMapSlot = subMesh.materialInstance->GetTextureSlot(1);
        const auto& metallicRoughnessSlot = subMesh.materialInstance->GetTextureSlot(2);

        // 베이스 컬러 텍스처
        if (baseColorSlot.texture) {
            commandList->SetGraphicsRootDescriptorTable(3, baseColorSlot.handle);
        }

        // 노말 맵
        if (normalMapSlot.texture) {
            commandList->SetGraphicsRootDescriptorTable(4, normalMapSlot.handle);
        }

        // 메탈릭-러프니스 맵
        if (metallicRoughnessSlot.texture) {
            commandList->SetGraphicsRootDescriptorTable(5, metallicRoughnessSlot.handle);
        }

        // 버텍스/인덱스 버퍼 설정
        commandList->IASetVertexBuffers(0, 1, &subMesh.vertexBufferView);
        commandList->IASetIndexBuffer(&subMesh.indexBufferView);

        // 드로우 콜
        commandList->DrawIndexedInstanced(subMesh.indexCount, 1, 0, 0, 0);
    }
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

void MeshRenderer::UpdateBoundingSphere(const std::shared_ptr<Resource::ModelResource>& model)
{
    const auto& subMeshes = model->GetSubMeshes();
    if (subMeshes.empty()) {
        m_boundingSphereCenter = XMFLOAT3(0, 0, 0);
        m_boundingSphereRadius = 0.0f;
        return;
    }

    // 모든 서브메시의 바운딩 스피어를 포함하는 새로운 바운딩 스피어 계산
    if (subMeshes.size() == 1) {
        // 단일 메시인 경우 해당 메시의 바운딩 스피어 사용
        m_boundingSphereCenter = subMeshes[0].boundingSphereCenter;
        m_boundingSphereRadius = subMeshes[0].boundingSphereRadius;
    }
    else {
        // 모든 서브메시의 바운딩 스피어를 포함하는 새로운 바운딩 스피어 계산
        XMVECTOR minPos = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
        XMVECTOR maxPos = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.0f);

        for (const auto& subMesh : subMeshes) {
            XMVECTOR center = XMLoadFloat3(&subMesh.boundingSphereCenter);
            XMVECTOR radius = XMVectorReplicate(subMesh.boundingSphereRadius);

            minPos = XMVectorMin(minPos, XMVectorSubtract(center, radius));
            maxPos = XMVectorMax(maxPos, XMVectorAdd(center, radius));
        }

        // 중심점과 반지름 계산
        XMVECTOR center = XMVectorScale(XMVectorAdd(minPos, maxPos), 0.5f);
        XMVECTOR radius = XMVectorScale(XMVectorSubtract(maxPos, minPos), 0.5f);

        XMStoreFloat3(&m_boundingSphereCenter, center);
        m_boundingSphereRadius = XMVectorGetX(XMVector3Length(radius));
    }
}

//bool MeshRenderer::CreateVertexBuffer(const std::vector<Vertex>& vertices)
//{
//    auto device = Engine::Instance().GetDevice();
//
//    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
//
//    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
//
//    HRESULT hr = device->CreateCommittedResource(
//        &heapProperties,
//        D3D12_HEAP_FLAG_NONE,
//        &resourceDesc,
//        D3D12_RESOURCE_STATE_GENERIC_READ,
//        nullptr,
//        IID_PPV_ARGS(&m_resources->vertexBuffer));
//
//    if (FAILED(hr)) {
//        Logger::Instance().Error("정점 버퍼 생성 실패");
//        return false;
//    }
//
//    // 데이터 복사
//    UINT8* pVertexDataBegin;
//    CD3DX12_RANGE readRange(0, 0);
//    hr = m_resources->vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
//    if (FAILED(hr)) return false;
//
//    memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
//    m_resources->vertexBuffer->Unmap(0, nullptr);
//
//    // 버퍼 뷰 생성
//    m_resources->vertexBufferView.BufferLocation = m_resources->vertexBuffer->GetGPUVirtualAddress();
//    m_resources->vertexBufferView.StrideInBytes = sizeof(Vertex);
//    m_resources->vertexBufferView.SizeInBytes = vertexBufferSize;
//
//    return true;
//}

//bool MeshRenderer::CreateIndexBuffer(const std::vector<UINT>& indices) 
//{
//    auto device = Engine::Instance().GetDevice();
//
//    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(UINT));
//    m_resources->indexCount = static_cast<UINT>(indices.size());
//
//    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
//
//    HRESULT hr = device->CreateCommittedResource(
//        &heapProperties,
//        D3D12_HEAP_FLAG_NONE,
//        &resourceDesc,
//        D3D12_RESOURCE_STATE_GENERIC_READ,
//        nullptr,
//        IID_PPV_ARGS(&m_resources->indexBuffer));
//
//    if (FAILED(hr)) {
//        Logger::Instance().Error("인덱스 버퍼 생성 실패");
//        return false;
//    }
//
//    // 데이터 복사
//    UINT8* pIndexDataBegin;
//    CD3DX12_RANGE readRange(0, 0);
//    hr = m_resources->indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
//    if (FAILED(hr)) return false;
//
//    memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
//    m_resources->indexBuffer->Unmap(0, nullptr);
//
//    // 버퍼 뷰 생성
//    m_resources->indexBufferView.BufferLocation = m_resources->indexBuffer->GetGPUVirtualAddress();
//    m_resources->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
//    m_resources->indexBufferView.SizeInBytes = indexBufferSize;
//
//    return true;
//}

bool MeshRenderer::CreateSubMeshResources(
    const Resource::ModelResource::SubMesh& subMesh, 
    MeshResources::SubMeshResources& resources)
{
    auto device = Engine::Instance().GetDevice();
    if (!device) return false;

    // 버텍스 버퍼 생성
    const UINT vertexBufferSize = static_cast<UINT>(subMesh.vertices.size() * sizeof(Vertex));
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resources.vertexBuffer));

    if (FAILED(hr)) {
        Logger::Instance().Error("버텍스 버퍼 생성 실패");
        return false;
    }

    // 버텍스 데이터 복사
    UINT8* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    hr = resources.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin));
    if (FAILED(hr)) return false;

    memcpy(vertexDataBegin, subMesh.vertices.data(), vertexBufferSize);
    resources.vertexBuffer->Unmap(0, nullptr);

    // 버텍스 버퍼 뷰 생성
    resources.vertexBufferView.BufferLocation = resources.vertexBuffer->GetGPUVirtualAddress();
    resources.vertexBufferView.StrideInBytes = sizeof(Vertex);
    resources.vertexBufferView.SizeInBytes = vertexBufferSize;

    // 인덱스 버퍼 생성 (위와 유사한 과정)
    const UINT indexBufferSize = static_cast<UINT>(subMesh.indices.size() * sizeof(UINT));
    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resources.indexBuffer));

    if (FAILED(hr)) {
        Logger::Instance().Error("인덱스 버퍼 생성 실패");
        return false;
    }

    // 인덱스 데이터 복사
    UINT8* indexDataBegin;
    hr = resources.indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&indexDataBegin));
    if (FAILED(hr)) return false;

    memcpy(indexDataBegin, subMesh.indices.data(), indexBufferSize);
    resources.indexBuffer->Unmap(0, nullptr);

    // 인덱스 버퍼 뷰 생성
    resources.indexBufferView.BufferLocation = resources.indexBuffer->GetGPUVirtualAddress();
    resources.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    resources.indexBufferView.SizeInBytes = indexBufferSize;
    resources.indexCount = static_cast<UINT>(subMesh.indices.size());

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