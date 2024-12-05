#pragma once
#include "IAllocator.h"

namespace Memory
{
    class StackAllocator : public IAllocator {
    public:
        // �޸� ��Ŀ - ���� ���� ��ġ�� ����
        class Marker {
            friend class StackAllocator;
        private:
            size_t position;
			Marker(size_t pos) : position(pos) {}
        };

        explicit StackAllocator(size_t size, std::string name = "StackAllocator");

        ~StackAllocator() override;

        // ���� ����
        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;

        AllocationResult Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override;

        void Deallocate(void* ptr) override;

        // ���� ���� ��ġ�� ��Ŀ�� ����
        Marker GetMarker() const { return Marker{ m_current }; }

        // ��Ŀ ��ġ�� ������ �ǵ���
        void RollbackTo(Marker marker);

        // ��ü ���� ����
        void Reset();

        size_t GetUsedMemory() const override { return m_current; }
        size_t GetTotalMemory() const override { return m_totalSize; }
        std::string GetName() const override { return m_name; }

    private:
        struct AllocationHeader {
            uint32_t adjustment;  // ������ ���� ������
            uint32_t size;        // �Ҵ�� ũ��
        };

        bool IsHeaderValid(const AllocationHeader* header) const {
            return header >= reinterpret_cast<AllocationHeader*>(m_memory) &&
                header < reinterpret_cast<AllocationHeader*>(m_memory + m_totalSize);
        }

        uint8_t* m_memory;      // �Ҵ�� �޸� ���� ������
        size_t m_totalSize;     // ��ü �޸� ũ��
        size_t m_current;       // ���� ���� ������ ��ġ
        size_t m_peak;          // �ִ� ��뷮 (������)
        std::string m_name;     // �Ҵ��� �̸� (������)
    };
}