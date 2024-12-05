#pragma once
#include "IAllocator.h"

namespace Memory
{
    class PoolAllocator : public IAllocator {
    public:
        // size: �� ����� ũ��
        // count: ����� ����
        // alignment: ��� ���� ũ��
        PoolAllocator(size_t blockSize, size_t blockCount,
            size_t alignment = DEFAULT_ALIGNMENT,
            std::string name = "PoolAllocator");

        ~PoolAllocator() override;

        // ���� ����
        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        AllocationResult Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override;

        void Deallocate(void* ptr) override;

        size_t GetUsedMemory() const override { return m_usedBlocks * m_blockSize; }
        size_t GetTotalMemory() const override { return m_totalSize; }
        std::string GetName() const override { return m_name; }

        // �߰� ��� ����
        size_t GetBlockSize() const { return m_blockSize; }
        size_t GetBlockCount() const { return m_blockCount; }
        size_t GetUsedBlocks() const { return m_usedBlocks; }
        size_t GetFreeBlocks() const { return m_blockCount - m_usedBlocks; }

        // Ǯ ����
        void Reset();

    private:
        void InitializeFreeList();

        bool IsPointerValid(void* ptr) const;

        uint8_t* m_memory;       // �Ҵ�� �޸� ���� ������
        void* m_freeList;        // ��� ������ ��ϵ��� ���� ����Ʈ
        size_t m_blockSize;      // �� ����� ũ��
        size_t m_blockCount;     // ��ü ��� ����
        size_t m_alignment;      // ��� ���� ũ��
        size_t m_totalSize;      // ��ü �޸� ũ��
        size_t m_usedBlocks;     // ���� ��� ���� ��� ��
        size_t m_peak;           // �ִ� ��� ��� �� (������)
        std::string m_name;      // �Ҵ��� �̸� (������)
    };

    // Ư�� Ÿ���� ���� Ǯ �Ҵ��� ����
    template<typename T, size_t BlockCount>
    class TypedPoolAllocator : public PoolAllocator {
    public:
        explicit TypedPoolAllocator(const char* name = "TypedPoolAllocator")
            : PoolAllocator(sizeof(T), BlockCount, alignof(T), name) {}

        // Ÿ�� �������� ����� �Ҵ�/���� �Լ�
        T* Allocate() {
            auto result = PoolAllocator::Allocate(sizeof(T), alignof(T));
            return static_cast<T*>(result.ptr);
        }

        void Deallocate(T* ptr) {
            PoolAllocator::Deallocate(ptr);
        }
    };
}