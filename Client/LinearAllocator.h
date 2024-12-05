#pragma once
#include "IAllocator.h"

namespace Memory
{
	class LinearAllocator : public IAllocator {
	public:
		// size: 전체 할당할 메모리 크기
		// name: 디버깅용 이름
		explicit LinearAllocator(size_t size, std::string name = "LinearAllocator");

		~LinearAllocator() override;

		// 복사 금지
		LinearAllocator(const LinearAllocator&) = delete;
		LinearAllocator& operator=(const LinearAllocator&) = delete;

		// 할당 구현
		AllocationResult Allocate(size_t size, size_t alignment) override;

		// 선형 할당자는 개별 해제를 지원하지 않음
		void Deallocate(void* ptr) override;

		// 전체 메모리 리셋
		void Reset();

		// 현재 사용 중인 메모리 크기
		size_t GetUsedMemory() const override { return m_used; }

		// 전체 할당된 메모리 크기
		size_t GetTotalMemory() const override { return m_totalSize; }

		// 할당자 이름 반환
		std::string GetName() const override { return m_name; }

	private:
		uint8_t* m_memory;     // 할당된 메모리 시작 포인터
		size_t m_totalSize;    // 전체 메모리 크기
		size_t m_used;         // 현재 사용 중인 크기
		size_t m_peak;         // 최대 사용량 (디버깅용)
		std::string m_name;    // 할당자 이름 (디버깅용)
	};
}