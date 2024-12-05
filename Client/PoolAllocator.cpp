#include "pch.h"
#include "PoolAllocator.h"
#include "Logger.h"

Memory::PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount, size_t alignment, std::string name)
    : m_blockSize(AlignSize(blockSize, alignment))
    , m_blockCount(blockCount)
    , m_alignment(alignment)
    , m_usedBlocks(0)
    , m_peak(0)
    , m_name(name)
{
    // 전체 필요한 메모리 크기 계산
    m_totalSize = m_blockSize * m_blockCount;

    Logger::Instance().Info("[{}] 생성됨. 블록 크기: {} bytes, 블록 수: {}, Total: {} bytes",
        m_name, m_blockSize, m_blockCount, m_totalSize);

    // 메모리 할당
    m_memory = static_cast<uint8_t*>(malloc(m_totalSize));
    if (!m_memory) {
        throw std::runtime_error("풀 할당자를 위한 메모리 할당에 실패했습니다");
    }

    // 프리 리스트 초기화
    InitializeFreeList();
}

Memory::PoolAllocator::~PoolAllocator()
{
    Logger::Instance().Info("[{}] 제거됨. 최대 사용량: {} 블록", m_name, m_peak);
    free(m_memory);
}

Memory::AllocationResult Memory::PoolAllocator::Allocate(size_t size, size_t alignment)
{
    // 요청된 크기가 블록 크기보다 크면 실패
    if (size > m_blockSize) {
        Logger::Instance().Error("[{}] 요청된 크기 {}가 블록 크기 {}를 초과했습니다.",
            m_name, size, m_blockSize);
        return { nullptr, 0, 0 };
    }

    // 더 이상 사용 가능한 블록이 없으면 실패
    if (!m_freeList) {
        Logger::Instance().Error("[{}] 메모리 부족. 사용 가능한 블록이 없습니다", m_name);
        return { nullptr, 0, 0 };
    }

    // 프리 리스트에서 블록 하나를 가져옴
    void* ptr = m_freeList;
    m_freeList = *reinterpret_cast<void**>(m_freeList);

    m_usedBlocks++;
    m_peak = std::max(m_peak, m_usedBlocks);

    Logger::Instance().Debug("[{}] 블록 할당됨: {:p}", m_name, ptr);

    return { ptr, m_blockSize, m_alignment };
}

void Memory::PoolAllocator::Deallocate(void* ptr)
{
    if (!ptr) return;

    // 포인터가 유효한지 검사
    if (!IsPointerValid(ptr)) {
        Logger::Instance().Error("[{}] 잘못된 포인터로 메모리 해제 시도: {:p}", m_name, ptr);
        return;
    }

    // 해제된 블록을 프리 리스트의 앞에 추가
    *reinterpret_cast<void**>(ptr) = m_freeList;
    m_freeList = ptr;

    m_usedBlocks--;

    Logger::Instance().Debug("[{}] 블록 해제됨: {:p}", m_name, ptr);
}

void Memory::PoolAllocator::Reset()
{
    Logger::Instance().Info("[{}] 초기화됨. 이전 최대 사용량: {} 블록", m_name, m_peak);
    InitializeFreeList();
    m_usedBlocks = 0;
    m_peak = 0;
}

void Memory::PoolAllocator::InitializeFreeList()
{
    // 모든 블록을 연결 리스트로 연결
    m_freeList = m_memory;
    uint8_t* current = m_memory;

    for (size_t i = 0; i < m_blockCount - 1; ++i) {
        *reinterpret_cast<void**>(current) = current + m_blockSize;
        current += m_blockSize;
    }

    // 마지막 블록은 nullptr을 가리키도록 함
    *reinterpret_cast<void**>(current) = nullptr;

    // 디버그 모드에서는 메모리를 특정 패턴으로 초기화
    IFDEBUG(memset(m_memory, 0xCD, m_totalSize));
}

bool Memory::PoolAllocator::IsPointerValid(void* ptr) const
{
    // 포인터가 메모리 풀 범위 내에 있는지 확인
    return ptr >= m_memory &&
        ptr < m_memory + m_totalSize &&
        // 블록 경계에 맞는지 확인
        ((reinterpret_cast<uint8_t*>(ptr) - m_memory) % m_blockSize == 0);
}
