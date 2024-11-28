#include "pch.h"
#include "Utils.h"

void UpdateBufferResource(
    ID3D12Device* device,
    ID3D12CommandQueue* commandQueue,
    ID3D12Resource* destResource,
    const void* bufferData,
    size_t bufferSize)
{
    // 업로드 힙의 속성 정의
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    // 업로드용 임시 버퍼 생성
    ComPtr<ID3D12Resource> uploadBuffer;
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));

    // 업로드 버퍼에 데이터 복사
    void* mappedData;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(uploadBuffer->Map(0, &readRange, &mappedData));
    memcpy(mappedData, bufferData, bufferSize);
    uploadBuffer->Unmap(0, nullptr);

    // 커맨드 할당자와 리스트 생성
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

    // 리소스 배리어 설정 (복사 대상을 복사 상태로 전환)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        destResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &barrier);

    // 복사 명령 기록
    commandList->CopyBufferRegion(
        destResource, 0,
        uploadBuffer.Get(), 0,
        bufferSize);

    // 리소스 배리어 설정 (복사 상태에서 사용 가능 상태로 전환)
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        destResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    commandList->ResourceBarrier(1, &barrier);

    // 커맨드 리스트 닫기
    ThrowIfFailed(commandList->Close());

    // 커맨드 리스트 실행
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 펜스를 사용하여 GPU 작업 완료 대기
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