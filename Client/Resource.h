#pragma once
// 리소스 상태
enum class ResourceState {
    Unloaded,   // 로드되지 않음
    Loading,    // 로딩 중
    Ready,      // 사용 가능
    Failed      // 로드 실패
};

// 리소스 타입
enum class ResourceType {
    Texture,
    Mesh,
    Shader,
    Sound,
    Material,
    Animation
};

// 리소스 기본 클래스 (인터페이스)
class Resource {
public:
    explicit Resource(const std::string& name)
        : m_name(name)
        , m_state(ResourceState::Unloaded)
        , m_refCount(0)
    {}

    virtual ~Resource() = default;

    // 순수 가상 함수
    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual ResourceType GetType() const = 0;
    virtual size_t GetMemoryUsage() const = 0;

    // 공통 인터페이스
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