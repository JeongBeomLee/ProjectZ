#pragma once
#include "pch.h"

class IResource {
public:
    virtual ~IResource() = default;

    // 리소스 생명주기 관리
    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual bool IsLoaded() const = 0;

    // 참조 카운트 관리
    void AddRef() { m_refCount++; }
    void Release() {
        m_refCount--;
        if (m_refCount <= 0) {
            Unload();
        }
    }
    uint32_t GetRefCount() const { return m_refCount; }

    // 리소스 식별자 접근
    const std::string& GetId() const { return m_id; }
    const std::string& GetPath() const { return m_path; }

protected:
    IResource(const std::string& id, const std::string& path)
        : m_id(id)
        , m_path(path)
        , m_refCount(0)
        , m_isLoaded(false)
    {
    }

    std::string m_id;      // 리소스의 고유 식별자
    std::string m_path;    // 리소스 파일 경로
    uint32_t m_refCount;   // 참조 카운트
    bool m_isLoaded;       // 로드 상태
};