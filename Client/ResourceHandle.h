#pragma once
// 리소스 로딩 설정
struct ResourceLoadConfig {
    bool asyncLoad = true;           // 비동기 로딩 여부
    bool cacheInMemory = true;       // 메모리 캐싱 여부
    size_t memoryBudget = 0;         // 메모리 제한 (0 = 무제한)
    std::string searchPath;          // 리소스 검색 경로
};

// 리소스 핸들
template<typename T>
class ResourceHandle {
public:
    ResourceHandle() : m_resource(nullptr) {}
    explicit ResourceHandle(T* resource) : m_resource(resource) {
        if (m_resource) m_resource->AddRef();
    }

    ResourceHandle(const ResourceHandle& other) : m_resource(other.m_resource) {
        if (m_resource) m_resource->AddRef();
    }

    ResourceHandle(ResourceHandle&& other) noexcept
        : m_resource(other.m_resource) {
        other.m_resource = nullptr;
    }

    ~ResourceHandle() {
        if (m_resource) m_resource->Release();
    }

    ResourceHandle& operator=(const ResourceHandle& other) {
        if (this != &other) {
            if (m_resource) m_resource->Release();
            m_resource = other.m_resource;
            if (m_resource) m_resource->AddRef();
        }
        return *this;
    }

    ResourceHandle& operator=(ResourceHandle&& other) noexcept {
        if (this != &other) {
            if (m_resource) m_resource->Release();
            m_resource = other.m_resource;
            other.m_resource = nullptr;
        }
        return *this;
    }

    T* Get() const { return m_resource; }
    T* operator->() const { return m_resource; }
    T& operator*() const { return *m_resource; }
    bool IsValid() const { return m_resource != nullptr; }

private:
    T* m_resource;
};