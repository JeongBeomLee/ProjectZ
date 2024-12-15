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
            Logger::Instance().Info("���ҽ� ����: {}", path);
            it->second->Unload();
            m_resources.erase(it);
        }
    }

    void ResourceManager::GarbageCollect() 
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        for (auto it = m_resources.begin(); it != m_resources.end();) {
            if (it->second.use_count() == 1) {  // ResourceManager�� ���� ��
                Logger::Instance().Info("�̻�� ���ҽ� ����: {}", it->first);
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
            Logger::Instance().Info("���ҽ� ����: {}", path);
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
        Logger::Instance().Info("=== ���ҽ� ���� ===");
        Logger::Instance().Info("�� ���ҽ� ��: {}", m_resources.size());
        Logger::Instance().Info("�� �޸� ��뷮: {} bytes", GetTotalMemoryUsage());

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

        // ���ҽ� �ε� �̺�Ʈ �߻�
        Event::ResourceEvent event(resource->GetPath(), Event::ResourceEvent::Type::Started);
        EventManager::Instance().Publish(event);

        // �񵿱� �ε� ����
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

            // future�� �غ�Ǿ����� Ȯ��
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
                break;  // ���� �ε� ���� �½�ũ�� �����Ƿ� �ߴ�
            }
        }
    }
}