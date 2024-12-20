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

        // 리소스 요청 - 캐시된 리소스가 있으면 반환, 없으면 새로 생성
        template<typename T, typename... Args>
        std::shared_ptr<T> RequestResource(const std::string& path, Args&&... args) {
            static_assert(std::is_base_of<IResource, T>::value,
                "T must inherit from IResource");

            // 리소스 타입별 고유 키 생성
            std::string resourceKey = CreateResourceKey<T>(path, args...);

            std::shared_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_resources.find(resourceKey);
            if (it != m_resources.end()) {
                if (auto resource = std::dynamic_pointer_cast<T>(it->second)) {
                    return resource;
                }
                Logger::Instance().Error("리소스 타입 불일치: {}", resourceKey);
                return nullptr;
            }
            lock.unlock();

            std::unique_lock<std::shared_mutex> writeLock(m_mutex);
            auto resource = std::make_shared<T>(path, std::forward<Args>(args)...);
            m_resources[resourceKey] = resource;

            QueueResourceLoading(resource);

            return resource;
        }

        // 리소스 해제 요청
        void ReleaseResource(const std::string& path);

        // 사용하지 않는 리소스 정리
        void GarbageCollect();

        // 모든 리소스 즉시 해제
        void ReleaseAllResources();

        // 현재 메모리 사용량 조회
        size_t GetTotalMemoryUsage() const;

        // 리소스 상태 출력 (디버깅용)
        void PrintResourceStats() const;

    private:
        ResourceManager() = default;
        ~ResourceManager() { ReleaseAllResources(); }

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // 비동기 리소스 로딩을 위한 내부 메서드
        void QueueResourceLoading(std::shared_ptr<IResource> resource);
        void ProcessLoadingQueue();

        // 리소스 저장소
        std::unordered_map<std::string, std::shared_ptr<IResource>> m_resources;

        // 로딩 큐
        struct LoadingTask {
            std::shared_ptr<IResource> resource;
            std::future<bool> loadingFuture;
        };
        std::queue<LoadingTask> m_loadingQueue;

		// 셰이더 리소스 키 생성
        template<typename T>
        std::string CreateResourceKey(const std::string& path,
            ShaderResource::ShaderType type, const std::string& entryPoint) {
            if constexpr (std::is_same_v<T, ShaderResource>) {
                return std::format("{}#{}#{}", path,
                    static_cast<int>(type), entryPoint);
            }
            return path;
        }

		// 텍스처 리소스 키 생성
		template<typename T>
        std::string CreateResourceKey(const std::string& path) {
            if constexpr (std::is_same_v<T, TextureResource>) {
                return path;
            }
			return path;
        }

        // 동기화를 위한 뮤텍스
        mutable std::shared_mutex m_mutex;
        std::mutex m_loadingMutex;

        // 메모리 관리를 위한 할당자
        Memory::IAllocator* m_allocator = nullptr;
    };

    // 편의를 위한 전역 함수들
    template<typename T, typename... Args>
    inline std::shared_ptr<T> RequestResource(const std::string& path, Args&&... args) {
        return ResourceManager::Instance().RequestResource<T>(path, std::forward<Args>(args)...);
    }

    inline void ReleaseResource(const std::string& path) {
        ResourceManager::Instance().ReleaseResource(path);
    }
}