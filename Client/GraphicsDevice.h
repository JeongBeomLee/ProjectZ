#pragma once
#include "pch.h"

class GraphicsDevice {
public:
    static GraphicsDevice& Instance() {
        static GraphicsDevice instance;
        return instance;
    }

    void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue) {
        m_device = device;
        m_commandQueue = commandQueue;
    }

    ID3D12Device* GetDevice() const { return m_device; }
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue; }

private:
    GraphicsDevice() = default;
    ~GraphicsDevice() = default;

    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;

    ID3D12Device* m_device = nullptr;
    ID3D12CommandQueue* m_commandQueue = nullptr;
};