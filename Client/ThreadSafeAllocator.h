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
        // 기본 생성자를 통한 래핑
        template<typename... Args>
        explicit ThreadSafeAllocator(Args&&... args)
            : m_allocator(std::forward<Args>(args)...)
            , m_name("ThreadSafe" + std::string(m_allocator.GetName())) {
            Logger::Instance().Info("[{}] Created", m_name);
        }

        ~ThreadSafeAllocator() override {
            Logger::Instance().Info("[{}] Destroyed", m_name);
        }

        // 복사 금지
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

        // 기본 할당자의 특수 메서드에 대한 접근
        template<typename F, typename... Args>
        auto InvokeMethod(F&& method, Args&&... args) {
            std::lock_guard<std::shared_mutex> lock(m_mutex);
            return (m_allocator.*method)(std::forward<Args>(args)...);
        }

        // 락 획득/해제 수동 제어 (주의해서 사용)
        void Lock() { m_mutex.lock(); }
        void Unlock() { m_mutex.unlock(); }
        void SharedLock() { m_mutex.lock_shared(); }
        void SharedUnlock() { m_mutex.unlock_shared(); }

    private:
        BaseAllocator m_allocator;                  // 래핑할 기본 할당자
        mutable std::shared_mutex m_mutex;          // 읽기-쓰기 뮤텍스
        std::string m_name;                         // 할당자 이름
    };

    // 더 세밀한 제어가 필요한 경우를 위한 Lock 객체
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

    // 템플릿 특화된 풀 할당자를 위한 스레드 세이프 버전
    template<typename T, size_t BlockCount>
    class ThreadSafeTypedPoolAllocator 
        : public ThreadSafeAllocator<TypedPoolAllocator<T, BlockCount>> {
    public:
        explicit ThreadSafeTypedPoolAllocator(const char* name = "ThreadSafeTypedPool")
            : ThreadSafeAllocator<TypedPoolAllocator<T, BlockCount>>(name) { }

        // 타입 안전성이 보장된 할당/해제 메서드
        T* AllocateTyped() {
            auto result = this->Allocate(sizeof(T), alignof(T));
            return static_cast<T*>(result.ptr);
        }

        void DeallocateTyped(T* ptr) {
            this->Deallocate(ptr);
        }
    };
}