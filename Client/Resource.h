#pragma once
// ���ҽ� ����
enum class ResourceState {
    Unloaded,   // �ε���� ����
    Loading,    // �ε� ��
    Ready,      // ��� ����
    Failed      // �ε� ����
};

// ���ҽ� Ÿ��
enum class ResourceType {
    Texture,
    Mesh,
    Shader,
    Sound,
    Material,
    Animation
};

// ���ҽ� �⺻ Ŭ���� (�������̽�)
class Resource {
public:
    explicit Resource(const std::string& name)
        : m_name(name)
        , m_state(ResourceState::Unloaded)
        , m_refCount(0)
    {}

    virtual ~Resource() = default;

    // ���� ���� �Լ�
    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual ResourceType GetType() const = 0;
    virtual size_t GetMemoryUsage() const = 0;

    // ���� �������̽�
    const std::string& GetName() const { return m_name; }
    ResourceState GetState() const { return m_state; }
    void AddRef() { ++m_refCount; }
    void Release() {
        if (--m_refCount == 0) {
            Unload();
        }
    }
    uint32_t GetRefCount() const { return m_refCount; }

protected:
    std::string m_name;
    std::atomic<ResourceState> m_state;
    std::atomic<uint32_t> m_refCount;
    std::string m_errorMessage;
};