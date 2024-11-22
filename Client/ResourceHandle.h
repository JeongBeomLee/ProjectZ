#pragma once
// ���ҽ� �ε� ����
struct ResourceLoadConfig {
    bool asyncLoad = true;           // �񵿱� �ε� ����
    bool cacheInMemory = true;       // �޸� ĳ�� ����
    size_t memoryBudget = 0;         // �޸� ���� (0 = ������)
    std::string searchPath;          // ���ҽ� �˻� ���
};

// ���ҽ� �ڵ�
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