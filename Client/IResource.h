#pragma once
#include "pch.h"

namespace Resource
{
    // 리소스의 기본 상태를 나타내는 열거형
    enum class ResourceState {
        Unloaded,   // 초기 상태
        Loading,    // 로드 중
        Loaded,     // 로드 완료
        Failed      // 로드 실패
    };

    class IResource {
    public:
        virtual ~IResource() = default;

        // 순수 가상 함수들
        virtual bool Load(const std::string& path) = 0;
        virtual void Unload() = 0;

        // 레퍼런스 카운트 관리
        void AddReference() { ++m_refCount; }
        bool RemoveReference() {
            if (--m_refCount == 0) {
                Unload();
                return true;
            }
            return false;
        }

        // 접근자 함수들
        const std::string& GetPath() const { return m_path; }
        uint32_t GetRefCount() const { return m_refCount; }
        ResourceState GetState() const { return m_state; }
        const std::string& GetError() const { return m_error; }

    protected:
        // 파생 클래스에서 접근 가능한 멤버 변수들
        std::string m_path;                      // 리소스 경로
        std::string m_error;                     // 에러 메시지
        std::atomic<uint32_t> m_refCount = 0;    // 레퍼런스 카운트
        ResourceState m_state = ResourceState::Unloaded;  // 현재 상태

        // 상태 변경 헬퍼 함수
        void SetState(ResourceState state) { m_state = state; }
        void SetError(const std::string& error) {
            m_error = error;
            m_state = ResourceState::Failed;
        }
    };
}