#pragma once
#include "LockFreeStructures.h"
#include "Resource.h"
#include "ResourceHandle.h"

class ResourceManager {
public:
    static ResourceManager& GetInstance();

    // 초기화 및 종료
    bool Initialize(const ResourceLoadConfig& config);
    void Shutdown();

    // 리소스 로딩 및 관리
    template<typename T>
    ResourceHandle<T> LoadResource(const std::string& name);

    template<typename T>
    std::future<ResourceHandle<T>> LoadResourceAsync(const std::string& name);

    template<typename T>
    bool UnloadResource(const std::string& name);

    void UnloadUnusedResources();
    void UnloadAllResources();

    // 리소스 접근
    template<typename T>
    ResourceHandle<T> GetResource(const std::string& name);

    template<typename T>
    bool HasResource(const std::string& name) const;

    // 캐시 관리
    void SetMemoryBudget(size_t bytes);
    size_t GetTotalMemoryUsage() const;
    void TrimMemory(size_t targetUsage);

    // 통계 및 디버깅
    struct Statistics {
        size_t totalResources;
        size_t loadedResources;
        size_t failedResources;
        size_t pendingLoads;
        size_t memoryUsage;
        size_t peakMemoryUsage;
    };

    Statistics GetStatistics() const;
    void DumpResourceStatus() const;

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    template<typename T>
    T* CreateResource(const std::string& name);

    template<typename T>
    T* FindResourceByName(const std::string& name) const;

    void RegisterResourceType(ResourceType type,
        const std::type_index& typeIndex);

    bool LoadResourceInternal(Resource* resource, bool async);
    void ProcessLoadQueue();
    void UpdateResourceStatus();

private:
    ResourceLoadConfig m_config;
    mutable std::shared_mutex m_resourceMutex;

    // 리소스 컨테이너
    std::unordered_map<std::type_index,
        std::unordered_map<std::string, std::unique_ptr<Resource>>>
        m_resources;

    // 비동기 로딩 큐
    struct LoadRequest {
        Resource* resource;
        std::promise<bool> promise;
    };

    LockFreeQueue<LoadRequest> m_loadQueue;
    std::atomic<size_t> m_pendingLoads{ 0 };
    std::atomic<size_t> m_totalMemoryUsage{ 0 };
    std::atomic<size_t> m_peakMemoryUsage{ 0 };

    // 리소스 타입 매핑
    std::unordered_map<ResourceType, std::type_index> m_typeMap;
    std::unordered_map<std::type_index, ResourceType> m_reverseTypeMap;
};

// 인라인 템플릿 구현
template<typename T>
ResourceHandle<T> ResourceManager::LoadResource(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);

    T* resource = FindResourceByName<T>(name);
    if (!resource) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> writeLock(m_resourceMutex);
        resource = CreateResource<T>(name);
    }

    if (resource && resource->GetState() == ResourceState::Unloaded) {
        LoadResourceInternal(resource, false);
    }

    return ResourceHandle<T>(resource);
}

template<typename T>
std::future<ResourceHandle<T>> ResourceManager::LoadResourceAsync(const std::string& name) {
    auto promise = std::promise<ResourceHandle<T>>();
    auto future = promise.get_future();

    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);
    T* resource = FindResourceByName<T>(name);

    if (!resource) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> writeLock(m_resourceMutex);
        resource = CreateResource<T>(name);
    }

    if (resource) {
        if (resource->GetState() == ResourceState::Unloaded) {
            LoadResourceInternal(resource, true);
        }
        promise.set_value(ResourceHandle<T>(resource));
    }
    else {
        promise.set_exception(std::make_exception_ptr(
            std::runtime_error("Failed to create resource")));
    }

    return future;
}

template<typename T>
bool ResourceManager::UnloadResource(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);
    auto* resource = FindResourceByName<T>(name);

    if (!resource) return false;

    if (resource->GetRefCount() == 0) {
        resource->Unload();
        auto& typeResources = m_resources[std::type_index(typeid(T))];
        typeResources.erase(name);
        return true;
    }

    return false;
}

template<typename T>
ResourceHandle<T> ResourceManager::GetResource(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);
    auto* resource = FindResourceByName<T>(name);
    return ResourceHandle<T>(resource);
}

template<typename T>
bool ResourceManager::HasResource(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);
    return FindResourceByName<T>(name) != nullptr;
}

template<typename T>
T* ResourceManager::FindResourceByName(const std::string& name) const {
    auto typeIndex = std::type_index(typeid(T));
    auto it = m_resources.find(typeIndex);
    if (it != m_resources.end()) {
        auto resourceIt = it->second.find(name);
        if (resourceIt != it->second.end()) {
            return static_cast<T*>(resourceIt->second.get());
        }
    }
    return nullptr;
}

template<typename T>
T* ResourceManager::CreateResource(const std::string& name) {
    auto typeIndex = std::type_index(typeid(T));
    auto& typeResources = m_resources[typeIndex];
    auto resource = std::make_unique<T>(name);
    T* rawPtr = resource.get();
    typeResources[name] = std::move(resource);
    return rawPtr;
}
