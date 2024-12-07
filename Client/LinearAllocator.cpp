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
    Logger::Instance().Info("[{}] ũ��: {} ����Ʈ�� ������", m_name, m_totalSize);
    m_memory = static_cast<uint8_t*>(malloc(size));

    if (!m_memory) {
        throw std::runtime_error("���� �Ҵ��� �޸� �Ҵ� ����");
    }
}

Memory::LinearAllocator::~LinearAllocator()
{
    Logger::Instance().Info("[{}] ���ŵ�. �ִ� ��뷮: {} ����Ʈ", m_name, m_peak);
    free(m_memory);
}

Memory::AllocationResult Memory::LinearAllocator::Allocate(size_t size, size_t alignment)
{
    // ������ ���� ������ ���
    size_t currentPtr = reinterpret_cast<size_t>(m_memory + m_used);
    size_t alignedPtr = AlignSize(currentPtr, alignment);
    size_t adjustment = alignedPtr - currentPtr;

    size_t totalSize = size + adjustment;

    // �Ҵ� �������� �˻�
    if (m_used + totalSize > m_totalSize) {
        Logger::Instance().Error("[{}] �޸� ����. ��û: {} ����Ʈ, ��밡��: {} ����Ʈ",
            m_name, totalSize, m_totalSize - m_used);
        return { nullptr, 0, 0 };
    }

    // �޸� �Ҵ�
    void* ptr = reinterpret_cast<void*>(alignedPtr);
    m_used += totalSize;

    // �ִ� ��뷮 ����
    m_peak = std::max(m_peak, m_used);

    Logger::Instance().Debug("[{}] {} ����Ʈ �Ҵ��. �ּ�: {:p} (����: {})",
        m_name, size, ptr, alignment);

    return { ptr, size, alignment };
}

void Memory::LinearAllocator::Deallocate(void* ptr)
{
    Logger::Instance().Warning("[{}] ���� �޸� ������ �������� �ʽ��ϴ�", m_name);
}

void Memory::LinearAllocator::Reset()
{
    m_used = 0;
    //Logger::Instance().Info("[{}] �ʱ�ȭ��. �ִ� ��뷮: {} ����Ʈ", m_name, m_peak);
}
