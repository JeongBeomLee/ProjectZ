#include "pch.h"
#include "MemoryManager.h"

Memory::MemoryManager& Memory::MemoryManager::Instance()
{
    static MemoryManager instance;
    return instance;
}

bool Memory::MemoryManager::Initialize()
{
    try {
        // ������ �޸� (���� ���۸�)
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; ++i) {
            m_frameAllocators[i] = std::make_unique<ThreadSafeLinearAllocator>(
                8 * 1024 * 1024,  // 8MB
                "FrameAllocator" + std::to_string(i)
            );
        }

        // ���� �޸�
        m_levelAllocator = std::make_unique<ThreadSafeStackAllocator>(
            64 * 1024 * 1024,  // 64MB
            "LevelAllocator"
        );

        // ���� �޸�
        m_permanentAllocator = std::make_unique<ThreadSafeLinearAllocator>(
            32 * 1024 * 1024,  // 32MB
            "PermanentAllocator"
        );

        // ���� ������Ʈ Ǯ
        m_gameObjectAllocator = std::make_unique<ThreadSafePoolAllocator>(
            1024,        // ��� ũ��
            1024,        // ��� ����
            16,          // ����
            "GameObjectAllocator"
        );

        Logger::Instance().Info("�޸� �Ŵ��� �ʱ�ȭ �Ϸ�");
        return true;
    }
    catch (const std::exception& e) {
        Logger::Instance().Fatal("�޸� �Ŵ��� �ʱ�ȭ ����: {}", e.what());
        return false;
    }
}

void Memory::MemoryManager::Shutdown()
{
    Logger::Instance().Info("�޸� �Ŵ��� ����");

    for (auto& allocator : m_frameAllocators) {
        allocator.reset();
    }
    m_levelAllocator.reset();
    m_permanentAllocator.reset();
    m_gameObjectAllocator.reset();
}

void Memory::MemoryManager::BeginFrame()
{
    m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAME_BUFFER_COUNT;
    m_frameAllocators[m_currentFrameIndex]->InvokeMethod(&LinearAllocator::Reset);
}

void Memory::MemoryManager::ClearLevel()
{
    m_levelAllocator->InvokeMethod(&StackAllocator::Reset);
}

Memory::IAllocator* Memory::MemoryManager::GetAllocator(Domain domain)
{
    switch (domain) {
    case Domain::Frame:
        return m_frameAllocators[m_currentFrameIndex].get();
    case Domain::Level:
        return m_levelAllocator.get();
    case Domain::Permanent:
        return m_permanentAllocator.get();
    case Domain::GameObject:
        return m_gameObjectAllocator.get();
    default:
        Logger::Instance().Error("�߸��� �޸� �������� ��û��");
        return nullptr;
    }
}

void Memory::MemoryManager::PrintStats()
{
    Logger::Instance().Info("�޸� �Ŵ��� ���:");
    Logger::Instance().Info("������ �Ҵ���:");
    for (size_t i = 0; i < FRAME_BUFFER_COUNT; ++i) {
        Logger::Instance().Info("  ���� {}: {}/{} ����Ʈ ����",
            i,
            m_frameAllocators[i]->GetUsedMemory(),
            m_frameAllocators[i]->GetTotalMemory());
    }

    Logger::Instance().Info("���� �Ҵ���: {}/{} ����Ʈ ����",
        m_levelAllocator->GetUsedMemory(),
        m_levelAllocator->GetTotalMemory());

    Logger::Instance().Info("���� �Ҵ���: {}/{} ����Ʈ ����",
        m_permanentAllocator->GetUsedMemory(),
        m_permanentAllocator->GetTotalMemory());

    Logger::Instance().Info("���ӿ�����Ʈ �Ҵ���: {}/{} ����Ʈ ����",
        m_gameObjectAllocator->GetUsedMemory(),
        m_gameObjectAllocator->GetTotalMemory());
}
