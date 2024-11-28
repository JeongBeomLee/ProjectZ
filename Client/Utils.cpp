#include "pch.h"
#include "Utils.h"

void UpdateBufferResource(
    ID3D12Device* device,
    ID3D12CommandQueue* commandQueue,
    ID3D12Resource* destResource,
    const void* bufferData,
    size_t bufferSize)
{
    // ���ε� ���� �Ӽ� ����
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    // ���ε�� �ӽ� ���� ����
    ComPtr<ID3D12Resource> uploadBuffer;
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));

    // ���ε� ���ۿ� ������ ����
    void* mappedData;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(uploadBuffer->Map(0, &readRange, &mappedData));
    memcpy(mappedData, bufferData, bufferSize);
    uploadBuffer->Unmap(0, nullptr);

    // Ŀ�ǵ� �Ҵ��ڿ� ����Ʈ ����
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)));

    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)));

    // ���ҽ� �踮�� ���� (���� ����� ���� ���·� ��ȯ)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        destResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &barrier);

    // ���� ��� ���
    commandList->CopyBufferRegion(
        destResource, 0,
        uploadBuffer.Get(), 0,
        bufferSize);

    // ���ҽ� �踮�� ���� (���� ���¿��� ��� ���� ���·� ��ȯ)
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        destResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    commandList->ResourceBarrier(1, &barrier);

    // Ŀ�ǵ� ����Ʈ �ݱ�
    ThrowIfFailed(commandList->Close());

    // Ŀ�ǵ� ����Ʈ ����
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // �潺�� ����Ͽ� GPU �۾� �Ϸ� ���
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    UINT64 fenceValue = 1;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));

    if (fence->GetCompletedValue() < fenceValue) {
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (eventHandle == nullptr) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}