#pragma once
#include "pch.h"
#include "Logger.h"
namespace Resource
{
    // 리소스의 현재 상태
    enum class State {
        Unloaded,   // 초기 상태
        Loading,    // 로딩 중
        Ready,      // 사용 가능
        Failed      // 로딩 실패
    };

    // 리소스 타입
    enum class Type {
        Texture,    // 텍스처
        Model,      // 3D 모델
        Shader,     // 셰이더
        Sound,      // 사운드
        Material    // 머터리얼
    };

    // 리소스 인터페이스
    class IResource {
    public:
        virtual ~IResource() = default;

        virtual bool Load() = 0;

        virtual void Unload() = 0;

        virtual bool Reload() {
            Unload();
            return Load();
        }

        // 상태 확인 함수들
        bool IsReady() const { return m_state == State::Ready; }
        bool IsLoading() const { return m_state == State::Loading; }
        bool HasFailed() const { return m_state == State::Failed; }

        // Getter
        State GetState() const { return m_state; }
        Type GetType() const { return m_type; }
        const std::string& GetPath() const { return m_path; }
        const std::string& GetName() const { return m_name; }
        size_t GetSize() const { return m_size; }
        uint32_t GetRefCount() const { return m_refCount.load(); }

    protected:
        IResource(Type type, const std::string& path, const std::string& name)
            : m_type(type)
            , m_path(path)
            , m_name(name)
            , m_state(State::Unloaded)
            , m_size(0)
            , m_refCount(0) {
            Logger::Instance().Info("리소스 생성: {} ({})", m_name, m_path);
        }

		// Setter
        void SetState(State state) {
            m_state = state;
            Logger::Instance().Debug("리소스 상태 변경: {} -> {}", m_name,
                static_cast<int>(state));
        }

        void SetSize(size_t size) { m_size = size; }

        // 레퍼런스 카운팅
        void AddRef() { m_refCount++; }
        bool RemoveRef() { return (m_refCount-- == 1); }

    private:
        Type m_type;                    // 리소스 타입
        State m_state;                  // 현재 상태
        std::string m_path;             // 리소스 경로
        std::string m_name;             // 리소스 이름
        size_t m_size;                  // 메모리 사용량
        std::atomic<uint32_t> m_refCount; // 참조 카운트
    };
}