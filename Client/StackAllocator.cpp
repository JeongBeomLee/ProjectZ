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
    Logger::Instance().Info("[{}] ������. ũ��: {} ����Ʈ", m_name, m_totalSize);
    m_memory = static_cast<uint8_t*>(malloc(size));

    if (!m_memory) {
        throw std::runtime_error("���� �Ҵ��� �޸� �Ҵ� ����");
    }

    // ����� ��忡���� �޸𸮸� Ư�� �������� �ʱ�ȭ
    // 0xCD: ���� �Ҵ�� �� �޸�
    // 0xDD: ������ �� �޸�
    // 0xFD: �� �޸��� ���
    // 0xCC: �ʱ�ȭ���� ���� ���� �޸�
    IFDEBUG(memset(m_memory, 0xCD, size)); // 0xCD�� �Ҵ�� �޸𸮸� ��Ÿ���� VS ����� ����
}

Memory::StackAllocator::~StackAllocator()
{
    Logger::Instance().Info("[{}] �Ҹ��. �ִ� ��뷮: {} ����Ʈ", m_name, m_peak);

    // ����� ��忡���� ���� ���� �޸𸮸� �ٸ� �������� ǥ��
    IFDEBUG(memset(m_memory, 0xDD, m_totalSize));  // 0xDD�� ������ �޸𸮸� ��Ÿ���� VS ����� ����
    free(m_memory);
}

Memory::AllocationResult Memory::StackAllocator::Allocate(size_t size, size_t alignment)
{
    // ������ ���� ������ ���
    size_t currentPtr = reinterpret_cast<size_t>(m_memory + m_current);
    size_t alignedPtr = AlignSize(currentPtr, alignment);
    size_t adjustment = alignedPtr - currentPtr;

    // ��� ũ�⸦ ������ ��ü �ʿ� ũ�� ���
    const size_t headerSize = sizeof(AllocationHeader);
    size_t totalSize = headerSize + size + adjustment;

    // �Ҵ� �������� �˻�
    if (m_current + totalSize > m_totalSize) {
        Logger::Instance().Error("[{}] �޸� ����. ��û: {} ����Ʈ, ��� ����: {} ����Ʈ",
            m_name, totalSize, m_totalSize - m_current);
        return { nullptr, 0, 0 };
    }

    // ��� ���� ����
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(m_memory + m_current + adjustment);
    header->adjustment = static_cast<uint32_t>(adjustment);
    header->size = static_cast<uint32_t>(size);

    // ���� �޸� ������ ���
    void* userPtr = reinterpret_cast<void*>(reinterpret_cast<size_t>(header) + headerSize);

    // ���� ��ġ ������Ʈ
    m_current += totalSize;
    m_peak = std::max(m_peak, m_current);

    Logger::Instance().Debug("[{}] {} ����Ʈ �Ҵ��. �ּ�: {:p} (����: {})",
        m_name, size, userPtr, alignment);

    return { userPtr, size, alignment };
}

void Memory::StackAllocator::Deallocate(void* ptr)
{
    if (!ptr) return;

    // ��� ��ġ ���
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(
        static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));

    // ��ȿ�� �˻�
    if (!IsHeaderValid(header)) {
        Logger::Instance().Error("[{}] �߸��� �޸� ���� �õ�. �ּ�: {:p}", m_name, ptr);
        return;
    }

    // ������ �ֻ��� �Ҵ��� �ƴϸ� ����
    size_t expectedPos = reinterpret_cast<size_t>(ptr) + header->size;
    if (expectedPos != reinterpret_cast<size_t>(m_memory + m_current)) {
        Logger::Instance().Error("[{}] LIFO ������ �޸𸮸� �����ؾ� �մϴ�", m_name);
        return;
    }

    // ���� �����͸� ���� ��ġ�� �ǵ���
    m_current -= (sizeof(AllocationHeader) + header->size + header->adjustment);

    IFDEBUG(memset(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader),
        0xDD,
        sizeof(AllocationHeader) + header->size));

    Logger::Instance().Debug("[{}] {} ����Ʈ ������. �ּ�: {:p}",
        m_name, header->size, ptr);
}

void Memory::StackAllocator::RollbackTo(Marker marker)
{
    if (marker.position > m_current) {
        Logger::Instance().Error("[{}] �߸��� ��Ŀ ��ġ", m_name);
        return;
    }

    // ����� ��忡���� �ѹ�� �޸𸮸� Ư�� �������� ǥ��
    IFDEBUG(memset(m_memory + marker.position,
        0xDD,
        m_current - marker.position));

    m_current = marker.position;
	Logger::Instance().Debug("[{}] ��Ŀ�� �ѹ�. ���� ��ġ: {}", m_name, m_current);
}

void Memory::StackAllocator::Reset()
{
    IFDEBUG(memset(m_memory, 0xDD, m_current));
    m_current = 0;
    Logger::Instance().Info("[{}] �ʱ�ȭ��. �ִ� ��뷮: {} ����Ʈ", m_name, m_peak);
}
