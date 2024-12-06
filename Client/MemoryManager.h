#pragma once
#include "LinearAllocator.h"
#include "StackAllocator.h"
#include "PoolAllocator.h"
#include "ThreadSafeAllocator.h"
#include "Logger.h"

namespace Memory
{
    class MemoryManager {
    public:
        // �޸� ���� ����
        enum class Domain {
            Frame,      // �����Ӹ��� ���µǴ� �ӽ� �޸�
            Level,      // ����/�� ������ �����Ǵ� �޸�
            Permanent,  // ���� ��ü ���� ���� �����Ǵ� �޸�
            GameObject, // ���� ������Ʈ ���� �޸�
            Count
        };

        static MemoryManager& Instance();

        bool Initialize();

        void Shutdown();

        // ������ ���� �� ȣ��
        void BeginFrame();

        // ���� ���� �� ȣ��
        void ClearLevel();

        // �����κ� �Ҵ��� ���
        IAllocator* GetAllocator(Domain domain);

        // ��� ���� ���
        void PrintStats();

    private:
        MemoryManager() = default;
        ~MemoryManager() { Shutdown(); }

        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;

        std::array<std::unique_ptr<ThreadSafeLinearAllocator>, FRAME_BUFFER_COUNT> m_frameAllocators;
        std::unique_ptr<ThreadSafeStackAllocator> m_levelAllocator;
        std::unique_ptr<ThreadSafeLinearAllocator> m_permanentAllocator;
        std::unique_ptr<ThreadSafePoolAllocator> m_gameObjectAllocator;

        size_t m_currentFrameIndex = 0;
    };

    // ���Ǹ� ���� ���� �Լ���
    inline bool InitializeMemory() {
        return MemoryManager::Instance().Initialize();
    }

    inline void ShutdownMemory() {
        MemoryManager::Instance().Shutdown();
    }

    inline void BeginFrameMemory() {
        MemoryManager::Instance().BeginFrame();
    }

    template<typename T>
    T* AllocateFrameMemory() {
        auto allocator = MemoryManager::Instance().GetAllocator(MemoryManager::Domain::Frame);
        auto result = allocator->Allocate(sizeof(T), alignof(T));
        return new(result.ptr) T();
    }

    template<typename T>
    T* AllocatePermanentMemory() {
        auto allocator = MemoryManager::Instance().GetAllocator(MemoryManager::Domain::Permanent);
        auto result = allocator->Allocate(sizeof(T), alignof(T));
        return new(result.ptr) T();
    }
}