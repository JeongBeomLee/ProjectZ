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
        // 메모리 영역 구분
        enum class Domain {
            Frame,      // 프레임마다 리셋되는 임시 메모리
            Level,      // 레벨/씬 단위로 관리되는 메모리
            Permanent,  // 게임 전체 수명 동안 유지되는 메모리
            GameObject, // 게임 오브젝트 전용 메모리
            Count
        };

        static MemoryManager& Instance();

        bool Initialize();

        void Shutdown();

        // 프레임 시작 시 호출
        void BeginFrame();

        // 레벨 변경 시 호출
        void ClearLevel();

        // 도메인별 할당자 얻기
        IAllocator* GetAllocator(Domain domain);

        // 통계 정보 출력
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

    // 편의를 위한 전역 함수들
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