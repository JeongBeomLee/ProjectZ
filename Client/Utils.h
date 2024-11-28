#pragma once
void UpdateBufferResource(
    ID3D12Device* device,
    ID3D12CommandQueue* commandQueue,
    ID3D12Resource* destResource,
    const void* bufferData,
    size_t bufferSize);