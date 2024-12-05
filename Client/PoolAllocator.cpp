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
    // ��ü �ʿ��� �޸� ũ�� ���
    m_totalSize = m_blockSize * m_blockCount;

    Logger::Instance().Info("[{}] ������. ��� ũ��: {} bytes, ��� ��: {}, Total: {} bytes",
        m_name, m_blockSize, m_blockCount, m_totalSize);

    // �޸� �Ҵ�
    m_memory = static_cast<uint8_t*>(malloc(m_totalSize));
    if (!m_memory) {
        throw std::runtime_error("Ǯ �Ҵ��ڸ� ���� �޸� �Ҵ翡 �����߽��ϴ�");
    }

    // ���� ����Ʈ �ʱ�ȭ
    InitializeFreeList();
}

Memory::PoolAllocator::~PoolAllocator()
{
    Logger::Instance().Info("[{}] ���ŵ�. �ִ� ��뷮: {} ���", m_name, m_peak);
    free(m_memory);
}

Memory::AllocationResult Memory::PoolAllocator::Allocate(size_t size, size_t alignment)
{
    // ��û�� ũ�Ⱑ ��� ũ�⺸�� ũ�� ����
    if (size > m_blockSize) {
        Logger::Instance().Error("[{}] ��û�� ũ�� {}�� ��� ũ�� {}�� �ʰ��߽��ϴ�.",
            m_name, size, m_blockSize);
        return { nullptr, 0, 0 };
    }

    // �� �̻� ��� ������ ����� ������ ����
    if (!m_freeList) {
        Logger::Instance().Error("[{}] �޸� ����. ��� ������ ����� �����ϴ�", m_name);
        return { nullptr, 0, 0 };
    }

    // ���� ����Ʈ���� ��� �ϳ��� ������
    void* ptr = m_freeList;
    m_freeList = *reinterpret_cast<void**>(m_freeList);

    m_usedBlocks++;
    m_peak = std::max(m_peak, m_usedBlocks);

    Logger::Instance().Debug("[{}] ��� �Ҵ��: {:p}", m_name, ptr);

    return { ptr, m_blockSize, m_alignment };
}

void Memory::PoolAllocator::Deallocate(void* ptr)
{
    if (!ptr) return;

    // �����Ͱ� ��ȿ���� �˻�
    if (!IsPointerValid(ptr)) {
        Logger::Instance().Error("[{}] �߸��� �����ͷ� �޸� ���� �õ�: {:p}", m_name, ptr);
        return;
    }

    // ������ ����� ���� ����Ʈ�� �տ� �߰�
    *reinterpret_cast<void**>(ptr) = m_freeList;
    m_freeList = ptr;

    m_usedBlocks--;

    Logger::Instance().Debug("[{}] ��� ������: {:p}", m_name, ptr);
}

void Memory::PoolAllocator::Reset()
{
    Logger::Instance().Info("[{}] �ʱ�ȭ��. ���� �ִ� ��뷮: {} ���", m_name, m_peak);
    InitializeFreeList();
    m_usedBlocks = 0;
    m_peak = 0;
}

void Memory::PoolAllocator::InitializeFreeList()
{
    // ��� ����� ���� ����Ʈ�� ����
    m_freeList = m_memory;
    uint8_t* current = m_memory;

    for (size_t i = 0; i < m_blockCount - 1; ++i) {
        *reinterpret_cast<void**>(current) = current + m_blockSize;
        current += m_blockSize;
    }

    // ������ ����� nullptr�� ����Ű���� ��
    *reinterpret_cast<void**>(current) = nullptr;

    // ����� ��忡���� �޸𸮸� Ư�� �������� �ʱ�ȭ
    IFDEBUG(memset(m_memory, 0xCD, m_totalSize));
}

bool Memory::PoolAllocator::IsPointerValid(void* ptr) const
{
    // �����Ͱ� �޸� Ǯ ���� ���� �ִ��� Ȯ��
    return ptr >= m_memory &&
        ptr < m_memory + m_totalSize &&
        // ��� ��迡 �´��� Ȯ��
        ((reinterpret_cast<uint8_t*>(ptr) - m_memory) % m_blockSize == 0);
}
