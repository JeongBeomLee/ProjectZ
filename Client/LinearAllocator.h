#pragma once
#include "IAllocator.h"

namespace Memory
{
	class LinearAllocator : public IAllocator {
	public:
		// size: ��ü �Ҵ��� �޸� ũ��
		// name: ������ �̸�
		explicit LinearAllocator(size_t size, std::string name = "LinearAllocator");

		~LinearAllocator() override;

		// ���� ����
		LinearAllocator(const LinearAllocator&) = delete;
		LinearAllocator& operator=(const LinearAllocator&) = delete;

		// �Ҵ� ����
		AllocationResult Allocate(size_t size, size_t alignment) override;

		// ���� �Ҵ��ڴ� ���� ������ �������� ����
		void Deallocate(void* ptr) override;

		// ��ü �޸� ����
		void Reset();

		// ���� ��� ���� �޸� ũ��
		size_t GetUsedMemory() const override { return m_used; }

		// ��ü �Ҵ�� �޸� ũ��
		size_t GetTotalMemory() const override { return m_totalSize; }

		// �Ҵ��� �̸� ��ȯ
		std::string GetName() const override { return m_name; }

	private:
		uint8_t* m_memory;     // �Ҵ�� �޸� ���� ������
		size_t m_totalSize;    // ��ü �޸� ũ��
		size_t m_used;         // ���� ��� ���� ũ��
		size_t m_peak;         // �ִ� ��뷮 (������)
		std::string m_name;    // �Ҵ��� �̸� (������)
	};
}