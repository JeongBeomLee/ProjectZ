#pragma once
#include "pch.h"
#include "Logger.h"
namespace Resource
{
    // ���ҽ��� ���� ����
    enum class State {
        Unloaded,   // �ʱ� ����
        Loading,    // �ε� ��
        Ready,      // ��� ����
        Failed      // �ε� ����
    };

    // ���ҽ� Ÿ��
    enum class Type {
        Texture,    // �ؽ�ó
        Model,      // 3D ��
        Shader,     // ���̴�
        Sound,      // ����
        Material    // ���͸���
    };

    // ���ҽ� �������̽�
    class IResource {
    public:
        virtual ~IResource() = default;

        virtual bool Load() = 0;

        virtual void Unload() = 0;

        virtual bool Reload() {
            Unload();
            return Load();
        }

        // ���� Ȯ�� �Լ���
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
            Logger::Instance().Info("���ҽ� ����: {} ({})", m_name, m_path);
        }

		// Setter
        void SetState(State state) {
            m_state = state;
            Logger::Instance().Debug("���ҽ� ���� ����: {} -> {}", m_name,
                static_cast<int>(state));
        }

        void SetSize(size_t size) { m_size = size; }

        // ���۷��� ī����
        void AddRef() { m_refCount++; }
        bool RemoveRef() { return (m_refCount-- == 1); }

    private:
        Type m_type;                    // ���ҽ� Ÿ��
        State m_state;                  // ���� ����
        std::string m_path;             // ���ҽ� ���
        std::string m_name;             // ���ҽ� �̸�
        size_t m_size;                  // �޸� ��뷮
        std::atomic<uint32_t> m_refCount; // ���� ī��Ʈ
    };
}