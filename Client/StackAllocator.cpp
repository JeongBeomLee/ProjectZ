#include "pch.h"
#include "StackAllocator.h"
#include "Logger.h"

Memory::StackAllocator::StackAllocator(size_t size, std::string name)
    : m_totalSize(size)
    , m_current(0)
    , m_peak(0)
    , m_name(name)
    , m_memory(nullptr)
{
    Logger::Instance().Info("[{}] 생성됨. 크기: {} 바이트", m_name, m_totalSize);
    m_memory = static_cast<uint8_t*>(malloc(size));

    if (!m_memory) {
        throw std::runtime_error("스택 할당자 메모리 할당 실패");
    }

    // 디버그 모드에서는 메모리를 특정 패턴으로 초기화
    // 0xCD: 새로 할당된 힙 메모리
    // 0xDD: 해제된 힙 메모리
    // 0xFD: 힙 메모리의 경계
    // 0xCC: 초기화되지 않은 스택 메모리
    IFDEBUG(memset(m_memory, 0xCD, size)); // 0xCD는 할당된 메모리를 나타내는 VS 디버그 패턴
}

Memory::StackAllocator::~StackAllocator()
{
    Logger::Instance().Info("[{}] 소멸됨. 최대 사용량: {} 바이트", m_name, m_peak);

    // 디버그 모드에서는 해제 전에 메모리를 다른 패턴으로 표시
    IFDEBUG(memset(m_memory, 0xDD, m_totalSize));  // 0xDD는 해제된 메모리를 나타내는 VS 디버그 패턴
    free(m_memory);
}

Memory::AllocationResult Memory::StackAllocator::Allocate(size_t size, size_t alignment)
{
    // 정렬을 위한 오프셋 계산
    size_t currentPtr = reinterpret_cast<size_t>(m_memory + m_current);
    size_t alignedPtr = AlignSize(currentPtr, alignment);
    size_t adjustment = alignedPtr - currentPtr;

    // 헤더 크기를 포함한 전체 필요 크기 계산
    const size_t headerSize = sizeof(AllocationHeader);
    size_t totalSize = headerSize + size + adjustment;

    // 할당 가능한지 검사
    if (m_current + totalSize > m_totalSize) {
        Logger::Instance().Error("[{}] 메모리 부족. 요청: {} 바이트, 사용 가능: {} 바이트",
            m_name, totalSize, m_totalSize - m_current);
        return { nullptr, 0, 0 };
    }

    // 헤더 정보 저장
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(m_memory + m_current + adjustment);
    header->adjustment = static_cast<uint32_t>(adjustment);
    header->size = static_cast<uint32_t>(size);

    // 실제 메모리 포인터 계산
    void* userPtr = reinterpret_cast<void*>(reinterpret_cast<size_t>(header) + headerSize);

    // 현재 위치 업데이트
    m_current += totalSize;
    m_peak = std::max(m_peak, m_current);

    Logger::Instance().Debug("[{}] {} 바이트 할당됨. 주소: {:p} (정렬: {})",
        m_name, size, userPtr, alignment);

    return { userPtr, size, alignment };
}

void Memory::StackAllocator::Deallocate(void* ptr)
{
    if (!ptr) return;

    // 헤더 위치 계산
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(
        static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));

    // 유효성 검사
    if (!IsHeaderValid(header)) {
        Logger::Instance().Error("[{}] 잘못된 메모리 해제 시도. 주소: {:p}", m_name, ptr);
        return;
    }

    // 스택의 최상위 할당이 아니면 에러
    size_t expectedPos = reinterpret_cast<size_t>(ptr) + header->size;
    if (expectedPos != reinterpret_cast<size_t>(m_memory + m_current)) {
        Logger::Instance().Error("[{}] LIFO 순서로 메모리를 해제해야 합니다", m_name);
        return;
    }

    // 스택 포인터를 이전 위치로 되돌림
    m_current -= (sizeof(AllocationHeader) + header->size + header->adjustment);

    IFDEBUG(memset(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader),
        0xDD,
        sizeof(AllocationHeader) + header->size));

    Logger::Instance().Debug("[{}] {} 바이트 해제됨. 주소: {:p}",
        m_name, header->size, ptr);
}

void Memory::StackAllocator::RollbackTo(Marker marker)
{
    if (marker.position > m_current) {
        Logger::Instance().Error("[{}] 잘못된 마커 위치", m_name);
        return;
    }

    // 디버그 모드에서는 롤백된 메모리를 특정 패턴으로 표시
    IFDEBUG(memset(m_memory + marker.position,
        0xDD,
        m_current - marker.position));

    m_current = marker.position;
	Logger::Instance().Debug("[{}] 마커로 롤백. 현재 위치: {}", m_name, m_current);
}

void Memory::StackAllocator::Reset()
{
    IFDEBUG(memset(m_memory, 0xDD, m_current));
    m_current = 0;
    Logger::Instance().Info("[{}] 초기화됨. 최대 사용량: {} 바이트", m_name, m_peak);
}
