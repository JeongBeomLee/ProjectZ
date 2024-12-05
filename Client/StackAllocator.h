#pragma once
#include "IAllocator.h"

namespace Memory
{
    class StackAllocator : public IAllocator {
    public:
        // 메모리 마커 - 현재 스택 위치를 저장
        class Marker {
            friend class StackAllocator;
        private:
            size_t position;
			Marker(size_t pos) : position(pos) {}
        };

        explicit StackAllocator(size_t size, std::string name = "StackAllocator");

        ~StackAllocator() override;

        // 복사 금지
        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;

        AllocationResult Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override;

        void Deallocate(void* ptr) override;

        // 현재 스택 위치를 마커로 저장
        Marker GetMarker() const { return Marker{ m_current }; }

        // 마커 위치로 스택을 되돌림
        void RollbackTo(Marker marker);

        // 전체 스택 리셋
        void Reset();

        size_t GetUsedMemory() const override { return m_current; }
        size_t GetTotalMemory() const override { return m_totalSize; }
        std::string GetName() const override { return m_name; }

    private:
        struct AllocationHeader {
            uint32_t adjustment;  // 정렬을 위한 조정값
            uint32_t size;        // 할당된 크기
        };

        bool IsHeaderValid(const AllocationHeader* header) const {
            return header >= reinterpret_cast<AllocationHeader*>(m_memory) &&
                header < reinterpret_cast<AllocationHeader*>(m_memory + m_totalSize);
        }

        uint8_t* m_memory;      // 할당된 메모리 시작 포인터
        size_t m_totalSize;     // 전체 메모리 크기
        size_t m_current;       // 현재 스택 포인터 위치
        size_t m_peak;          // 최대 사용량 (디버깅용)
        std::string m_name;     // 할당자 이름 (디버깅용)
    };
}