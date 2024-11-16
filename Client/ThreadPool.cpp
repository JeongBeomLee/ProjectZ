#include "pch.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount)
    : m_stop(false)
{
    // 작업자 스레드 생성
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back([this] { WorkerThread(); });
    }
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::Shutdown() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }

    m_condition.notify_all();

    // 모든 작업자 스레드 종료 대기
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // 작업이 있거나 종료 신호가 올 때까지 대기
            m_condition.wait(lock, [this] {
                return !m_jobs.empty() || m_stop;
                });

            // 종료 조건 체크
            if (m_stop && m_jobs.empty()) {
                return;
            }

            // 작업 가져오기
            job = std::move(m_jobs.front());
            m_jobs.pop();
        }

        // 작업 실행
        job();
    }
}