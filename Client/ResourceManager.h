#pragma once
#include "IResource.h"
#include "MemoryManager.h"
#include "EventManager.h"
#include "ShaderResource.h"
#include "TextureResource.h"

namespace Resource
{
    class ResourceManager {
    public:
        static ResourceManager& Instance();

        // ���ҽ� ��û - ĳ�õ� ���ҽ��� ������ ��ȯ, ������ ���� ����
        template<typename T, typename... Args>
        std::shared_ptr<T> RequestResource(const std::string& path, Args&&... args) {
            static_assert(std::is_base_of<IResource, T>::value,
                "T must inherit from IResource");

            // ���ҽ� Ÿ�Ժ� ���� Ű ����
            std::string resourceKey = CreateResourceKey<T>(path, args...);

            std::shared_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_resources.find(resourceKey);
            if (it != m_resources.end()) {
                if (auto resource = std::dynamic_pointer_cast<T>(it->second)) {
                    return resource;
                }
                Logger::Instance().Error("���ҽ� Ÿ�� ����ġ: {}", resourceKey);
                return nullptr;
            }
            lock.unlock();

            std::unique_lock<std::shared_mutex> writeLock(m_mutex);
            auto resource = std::make_shared<T>(path, std::forward<Args>(args)...);
            m_resources[resourceKey] = resource;

            QueueResourceLoading(resource);

            return resource;
        }

        // ���ҽ� ���� ��û
        void ReleaseResource(const std::string& path);

        // ������� �ʴ� ���ҽ� ����
        void GarbageCollect();

        // ��� ���ҽ� ��� ����
        void ReleaseAllResources();

        // ���� �޸� ��뷮 ��ȸ
        size_t GetTotalMemoryUsage() const;

        // ���ҽ� ���� ��� (������)
        void PrintResourceStats() const;

    private:
        ResourceManager() = default;
        ~ResourceManager() { ReleaseAllResources(); }

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // �񵿱� ���ҽ� �ε��� ���� ���� �޼���
        void QueueResourceLoading(std::shared_ptr<IResource> resource);
        void ProcessLoadingQueue();

        // ���ҽ� �����
        std::unordered_map<std::string, std::shared_ptr<IResource>> m_resources;

        // �ε� ť
        struct LoadingTask {
            std::shared_ptr<IResource> resource;
            std::future<bool> loadingFuture;
        };
        std::queue<LoadingTask> m_loadingQueue;

		// ���̴� ���ҽ� Ű ����
        template<typename T>
        std::string CreateResourceKey(const std::string& path,
            ShaderResource::ShaderType type, const std::string& entryPoint) {
            if constexpr (std::is_same_v<T, ShaderResource>) {
                return std::format("{}#{}#{}", path,
                    static_cast<int>(type), entryPoint);
            }
            return path;
        }

		// �ؽ�ó ���ҽ� Ű ����
		template<typename T>
        std::string CreateResourceKey(const std::string& path) {
            if constexpr (std::is_same_v<T, TextureResource>) {
                return path;
            }
			return path;
        }

        // ����ȭ�� ���� ���ؽ�
        mutable std::shared_mutex m_mutex;
        std::mutex m_loadingMutex;

        // �޸� ������ ���� �Ҵ���
        Memory::IAllocator* m_allocator = nullptr;
    };

    // ���Ǹ� ���� ���� �Լ���
    template<typename T, typename... Args>
    inline std::shared_ptr<T> RequestResource(const std::string& path, Args&&... args) {
        return ResourceManager::Instance().RequestResource<T>(path, std::forward<Args>(args)...);
    }

    inline void ReleaseResource(const std::string& path) {
        ResourceManager::Instance().ReleaseResource(path);
    }
}