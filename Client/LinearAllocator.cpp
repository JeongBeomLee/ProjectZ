#include "pch.h"
#include "LinearAllocator.h"
#include "Logger.h"

Memory::LinearAllocator::LinearAllocator(size_t size, std::string name)
    : m_totalSize(size)
    , m_used(0)
    , m_peak(0)
    , m_name(name)
    , m_memory(nullptr)
{
    Logger::Instance().Info("[{}] 크기: {} 바이트로 생성됨", m_name, m_totalSize);
    m_memory = static_cast<uint8_t*>(malloc(size));

    if (!m_memory) {
        throw std::runtime_error("선형 할당자 메모리 할당 실패");
    }
}

Memory::LinearAllocator::~LinearAllocator()
{
    Logger::Instance().Info("[{}] 제거됨. 최대 사용량: {} 바이트", m_name, m_peak);
    free(m_memory);
}

Memory::AllocationResult Memory::LinearAllocator::Allocate(size_t size, size_t alignment)
{
    // 정렬을 위한 오프셋 계산
    size_t currentPtr = reinterpret_cast<size_t>(m_memory + m_used);
    size_t alignedPtr = AlignSize(currentPtr, alignment);
    size_t adjustment = alignedPtr - currentPtr;

    size_t totalSize = size + adjustment;

    // 할당 가능한지 검사
    if (m_used + totalSize > m_totalSize) {
        Logger::Instance().Error("[{}] 메모리 부족. 요청: {} 바이트, 사용가능: {} 바이트",
            m_name, totalSize, m_totalSize - m_used);
        return { nullptr, 0, 0 };
    }

    // 메모리 할당
    void* ptr = reinterpret_cast<void*>(alignedPtr);
    m_used += totalSize;

    // 최대 사용량 갱신
    m_peak = std::max(m_peak, m_used);

    Logger::Instance().Debug("[{}] {} 바이트 할당됨. 주소: {:p} (정렬: {})",
        m_name, size, ptr, alignment);

    return { ptr, size, alignment };
}

void Memory::LinearAllocator::Deallocate(void* ptr)
{
    Logger::Instance().Warning("[{}] 개별 메모리 해제는 지원되지 않습니다", m_name);
}

void Memory::LinearAllocator::Reset()
{
    m_used = 0;
    //Logger::Instance().Info("[{}] 초기화됨. 최대 사용량: {} 바이트", m_name, m_peak);
}
