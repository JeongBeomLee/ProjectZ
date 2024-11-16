#include "pch.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount)
    : m_stop(false)
{
    // �۾��� ������ ����
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

    // ��� �۾��� ������ ���� ���
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

            // �۾��� �ְų� ���� ��ȣ�� �� ������ ���
            m_condition.wait(lock, [this] {
                return !m_jobs.empty() || m_stop;
                });

            // ���� ���� üũ
            if (m_stop && m_jobs.empty()) {
                return;
            }

            // �۾� ��������
            job = std::move(m_jobs.front());
            m_jobs.pop();
        }

        // �۾� ����
        job();
    }
}