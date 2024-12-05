#pragma once
#include "IAllocator.h"
#include "LinearAllocator.h"
#include "StackAllocator.h"
#include "PoolAllocator.h"
#include "Logger.h"

namespace Memory
{
    template<typename BaseAllocator>
    class ThreadSafeAllocator : public IAllocator {
    public:
        // �⺻ �����ڸ� ���� ����
        template<typename... Args>
        explicit ThreadSafeAllocator(Args&&... args)
            : m_allocator(std::forward<Args>(args)...)
            , m_name("ThreadSafe" + std::string(m_allocator.GetName())) {
            Logger::Instance().Info("[{}] Created", m_name);
        }

        ~ThreadSafeAllocator() override {
            Logger::Instance().Info("[{}] Destroyed", m_name);
        }

        // ���� ����
        ThreadSafeAllocator(const ThreadSafeAllocator&) = delete;
        ThreadSafeAllocator& operator=(const ThreadSafeAllocator&) = delete;

        AllocationResult Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override {
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            auto result = m_allocator.Allocate(size, alignment);

            if (result.ptr) {
                Logger::Instance().Debug("[{}] Allocated {} bytes at {:p}",
                    m_name, size, result.ptr);
            }

            return result;
        }

        void Deallocate(void* ptr) override {
            if (!ptr) return;

            std::lock_guard<std::shared_mutex> lock(m_mutex);
            Logger::Instance().Debug("[{}] Deallocating memory at {:p}", m_name, ptr);
            m_allocator.Deallocate(ptr);
        }

        size_t GetUsedMemory() const override {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return m_allocator.GetUsedMemory();
        }

        size_t GetTotalMemory() const override {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return m_allocator.GetTotalMemory();
        }

        std::string GetName() const override { return m_name; }

        // �⺻ �Ҵ����� Ư�� �޼��忡 ���� ����
        template<typename F, typename... Args>
        auto InvokeMethod(F&& method, Args&&... args) {
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            return (m_allocator.*method)(std::forward<Args>(args)...);
        }

        // �� ȹ��/���� ���� ���� (�����ؼ� ���)
        void Lock() { m_mutex.lock(); }
        void Unlock() { m_mutex.unlock(); }
        void SharedLock() { m_mutex.lock_shared(); }
        void SharedUnlock() { m_mutex.unlock_shared(); }

    private:
        BaseAllocator m_allocator;                  // ������ �⺻ �Ҵ���
        mutable std::shared_mutex m_mutex;          // �б�-���� ���ؽ�
        std::string m_name;                         // �Ҵ��� �̸�
    };

    // �� ������ ��� �ʿ��� ��츦 ���� Lock ��ü
	template<typename Allocator>
    class ScopedLock {
    public:
        explicit ScopedLock(Allocator& allocator, bool shared = false)
            : m_allocator(allocator)
            , m_shared(shared) {
            if (m_shared) {
                m_allocator.SharedLock();
            }
            else {
                m_allocator.Lock();
            }
        }

        ~ScopedLock() {
            if (m_shared) {
                m_allocator.SharedUnlock();
            }
            else {
                m_allocator.Unlock();
            }
        }

    private:
        IAllocator& m_allocator;
        bool m_shared;
    };

    using ThreadSafeLinearAllocator = ThreadSafeAllocator<LinearAllocator>;
    using ThreadSafeStackAllocator = ThreadSafeAllocator<StackAllocator>;
    using ThreadSafePoolAllocator = ThreadSafeAllocator<PoolAllocator>;

    // ���ø� Ưȭ�� Ǯ �Ҵ��ڸ� ���� ������ ������ ����
    template<typename T, size_t BlockCount>
    class ThreadSafeTypedPoolAllocator 
        : public ThreadSafeAllocator<TypedPoolAllocator<T, BlockCount>> {
    public:
        explicit ThreadSafeTypedPoolAllocator(const char* name = "ThreadSafeTypedPool")
            : ThreadSafeAllocator<TypedPoolAllocator<T, BlockCount>>(name) { }

        // Ÿ�� �������� ����� �Ҵ�/���� �޼���
        T* AllocateTyped() {
            auto result = this->Allocate(sizeof(T), alignof(T));
            return static_cast<T*>(result.ptr);
        }

        void DeallocateTyped(T* ptr) {
            this->Deallocate(ptr);
        }
    };
}