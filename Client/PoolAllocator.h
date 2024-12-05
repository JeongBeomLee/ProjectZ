#pragma once
#include "IAllocator.h"

namespace Memory
{
    class PoolAllocator : public IAllocator {
    public:
        // size: 각 블록의 크기
        // count: 블록의 개수
        // alignment: 블록 정렬 크기
        PoolAllocator(size_t blockSize, size_t blockCount,
            size_t alignment = DEFAULT_ALIGNMENT,
            std::string name = "PoolAllocator");

        ~PoolAllocator() override;

        // 복사 금지
        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        AllocationResult Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override;

        void Deallocate(void* ptr) override;

        size_t GetUsedMemory() const override { return m_usedBlocks * m_blockSize; }
        size_t GetTotalMemory() const override { return m_totalSize; }
        std::string GetName() const override { return m_name; }

        // 추가 통계 정보
        size_t GetBlockSize() const { return m_blockSize; }
        size_t GetBlockCount() const { return m_blockCount; }
        size_t GetUsedBlocks() const { return m_usedBlocks; }
        size_t GetFreeBlocks() const { return m_blockCount - m_usedBlocks; }

        // 풀 리셋
        void Reset();

    private:
        void InitializeFreeList();

        bool IsPointerValid(void* ptr) const;

        uint8_t* m_memory;       // 할당된 메모리 시작 포인터
        void* m_freeList;        // 사용 가능한 블록들의 연결 리스트
        size_t m_blockSize;      // 각 블록의 크기
        size_t m_blockCount;     // 전체 블록 개수
        size_t m_alignment;      // 블록 정렬 크기
        size_t m_totalSize;      // 전체 메모리 크기
        size_t m_usedBlocks;     // 현재 사용 중인 블록 수
        size_t m_peak;           // 최대 사용 블록 수 (디버깅용)
        std::string m_name;      // 할당자 이름 (디버깅용)
    };

    // 특정 타입을 위한 풀 할당자 래퍼
    template<typename T, size_t BlockCount>
    class TypedPoolAllocator : public PoolAllocator {
    public:
        explicit TypedPoolAllocator(const char* name = "TypedPoolAllocator")
            : PoolAllocator(sizeof(T), BlockCount, alignof(T), name) {}

        // 타입 안전성이 보장된 할당/해제 함수
        T* Allocate() {
            auto result = PoolAllocator::Allocate(sizeof(T), alignof(T));
            return static_cast<T*>(result.ptr);
        }

        void Deallocate(T* ptr) {
            PoolAllocator::Deallocate(ptr);
        }
    };
}