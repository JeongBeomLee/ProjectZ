// ResourceManager.cpp
#include "pch.h"
#include "ResourceManager.h"

namespace Resource
{
    ResourceManager::ResourceManager()
        : m_resourceAllocator(32 * 1024 * 1024, "ResourceAllocator")
    {
        Logger::Instance().Info("리소스 매니저 초기화됨");
    }

    ResourceManager::~ResourceManager()
    {
        CleanupUnusedResources();
        Logger::Instance().Info("리소스 매니저 종료됨");
    }

    ResourceManager& ResourceManager::Instance()
    {
        static ResourceManager instance;
        return instance;
    }

    void ResourceManager::CleanupUnusedResources()
    {
        size_t unloadedCount = 0;
        auto it = m_resources.begin();
        while (it != m_resources.end()) {
            if (it->second.expired()) {
                Logger::Instance().Debug("만료된 리소스 제거: {}", it->first);
                it = m_resources.erase(it);
                unloadedCount++;
            }
            else {
				++it;
            }
        }

        Logger::Instance().Info("리소스 정리 완료. 제거된 리소스: {}", unloadedCount);
    }

    size_t ResourceManager::GetLoadedResourceCount() const
    {
        size_t count = 0;
        for (const auto& [path, weakResource] : m_resources) {
            if (!weakResource.expired()) {
                count++;
            }
        }
        return count;
    }

    void ResourceManager::PrintResourceStats() const
    {
        size_t activeCount = GetLoadedResourceCount();
        size_t totalCount = m_resources.size();

        Logger::Instance().Info("=== 리소스 통계 ===");
        Logger::Instance().Info("활성 리소스: {}", activeCount);
        Logger::Instance().Info("캐시된 총 리소스: {}", totalCount);
        Logger::Instance().Info("메모리 사용량: {}/{} 바이트",
            m_resourceAllocator.GetUsedMemory(),
            m_resourceAllocator.GetTotalMemory());

        // TODO: 타입별 리소스 카운트 출력은 나중에 구현
    }

    void ResourceManager::PreloadResources(const std::string& manifestPath)
    {
        // TODO: Phase 5에서 구현 예정
        Logger::Instance().Info("리소스 프리로딩 시작: {}", manifestPath);
    }
}