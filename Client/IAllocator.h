#pragma once

namespace Memory
{
    // �޸� ���� �⺻��
    constexpr size_t DEFAULT_ALIGNMENT = 16;

    // �޸� �Ҵ� ����� ��� ����ü
    struct AllocationResult {
        void* ptr;         // �Ҵ�� �޸� ������
        size_t size;       // �Ҵ�� ũ��
        size_t alignment;  // �޸� ����
    };

    // �޸� �Ҵ��� �������̽�
    class IAllocator {
    public:
        virtual ~IAllocator() = default;

        // �޸� �Ҵ�
        virtual AllocationResult Allocate(
            size_t size,
            size_t alignment = DEFAULT_ALIGNMENT
        ) = 0;

        // �޸� ����
        virtual void Deallocate(void* ptr) = 0;

        // ���� ��� ���� �޸� ũ��
        virtual size_t GetUsedMemory() const = 0;

        // �Ҵ�� �� �޸� ũ��
        virtual size_t GetTotalMemory() const = 0;

        // ������ �̸� ���
        virtual std::string GetName() const = 0;

    protected:
        // ���ĵ� ũ�� ��� ��ƿ��Ƽ �Լ�
        static constexpr size_t AlignSize(size_t size, size_t alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }
    };

    // �޸� ������ ��Ÿ���� ��ƿ��Ƽ Ŭ����
    class MemoryRange {
    public:
        MemoryRange(void* start, size_t size)
            : m_start(start)
            , m_size(size)
        {}

        void* GetStart() const { return m_start; }
        size_t GetSize() const { return m_size; }

        // �־��� �����Ͱ� �� �޸� ������ ���ԵǴ��� Ȯ��
        bool Contains(void* ptr) const {
            return  ptr >= m_start &&
                    ptr < static_cast<uint8_t*>(m_start) + m_size;
        }

    private:
        void* m_start;
        size_t m_size;
    };

    // RAII�� Ȱ���� �޸� ���� ���� Ŭ����
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

        // �̵� ������
        ScopedAllocation(ScopedAllocation&& other) noexcept
            : m_allocator(other.m_allocator)
            , m_result(other.m_result) {
            other.m_result.ptr = nullptr;
        }

        // ���� ����
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