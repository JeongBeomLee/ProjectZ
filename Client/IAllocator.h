#pragma once

namespace Memory
{
    // 메모리 정렬 기본값
    constexpr size_t DEFAULT_ALIGNMENT = 16;

    // 메모리 할당 결과를 담는 구조체
    struct AllocationResult {
        void* ptr;         // 할당된 메모리 포인터
        size_t size;       // 할당된 크기
        size_t alignment;  // 메모리 정렬
    };

    // 메모리 할당자 인터페이스
    class IAllocator {
    public:
        virtual ~IAllocator() = default;

        // 메모리 할당
        virtual AllocationResult Allocate(
            size_t size,
            size_t alignment = DEFAULT_ALIGNMENT
        ) = 0;

        // 메모리 해제
        virtual void Deallocate(void* ptr) = 0;

        // 현재 사용 중인 메모리 크기
        virtual size_t GetUsedMemory() const = 0;

        // 할당된 총 메모리 크기
        virtual size_t GetTotalMemory() const = 0;

        // 디버깅용 이름 얻기
        virtual std::string GetName() const = 0;

    protected:
        // 정렬된 크기 계산 유틸리티 함수
        static constexpr size_t AlignSize(size_t size, size_t alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }
    };

    // 메모리 범위를 나타내는 유틸리티 클래스
    class MemoryRange {
    public:
        MemoryRange(void* start, size_t size)
            : m_start(start)
            , m_size(size)
        {}

        void* GetStart() const { return m_start; }
        size_t GetSize() const { return m_size; }

        // 주어진 포인터가 이 메모리 범위에 포함되는지 확인
        bool Contains(void* ptr) const {
            return  ptr >= m_start &&
                    ptr < static_cast<uint8_t*>(m_start) + m_size;
        }

    private:
        void* m_start;
        size_t m_size;
    };

    // RAII를 활용한 메모리 해제 보장 클래스
    template<typename T>
    class ScopedAllocation {
    public:
        ScopedAllocation(IAllocator& allocator)
            : m_allocator(allocator)
            , m_result{ nullptr, 0, 0 } {
            m_result = allocator.Allocate(sizeof(T), alignof(T));
        }

        ~ScopedAllocation() {
            if (m_result.ptr) {
                m_allocator.Deallocate(m_result.ptr);
            }
        }

        // 이동 생성자
        ScopedAllocation(ScopedAllocation&& other) noexcept
            : m_allocator(other.m_allocator)
            , m_result(other.m_result) {
            other.m_result.ptr = nullptr;
        }

        // 복사 금지
        ScopedAllocation(const ScopedAllocation&) = delete;
        ScopedAllocation& operator=(const ScopedAllocation&) = delete;

        void* Get() const { return m_result.ptr; }
        size_t GetSize() const { return m_result.size; }
        operator bool() const { return m_result.ptr != nullptr; }

    private:
        IAllocator& m_allocator;
        AllocationResult m_result;
    };
}