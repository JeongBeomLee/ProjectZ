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
        // 프레임 메모리 (더블 버퍼링)
        for (size_t i = 0; i < FRAME_BUFFER_COUNT; ++i) {
            m_frameAllocators[i] = std::make_unique<ThreadSafeLinearAllocator>(
                8 * 1024 * 1024,  // 8MB
                "FrameAllocator" + std::to_string(i)
            );
        }

        // 레벨 메모리
        m_levelAllocator = std::make_unique<ThreadSafeStackAllocator>(
            64 * 1024 * 1024,  // 64MB
            "LevelAllocator"
        );

        // 영구 메모리
        m_permanentAllocator = std::make_unique<ThreadSafeLinearAllocator>(
            32 * 1024 * 1024,  // 32MB
            "PermanentAllocator"
        );

        // 게임 오브젝트 풀
        m_gameObjectAllocator = std::make_unique<ThreadSafePoolAllocator>(
            1024,        // 블록 크기
            1024,        // 블록 개수
            16,          // 정렬
            "GameObjectAllocator"
        );

        Logger::Instance().Info("메모리 매니저 초기화 완료");
        return true;
    }
    catch (const std::exception& e) {
        Logger::Instance().Fatal("메모리 매니저 초기화 실패: {}", e.what());
        return false;
    }
}

void Memory::MemoryManager::Shutdown()
{
    Logger::Instance().Info("메모리 매니저 종료");

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
        Logger::Instance().Error("잘못된 메모리 도메인이 요청됨");
        return nullptr;
    }
}

void Memory::MemoryManager::PrintStats()
{
    Logger::Instance().Info("메모리 매니저 통계:");
    Logger::Instance().Info("프레임 할당자:");
    for (size_t i = 0; i < FRAME_BUFFER_COUNT; ++i) {
        Logger::Instance().Info("  버퍼 {}: {}/{} 바이트 사용됨",
            i,
            m_frameAllocators[i]->GetUsedMemory(),
            m_frameAllocators[i]->GetTotalMemory());
    }

    Logger::Instance().Info("레벨 할당자: {}/{} 바이트 사용됨",
        m_levelAllocator->GetUsedMemory(),
        m_levelAllocator->GetTotalMemory());

    Logger::Instance().Info("영구 할당자: {}/{} 바이트 사용됨",
        m_permanentAllocator->GetUsedMemory(),
        m_permanentAllocator->GetTotalMemory());

    Logger::Instance().Info("게임오브젝트 할당자: {}/{} 바이트 사용됨",
        m_gameObjectAllocator->GetUsedMemory(),
        m_gameObjectAllocator->GetTotalMemory());
}
