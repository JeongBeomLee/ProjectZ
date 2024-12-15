#include "pch.h"
#include "ResourceManager.h"

namespace Resource
{
    ResourceManager& ResourceManager::Instance() 
    {
        static ResourceManager instance;
        return instance;
    }

    void ResourceManager::ReleaseResource(const std::string& path) 
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_resources.find(path);
        if (it != m_resources.end()) {
            Logger::Instance().Info("리소스 해제: {}", path);
            it->second->Unload();
            m_resources.erase(it);
        }
    }

    void ResourceManager::GarbageCollect() 
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        for (auto it = m_resources.begin(); it != m_resources.end();) {
            if (it->second.use_count() == 1) {  // ResourceManager만 참조 중
                Logger::Instance().Info("미사용 리소스 정리: {}", it->first);
                it->second->Unload();
                it = m_resources.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void ResourceManager::ReleaseAllResources() 
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        for (auto& [path, resource] : m_resources) {
            Logger::Instance().Info("리소스 해제: {}", path);
            resource->Unload();
        }
        m_resources.clear();
    }

    size_t ResourceManager::GetTotalMemoryUsage() const 
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        size_t total = 0;
        for (const auto& [path, resource] : m_resources) {
            total += resource->GetSize();
        }
        return total;
    }

    void ResourceManager::PrintResourceStats() const 
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        Logger::Instance().Info("=== 리소스 상태 ===");
        Logger::Instance().Info("총 리소스 수: {}", m_resources.size());
        Logger::Instance().Info("총 메모리 사용량: {} bytes", GetTotalMemoryUsage());

        for (const auto& [path, resource] : m_resources) {
            Logger::Instance().Info("- {} ({}): {} bytes, {} refs",
                resource->GetName(),
                path,
                resource->GetSize(),
                resource->GetRefCount());
        }
    }

    void ResourceManager::QueueResourceLoading(std::shared_ptr<IResource> resource) 
    {
        std::lock_guard<std::mutex> lock(m_loadingMutex);

        // 리소스 로딩 이벤트 발생
        Event::ResourceEvent event(resource->GetPath(), Event::ResourceEvent::Type::Started);
        EventManager::Instance().Publish(event);

        // 비동기 로딩 시작
        auto future = std::async(std::launch::async, [resource]() {
            return resource->Load();
            });

        m_loadingQueue.push({ resource, std::move(future) });
    }

    void ResourceManager::ProcessLoadingQueue() 
    {
        std::lock_guard<std::mutex> lock(m_loadingMutex);

        while (!m_loadingQueue.empty()) {
            auto& task = m_loadingQueue.front();

            // future가 준비되었는지 확인
            if (task.loadingFuture.wait_for(std::chrono::seconds(0))
                == std::future_status::ready) {

                bool success = task.loadingFuture.get();
                Event::ResourceEvent event(
                    task.resource->GetPath(),
                    success ? Event::ResourceEvent::Type::Completed
                    : Event::ResourceEvent::Type::Failed,
                    success ? "" : "Loading failed"
                    );
                EventManager::Instance().Publish(event);

                m_loadingQueue.pop();
            }
            else {
                break;  // 아직 로딩 중인 태스크가 있으므로 중단
            }
        }
    }
}